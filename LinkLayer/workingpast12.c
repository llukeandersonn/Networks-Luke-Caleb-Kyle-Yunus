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
uint32_t third_delta = 0;
int bit_count = 0;
int valid_read_rate = 0;
uint32_t avg_read_rate = 0;
    int bits[20];

// Function to decode Manchester bit based on transitions
int decode_manchester_bit(int last_state, int current_state) {
    if (last_state == 0 && current_state == 1) {
	printf("bit: 1");    
        return 1;  // Low-to-High transition represents 1
    } else if (last_state == 1 && current_state == 0) {
	printf("bit: 0");    
        return 0;  // High-to-Low transition represents 0
    }
    return -1;  // Error in decoding
}

// Callback function triggered on GPIO state changes (receiver)
void gpio_state_change_callback(int pi, unsigned gpio, unsigned level, uint32_t tick) {
    // Print the GPIO pin, the new state (level), and the current tick
    printf("GPIO %d changed to %d at tick: %u microseconds\n", gpio, level, tick);
    //receive_manchester_encoded_message(pi, 26);

    if (bit_count == 0) {
        // First transition detected
        last_tick = tick;
        bit_count++;
    } else if (bit_count == 1) {
        // Second transition detected
        last_tick = tick;
        bit_count++;
    } else if (bit_count == 2) {
        // Third transition detected, calculate delta t
        last_tick = tick;
        bit_count++;
    } else if (bit_count == 3) {
        // Fourth transition detected, calculate second delta
        second_delta = tick - last_tick;
        //avg_read_rate = (first_delta + second_delta) / 2;
        last_tick = tick;
        bit_count++;
        valid_read_rate = 1;
        printf("Second delta: %u microseconds\n", second_delta);
        printf("Average read rate: %u microseconds\n", avg_read_rate);
    } else if (bit_count == 4) {
        // Fifth transition detected, calculate third delta
        third_delta = tick - last_tick;
        avg_read_rate = (second_delta + third_delta) / 2;
        last_tick = tick;
        bit_count++;
        valid_read_rate = 1;
        printf("Third delta: %u microseconds\n", third_delta);
        printf("Average read rate: %u microseconds\n", avg_read_rate);
    } else {
        // For subsequent transitions, validate based on average read rate
        uint32_t delta = tick - last_tick;

        uint32_t lower_bound = avg_read_rate - (avg_read_rate / 3);
        uint32_t upper_bound = avg_read_rate + (avg_read_rate / 3);

        if (delta >= lower_bound && delta <= upper_bound) {
            // Valid bit received within the acceptable timing range
            last_tick = tick;
            printf("Valid bit received with delta: %u microseconds\n", delta);
	    bits[bit_count-5]=level;
	    bit_count++;
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
// Function to add a parity bit to the given message
// Even 1s
void add_parity_bit(int* message, int size){
    //initialize 1 counter
    int ct = 0;
    //loop through message and count 1s
for(int i = 0; i < size; i++){
    if(message[i] == 1){
        ct++;
    }
}
//if there's already an even number, put a 0 on the end
if(ct % 2 == 0){
    message[size-1] = 0;
//otherwise, put a 1 on the end
}else{
    message[size-1] = 1;
}


}
void check_parity_bit(int* message, int size){
    //initialize 1 counter
    int ct = 0;
    //loop through message and count 1s
for(int i = 0; i < size; i++){
    if(message[i] == 1){
        ct++;
    }
}
//if there's an even number, print good bits
if(ct % 2 == 0){
    printf("Good Bits!");}
//otherwise, print bad bits
else{
    printf("Bad Bits!");
}}




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
    
    regularize();  // Regularize GPIO pins before sending
    time_sleep(5);
    pthread_t listener_thread;

    // Start listener thread to listen for Manchester-encoded messages on GPIO 26
    if (pthread_create(&listener_thread, NULL, listen, NULL)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    int head[] = {1, 0, 1, 0, 1};   // Header for synchronization
    int biit[] = {1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1};  // Message to be sent
    int userInput[17]; //initialize input array
    printf("Enter your desired bits (16 required) \n"); //prompt user
    for(int i = 0; i < 16; i++){ //loop through each element of array, update with applicable bit
    	scanf("%d", &userInput[i]);
    }
    int sOhead = sizeof(head) / sizeof(head[0]);
    int sObits = sizeof(biit) / sizeof(biit[0]);
    int sOinput = sizeof(userInput) / sizeof(userInput[0]);

    //time_sleep(1);
    // Send Manchester-encoded header and message
    int *ui_ptr = userInput;
    add_parity_bit(ui_ptr, sOinput);

    sendFunction(head, sOhead);
   // sendFunction(biit, sObits);
    sendFunction(userInput, sOinput);
    time_sleep(5);

    // Stop listener thread after communication is done
    stop_thread(&listener_thread);
    int *bit_ptr = bits;
    int sOmess = sizeof(bits) / sizeof(bits[0]);
    for(int loop = 0; loop < 21; loop++){
      printf("%d ", bits[loop]);
    }
    printf("\n");
//    check_parity_bit(bit_ptr, sOmess);
    return 0;
}
