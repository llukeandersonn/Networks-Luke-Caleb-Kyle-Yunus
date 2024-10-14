#include <stdio.h>
#include "pigpiod_if2.h"
#include <time.h>

// Function to set up and send a Manchester-encoded bit using pigpio's wave features
void send_manchester_bit_wave(int pi, int gpio, int bit) {
    // Define the pulses (low-to-high or high-to-low transitions for each bit)
    gpioPulse_t pulse[2];  // Pulse structure to define wave

    if (bit == 1) {
        pulse[0].gpioOn = (1 << gpio);   // Low-to-high transition for 1
        pulse[0].gpioOff = 0;
        pulse[0].usDelay = 500;          // Half-bit duration

        pulse[1].gpioOn = 0;
        pulse[1].gpioOff = (1 << gpio);  // Return to low
        pulse[1].usDelay = 500;
    } else {
        pulse[0].gpioOn = 0;
        pulse[0].gpioOff = (1 << gpio);  // High-to-low transition for 0
        pulse[0].usDelay = 500;

        pulse[1].gpioOn = (1 << gpio);   // Return to high
        pulse[1].gpioOff = 0;
        pulse[1].usDelay = 500;
    }

    // Create and transmit the wave based on the pulses
    wave_add_generic(pi, 2, pulse);
    int wave_id = wave_create(pi);  // Create a wave using the pulses
    wave_send_once(pi, wave_id);  // Send the wave once

    while (wave_tx_busy(pi)) {    // Wait for the wave to be transmitted
        time_sleep(0.001);
    }
    wave_delete(pi, wave_id);     // Cleanup the wave after transmission
}

// Function to send sync signal using pigpio's wave features
void send_sync_signal_wave(int pi, int gpio) {
    for (int i = 0; i < 8; i++) {
        send_manchester_bit_wave(pi, gpio, i % 2);  // Send alternating bits (01010101)
    }
}

// Function to send a Manchester-encoded message
void send_manchester_encoded_message(int pi, int gpio, const char *message) {
    send_sync_signal_wave(pi, gpio);  // Send sync signal before the message

    for (int i = 0; message[i] != '\0'; i++) {
        char byte = message[i];
//	printf("%c", byte);
        for (int j = 7; j >= 0; j--) {
            int bit = (byte >> j) & 1;  // Extract each bit from the byte
            send_manchester_bit_wave(pi, gpio, bit);  // Send each bit using Manchester encoding
        }
    }
}

void regularize() {
    int pi = pigpio_start(NULL, NULL);  // Connect to pigpiod daemon
    if (pi < 0) {
        printf("Failed to connect to pigpiod daemon\n");
        return;
    }

    set_mode(pi, 23, PI_OUTPUT);  // Set GPIO pin 23 as output (transmitter)

    char userInput[64];
    printf("Enter your desired message: ");
    fgets(userInput, sizeof(userInput), stdin);

    send_manchester_encoded_message(pi, 23, userInput);  // Send the Manchester-encoded message
    
    pigpio_stop(pi);  // Disconnect from pigpiod daemon
}

int main() {
    regularize();  // Start sending the Manchester-encoded message
    return 0;
}
