#include <stdio.h>
#include <pigpiod_if2.h>
#include <time.h>
#define HEADER "HEADER"
// Function to decode Manchester bit based on transitions
int decode_manchester_bit(int last_state, int current_state) {
    if (last_state == 0 && current_state == 1) {
	printf("byte: 1");    
        return 1;  // Low-to-High transition represents 1
    } else if (last_state == 1 && current_state == 0) {
	printf("byte: 0");    
        return 0;  // High-to-Low transition represents 0
    }
    return -1;  // Error in decoding
}

// Function to receive and decode Manchester-encoded message
void receive_manchester_encoded_message(int pi, int gpio) {
    int last_state = gpio_read(pi, gpio);
    int bit_count = 0;
    char received_byte = 0;
    int header_found = 0;
    while (1) {
        int current_state = gpio_read(pi, gpio);
	printf("Last state: %d, Current State: %d\n", last_state, current_state);
	int decoded_bit = decode_manchester_bit(last_state, current_state);

        if (decoded_bit != -1) {
            received_byte = (received_byte << 1) | decoded_bit;
            bit_count++;

            if (bit_count == 8) {
		    if (!header_found) {
			if (received_byte == &HEADER) {
				printf("Header found. \n");
				header_found = 1;
			} else {
				printf("Header not found. \n");
				received_byte = 0;
			}	
		    }	else {

                // Full byte received, print it out
            //      printf("%c", received_byte);
          //        fflush(stdout);
	        printf("Received Byte: %02X\n", received_byte);
                received_byte = 0;
                bit_count = 0;
            }
        }
	    }else{
	printf("Error");

	    }
        last_state = current_state;
        time_sleep(0.001);  // Adjust timing as needed
    }
}

// Listener function modified for Manchester encoding
void *listen(void *arg) {
    int pi = pigpio_start(NULL, NULL);  // Connect to pigpiod daemon
    if (pi < 0) {
        printf("Failed to connect to pigpiod daemon\n");
        return NULL;
    }

    set_mode(pi, 26, PI_INPUT);  // Set GPIO pin 26 as input (receiver)

    printf("Listening for Manchester-encoded message:\n");
    receive_manchester_encoded_message(pi, 26);  // Start receiving message

    pigpio_stop(pi);  // Disconnect from pigpiod daemon
    return NULL;
}

int main() {
    pthread_t p1;
    pthread_create(&p1, NULL, listen, NULL);  // Start listener thread
    pthread_join(p1, NULL);  // Wait for listener to finish

    return 0;
}
