#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include "pigpiod_if2.h"

// Global variables for time tracking
uint32_t last_tick = 0;
uint32_t first_delta = 0;
uint32_t second_delta = 0;
int bit_count = 0;
int valid_read_rate = 0;
uint32_t avg_read_rate = 0;

// Callback function triggered on GPIO state changes (receiver)
void gpio_state_change_callback(int pi, unsigned gpio, unsigned level, uint32_t tick) {
    // Print the GPIO pin, the new state (level), and the current tick
    printf("GPIO %d changed to %d at tick: %u microseconds\n", gpio, level, tick);

    if (bit_count == 0) {
        // First transition detected
        last_tick = tick;
        bit_count++;
    } else if (bit_count == 1) {
        // Second transition detected, calculate delta t
       // first_delta = tick - last_tick;
	first_delta = 50000;
        last_tick = tick;
        bit_count++;
        printf("First delta: %u microseconds\n", first_delta);
    } else if (bit_count == 2) {
        // Third transition detected, calculate second delta
        second_delta = tick - last_tick;
        avg_read_rate = (first_delta + second_delta) / 2;
        last_tick = tick;
        bit_count++;
        valid_read_rate = 1;
        printf("Second delta: %u microseconds\n", second_delta);
        printf("Average read rate: %u microseconds\n", avg_read_rate);
    } else {
        // For subsequent transitions, validate based on average read rate
        uint32_t delta = tick - last_tick;
        last_tick = tick;

        uint32_t lower_bound = avg_read_rate - (avg_read_rate / 3);
        uint32_t upper_bound = avg_read_rate + (avg_read_rate / 3);

        if (delta >= lower_bound && delta <= upper_bound) {
            // Valid bit received within the acceptable timing range
            printf("Valid bit received with delta: %u microseconds\n", delta);
        } else {
            // Ignore invalid transitions
            printf("Invalid bit received (outside timing window) with delta: %u microseconds\n", delta);
        }
    }
}

// Manchester encoding for sending a bit (transmitter)
void sendManchesterEncodedBit(int pi, int gpio, int bit) {
    if (bit == 1) {
        // Manchester encoding for bit 1 (Low-to-High transition)
        gpio_write(pi, gpio, 0);
        time_sleep(0.05);  // Half-bit period
        gpio_write(pi, gpio, 1);
        time_sleep(0.05);  // Half-bit period
    } else if (bit == 0) {
        // Manchester encoding for bit 0 (High-to-Low transition)
        gpio_write(pi, gpio, 1);
        time_sleep(0.05);  // Half-bit period
        gpio_write(pi, gpio, 0);
        time_sleep(0.05);  // Half-bit period
    }
}

// Function to send a Manchester-encoded message
void sendFunction(int bits[], int sObits) {
    int pi = pigpio_start(NULL, NULL);
    set_mode(pi, 23, PI_OUTPUT);  // GPIO 23 used for transmitting data

    for (int i = 0; i < sObits; i++) {
        // Send each bit using Manchester encoding
        sendManchesterEncodedBit(pi, 23, bits[i]);
    }

    // Print current tick after sending the bits
    uint32_t current_tick = get_current_tick(pi);  // Get the current tick in microseconds
    printf("Message sent at tick: %u microseconds\n", current_tick);

    pigpio_stop(pi);  // Stop pigpio
}

// Listener function to detect Manchester-encoded messages (receiver)
void *listen(void *arg) {
    int pi = pigpio_start(NULL, NULL);
    set_mode(pi, 26, PI_INPUT);  // GPIO 26 used for receiving data

    // Set up the callback on GPIO 26 to trigger on both rising and falling edges
    callback(pi, 26, EITHER_EDGE, gpio_state_change_callback);

    printf("Listening for Manchester-encoded message on GPIO 26...\n");

    // Keep the program running to process callbacks
    while (1) {
        time_sleep(1);  // Sleep to keep the listener active
    }

    pigpio_stop(pi);  // Stop pigpio
    return NULL;
}

// Function to initialize the GPIO pins and regularize them
void regularize() {
    // Initialize pigpio library and GPIO pins
    int pi = pigpio_start(NULL, NULL);

    set_mode(pi, 23, PI_OUTPUT);  // GPIO 23 as output (transmitter)
    set_mode(pi, 26, PI_INPUT);   // GPIO 26 as input (receiver)

    // Reset GPIO pins (set to high then low)
    gpio_write(pi, 23, 1);
    gpio_write(pi, 23, 0);

    pigpio_stop(pi);  // Stop pigpio
}

// Function to stop a thread
void stop_thread(pthread_t *pth) {
    pthread_cancel(*pth);  // Cancels the thread
    pthread_join(*pth, NULL);  // Waits for the thread to terminate
}

int main(int argc, char *argv[]) {  
    time_sleep(1);
    pthread_t listener_thread;

    // Start listener thread to listen for Manchester-encoded messages on GPIO 26
    if (pthread_create(&listener_thread, NULL, listen, NULL)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    int head[] = {1, 1, 1, 1};  // Header for synchronization
    int biit[] = {1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1};  // Message to be sent

    int sOhead = sizeof(head) / sizeof(head[0]);
    int sObits = sizeof(biit) / sizeof(biit[0]);

    regularize();  // Regularize GPIO pins before sending
    time_sleep(0.5);

    // Send Manchester-encoded header and message
    sendFunction(head, sOhead);
    sendFunction(biit, sObits);

    time_sleep(1);

    // Stop listener thread after communication is done
    stop_thread(&listener_thread);

    return 0;
}
