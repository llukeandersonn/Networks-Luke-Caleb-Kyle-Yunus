#include <stdio.h>
#include <pigpiod_if2.h>
#include <time.h>

// Manchester encoding function for sending a bit via pigpiod API
void send_manchester_bit(int pi, int gpio, int bit) {
    if (bit == 1) {
        gpio_write(pi, gpio, 0);  // Low-to-High transition for 1
        time_sleep(0.002);        // Short pulse
        gpio_write(pi, gpio, 1);
    } else {
        gpio_write(pi, gpio, 1);  // High-to-Low transition for 0
        time_sleep(0.002);        // Short pulse
        gpio_write(pi, gpio, 0);
    }
    time_sleep(0.002);  // Pause between bits
}

// Function to send Manchester-encoded message
void send_manchester_encoded_message(int pi, int gpio, const char *message) {
    int i, j;
    const char* header = "HEADER";
    for (int k = 0; header[k] != '\0'; k++) {
	char byte1 = header[k];
	for (int l = 7; l >= 0; l--) {
		int bit1 = (byte1 >> l) & 1;
		send_manchester_bit(pi, gpio, bit1);

	}
    }




    for (i = 0; message[i] != '\0'; i++) {
        char byte = message[i];
        for (j = 7; j >= 0; j--) {
            int bit = (byte >> j) & 1;  // Extract each bit from byte
            send_manchester_bit(pi, gpio, bit);
	    printf("Bit: %02X\n", bit);
        }
    }
}

// Original regularize function modified for Manchester encoding
void regularize() {
    int pi = pigpio_start(NULL, NULL);  // Connect to pigpiod daemon
    if (pi < 0) {
        printf("Failed to connect to pigpiod daemon\n");
        return;
    }

    set_mode(pi, 23, PI_OUTPUT);  // Set GPIO pin 23 as output (transmitter)

    const char *message1 = "Hello Pi";
    char userInput[64];
    printf("Enter your desired message! \n");
    fgets(userInput, sizeof(userInput), stdin);
    
    send_manchester_encoded_message(pi, 23, userInput);  // Send message
   // send_manchester_encoded_message(pi, 23, message2);
    pigpio_stop(pi);  // Disconnect from pigpiod daemon
}

int main() {
    regularize();  // Start sending Manchester-encoded message
    return 0;
}
