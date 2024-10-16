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
#include <stdlib.h>
#include <stdbool.h>
#define MAX_SIZE 50

// Global variables for time tracking
pthread_mutex_t mutex;
uint32_t last_tick = 0;
uint32_t first_delta = 0;
uint32_t second_delta = 0;
uint32_t third_delta = 0;
int bit_count = 0;
int valid_read_rate = 0;
uint32_t avg_read_rate = 0;
int* bits = NULL;
char* buf = NULL;
char* recv = NULL;
int* bin = NULL;
struct thread_stuff{
	int pi_thread;
};
struct send_stuff{
	int pi_send;
	int* msg;
	int size;
};
bool thread_term=false;
//Method to convert characters to an 'array' of their binary ASCII values
void strToBin(int *bin_arr, int bin_size, size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    for (i = 0; i < size; i++) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            //printf("%u", byte);
            bin_arr[(((i+1)*8)-(j+1))] = byte;
        }
    }
}

void binToStr(int *bin_arr, char *char_arr, int char_size) {
    for (int i = 0; i < char_size; i++) {
        char car = 0b00000000;
        for (int j = 0; j < 8; j++) {
            car |= (bin_arr[8 * i + j] << (7 - j));  // Bit shifting to form each character
        }
        char_arr[i] = car;  // Store the character in the array
    }
}
bool checkLast16Zeroes(int *arr, int size) {
    if (size < 16) {
        // If the array has fewer than 16 elements, return false
        return false;
    }
    
    // Check the last 16 elements
    for (int i = size - 16; i < size; i++) {
        if (arr[i] != 0) {
            return false;  // Return false if any of the last 16 elements is non-zero
        }
    }
    
    return true;  // Return true if all 16 elements are zero
}

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
    //printf("GPIO %d changed to %d at tick: %u microseconds\n", gpio, level, tick);
    //receive_manchester_encoded_message(pi, 26);
    pthread_mutex_lock(&mutex);
    if (bit_count == 0 && level == 1) {
        // First transition detected
        last_tick = tick;
        bit_count++;
    } else if (bit_count == 1 && level == 0) {
        // Second transition detected
        bit_count++;
	if (tick-last_tick > 120000) {
		bit_count = 0;
	}
        last_tick = tick;
    } else if (bit_count == 2 && level == 1) {
        // Third transition detected, calculate delta t
        bit_count++;
	if (tick-last_tick > 120000) {
		bit_count = 0;
	}
        last_tick = tick;
    } else if (bit_count == 3 && level == 0) {
        // Fourth transition detected, calculate second delta
        second_delta = tick - last_tick;
        //avg_read_rate = (first_delta + second_delta) / 2;
        bit_count++;
	if (tick-last_tick > 120000) {
		bit_count = 0;
	}
        last_tick = tick;
        valid_read_rate = 1;
        printf("Second delta: %u microseconds\n", second_delta);
        printf("Average read rate: %u microseconds\n", avg_read_rate);
    } else if (bit_count == 4 && level == 1) {
        // Fifth transition detected, calculate third delta
        third_delta = tick - last_tick;
        avg_read_rate = (second_delta + third_delta) / 2;
        bit_count++;
	if (tick-last_tick > 120000) {
		bit_count = 0;
	}
        last_tick = tick;
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
          //  printf("Valid bit received with delta: %u microseconds\n", delta);
	    bits[bit_count-5]=level;
	  //  printf("Received bit: %d at index %d\n", level, bit_count-5);
        bit_count++;

        if(checkLast16Zeroes(bits, bit_count-5)){
            bit_count = 0;
            binToStr(bits, recv, MAX_SIZE);
            printf("Received message: %s\n", recv);
            printf("\n");
            


        for (int m=0; m<MAX_SIZE; m++){
	    printf("%c", recv[m]);
        }
  

        }
        } else {
            // Ignore invalid transitions
            //printf("Invalid bit received (outside timing window) with delta: %u microseconds\n", delta);


        }

    }
    pthread_mutex_unlock(&mutex);
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
void sendFunction(int pi, int* bits, int sObits) {
    set_mode(pi, 23, PI_OUTPUT);  // GPIO 23 used for transmitting data

    for (int i = 0; i < sObits; i++) {
        // Send each bit using Manchester encoding
        sendManchesterEncodedBit(pi, 23, bits[i]);
    }

    // Print current tick after sending the bits
    uint32_t current_tick = get_current_tick(pi);  // Get the current tick in microseconds
    printf("Message sent at tick: %u microseconds\n", current_tick);

}

// Listener function to detect Manchester-encoded messages (receiver)
void *listen(void* i) {

    struct thread_stuff *data=(struct thread_stuff*) i;

    set_mode(data->pi_thread, 23, PI_OUTPUT);  // GPIO 23 used for transmitting data

    // Set up the callback on GPIO 26 to trigger on both rising and falling edges
    callback(data->pi_thread, 26, EITHER_EDGE, gpio_state_change_callback);

    printf("Listening for Manchester-encoded message on GPIO 26...\n");
    
    // Keep the program running to process callbacks
    while (1) {
        time_sleep(1);  // Sleep to keep the listener active
	if(thread_term){
		pthread_exit((void *)1);
    }

    return NULL;
}
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
void regularize(int pi) {
    // Initialize pigpio library and GPIO pins

    set_mode(pi, 23, PI_OUTPUT);  // GPIO 23 as output (transmitter)
    set_mode(pi, 26, PI_INPUT);   // GPIO 26 as input (receiver)

    // Reset GPIO pins (set to high then low)
    gpio_write(pi, 23, 1);
    gpio_write(pi, 23, 0);

}
// Sender function to send Manchester-encoded messages (transmitter)
void *oursend(void* i) {
    struct send_stuff *sdata=(struct send_stuff*) i;

    pthread_mutex_lock(&mutex);

    int trailer[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int head[] = {1, 0, 1, 0,1};   // Header for synchronization
    int sOhead = sizeof(head) / sizeof(head[0]);
    int sOtrailer = sizeof(trailer) / sizeof(trailer[0]);
    sendFunction(sdata->pi_send, head, sOhead);
    sendFunction(sdata->pi_send, sdata->msg, sdata->size);
    sendFunction(sdata->pi_send, trailer, sOtrailer);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);

}





int main(int argc, char *argv[]) {  
    int pi = pigpio_start(NULL, NULL);
    //make stricts for threads 
    struct thread_stuff *data= (struct thread_stuff*)calloc(1,sizeof(struct thread_stuff));
    struct send_stuff *sdata = (struct send_stuff*)calloc(1, sizeof(struct send_stuff));
    //p[i for send listen thread
    data->pi_thread=pi;

    pthread_mutex_init(&mutex, NULL);

    regularize(pi);  // Regularize GPIO pins before sending
    time_sleep(2); //sleep so dont read regularize
   //declare threads
   pthread_t listener_thread;
    pthread_t sender_thread;
    // Start listener thread to listen for Manchester-encoded messages on GPIO 26
    if (pthread_create(&listener_thread, NULL, listen,(void *) data)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
    while(1){
    bits = calloc(MAX_SIZE * 8, sizeof(int));
    recv = calloc(MAX_SIZE + 1, sizeof(char));
    int k = 0;
    ssize_t bufsize = 0;
    ssize_t input_size;
    
    //ask user for input
    printf("\nEnter a string (no size limit, type 'exit' to quit):\n");
    //get user input
    input_size = getline(&buf, &bufsize, stdin);  // Use getline for flexible inputreturn 0;}
    bin = calloc((input_size) * 8, sizeof(int));
   //if user input is exit end program
    void *thread_return_value;
    if(strncmp(buf, "exit", 4) == 0){
	thread_term=true;
    pthread_join(listener_thread, &thread_return_value);
	    break;
    }
    //print string going to send
    printf("string is: %s", buf);

    //fill bin with 0s?????
    for (int k = 0; k < input_size*8; k++) {
         bin[k]=0;
    }
   
    //convert string to binary
    strToBin(bin, (input_size-1)*8, (input_size-1), buf);
    
      //print binary before send
    printf("\nBinary is: ");
    for (int k = 0; k < (input_size-1)*8; k++) {
        printf("%d", bin[k]);
    }
    
    int sObin = (input_size-1) * 8;
    int *bin_arr = bin;
    // Send Manchester-encoded header and message
    
    
    //add_parity_bit(bin_arr, sObin);
    sdata->pi_send = pi;
    sdata->size = sObin;
    sdata->msg = bin;
    
    if (pthread_create(&sender_thread, NULL, oursend,(void *) sdata)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
    //parody bit stuff saved for later
    // Stop listener thread after communication is done//int *bit_ptr = bits;
    //int sOmess = sizeof(bits) / sizeof(bits[0]);
    //check_parity_bit(bit_ptr, sOmess);
    
    //sleep so dont free before recove 
    time_sleep(15);

    
    
    free(bin);
    free(bits);
    free(buf);
    free(recv);
    
    pthread_join(sender_thread, &thread_return_value);


}
    void *thread_return_value;
    //free(*data->pi_thread);
    free(data);
    //free(bin);

    pthread_mutex_destroy(&mutex);

    free(bits);
    free(buf);
    free(recv);
    pigpio_stop(pi);
    //free(*sdata->pi_thread);
    //free(*sdata->size);
    //free(*sdata->msg);
    free(sdata);
    pthread_join(sender_thread, &thread_return_value);
    printf("stopped pigpio");
    return 0;
}
