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
#include <stdint.h>
#define MAX_SIZE 500

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
char* filen =NULL;
char* file_buf = NULL;
char* read_buf = NULL;
int* bin = NULL;
int* in_address;
int* from_address;
int* out_address;
int address;
int* ptr;
char* out_add_char;
bool file;
int forward = 0;
bool terminate=false;
struct thread_stuff{
	int pi_thread;
};
struct send_stuff{
	int pi_send;
    int* out_address;
    int* our_address;
	int* msg;
	int size;
    int* type;
};
bool thread_term=false;
bool in_file=false;
void clear_ts() {
int c;
while ((c = getchar()) != '\n' && c != EOF);
}

FILE* writeToFile(char* buffer) {
    char* filename = calloc(20, sizeof(char));
    int n;
    clear_ts();
    printf("Enter the filename with .txt extension: "); 
    if (fgets(filename, sizeof(filename), stdin) == NULL) { 
        printf("Fail to read the input stream"); 
    } 
    else { 
        filename[strcspn(filename, "\n")] = '\0'; 
    }
    //clear_ts(); 
    printf("Entered Data = %s\n", filename); 

    printf(" Input the number of lines to be written : ");
	scanf("%d", &n);

    // Declare the file pointer
    FILE* filePointer;

    // Get the data to be written in file
    printf("Enter the string: ");
    //fgets(buf, 1000, stdin);

    // Open the existing file GfgTest.c using fopen()
    // in write mode using "w" attribute
    filePointer = fopen(filename, "w");

    // Check if this filePointer is null
    // which maybe if the file does not exist
    if (filePointer == NULL) {
        printf("your text file failed to open.");
    }
    else {

        printf("The file is now opened.\n");

        // Write the dataToBeWritten into the file
        for (int h = 0; h < n+1; h++) {
            fgets(buffer, 100, stdin);
            if (strlen(buffer) > 0) {
            // writing in the file using fputs()
            fputs(buffer, filePointer);
            //fputs("\n", filePointer);
        }
        }
        

        // Closing the file using fclose()
        fclose(filePointer);

        printf("Data successfully written in file %s\n", filename);
        printf("The file is now closed.\n");
    }
    free(filename);
    return filePointer;
}

char* readFromFile(char* reading_char) {
    FILE* filePointer;
    char* code = calloc(100, sizeof(char));
    // Opening file in reading mode
    char* filename = calloc(20, sizeof(char));
    clear_ts();
    printf("Enter the filename with .txt extension: ");

    if (fgets(filename, sizeof(filename), stdin) == NULL)
        {printf("Fail to read the input stream");}
    else
        {filename[strcspn(filename, "\n")] = '\0';}

    printf("Entered Data = %s\n", filename);
    filePointer = fopen(filename, "r");

    if (NULL == filePointer) {
        printf("file can't be opened \n");
    }

    printf("Content of the file are:-: \n");

    // Printing what is written in file
    // character by character using loop.
    
    size_t i = 0;
    int c;
    clear_ts();
    while ((c = fgetc(filePointer)) != EOF)  {
        printf("%c", (char) c);
        reading_char[i++] = (char) c;
    }
    reading_char[i] = '\0';
    free(filename);
    free(code);
    fclose(filePointer);
    return reading_char;
}
void create_new_file(const char *file_name) {
    // Try to remove the file if it exists
    if (remove(file_name) == 0) {
        printf("Existing file \"%s\" deleted successfully.\n", file_name);
    } else {
        printf("No existing file found or failed to delete \"%s\".\n", file_name);
    }

    // Create a new file
    FILE *file = fopen(file_name, "w");
    if (!file) {
        perror("Failed to create new file");
        return;
    }

    printf("New file \"%s\" created successfully.\n", file_name);
    fclose(file);
}
void append_bits_to_file_as_characters(const int *bit_array, size_t length, const char *file_name) {
    FILE *file = fopen(file_name, "ab");  // Open the file in append-binary mode
    if (!file) {
        perror("Failed to open file");
        return;
    }

    unsigned char byte = 0;  // A byte to store the 8 bits (will be converted to a character)
    int bit_count = 0;       // To count how many bits have been processed into the current byte

    // Loop through the array of bits and convert them into characters
    for (size_t i = 0; i < length; i++) {
        byte = (byte << 1) | bit_array[i];  // Shift left and add the bit to the byte
        bit_count++;

        // If we have collected 8 bits, convert to a character and write to the file
        if (bit_count == 8) {
            fwrite(&byte, sizeof(unsigned char), 1, file);  // Write the byte as a character to the file
            bit_count = 0;  // Reset the bit counter for the next byte
            byte = 0;       // Reset the byte
        }
    }

    // If there are remaining bits that didn't form a full byte, pad with 0s and write
    if (bit_count > 0) {
        byte = byte << (8 - bit_count);  // Shift the remaining bits to the left (padding with zeros)
        fwrite(&byte, sizeof(unsigned char), 1, file);
    }

    fclose(file);  // Close the file
}
int check_for_pattern(const int *bit_array, size_t length) {
    // The binary pattern "0000000000011000000000011"
    int pattern[25] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1};
    
    // Check if the bit array has at least 25 bits to compare
    if (length < 25) {
        return 0;  // Not enough bits to match the pattern
    }

    // Compare the first 25 bits of the array with the pattern
    for (int i = 0; i < 25; i++) {
        if (bit_array[i] != pattern[i]) {
            return 0;  // Mismatch found, return false
        }
    }

    // If all bits match, return true
    return 1;
}

// Function to read the file and return its binary representation
int* file_to_binary_array(const char *file_path, size_t *binary_length) {
    FILE *file = fopen(file_path, "rb");  // Open the file in binary mode
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    // Find the size of the file
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    if (file_size == 0) {
        fprintf(stderr, "File is empty.\n");
        fclose(file);
        return NULL;
    }

    // Allocate memory to store the binary representation as an integer array (8 integers per byte)
    size_t total_bits = file_size * 8;
    int *binary_array = (int *)malloc(total_bits * sizeof(int));
    if (!binary_array) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    // Read each byte from the file, convert to binary, and store as integers in the array
    unsigned char byte;
    size_t index = 0;
    while (fread(&byte, 1, 1, file) == 1) {
        for (int i = 7; i >= 0; --i) {
            binary_array[index++] = (byte & (1 << i)) ? 1 : 0;  // Store each bit as an int (0 or 1)
        }
    }

    // Close the file
    fclose(file);

    // Set the total length of the binary data (in bits)
    *binary_length = total_bits;
    
    return binary_array;
}
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
    for (int i = size - 16; i < size - 1; i++) {
        if (arr[i] != 0) {
		return false;
    }
    }
    if (arr[size-1] != 1){
	//    printf("size: %d", size);
	  //  printf("digit at end: %d", arr[size]);
	    return false;
    }    
    return true;  // Return true if all 16 elements match trailer}

}
// Function to convert message length to bits
void len_to_bits(int *binlength, int bit_length) {
    int i = 0;
    while(bit_length > 0) {
        binlength[7-i] = bit_length % 2;
        bit_length = bit_length / 2;
        i++;
    }
    //printf("%d, ", bit_length);

}

// Function to convert bits to message length
int bits_to_len(int* inlen) {
	

    int car = 0b00000000;
    for (int i = 7; i >= 0; i--) {
        car |= (inlen[i] << 7-i); 
    }
    //printf("%d", car);
    return car;
}
// Manchester encoding for sending a bit (transmitter)
void sendManchesterEncodedBit(int pi, int gpio, int bit) {
    if (bit == 1) {
        // Manchester encoding for bit 1 (Low-to-High transition)
        gpio_write(pi, gpio, 0);
        time_sleep(0.01);  // Half-bit period
        gpio_write(pi, gpio, 1);
        time_sleep(0.01);  // Half-bit period
    } else if (bit == 0) {
        // Manchester encoding for bit 0 (High-to-Low transition)
        gpio_write(pi, gpio, 1);
        time_sleep(0.01);  // Half-bit period
        gpio_write(pi, gpio, 0);
        time_sleep(0.01);  // Half-bit period
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
    //printf("Message sent at tick: %u microseconds\n", current_tick);

}

// Sender function to send Manchester-encoded messages (transmitter)
void *oursend(void* i) {
    struct send_stuff *sdata=(struct send_stuff*) i;

    pthread_mutex_lock(&mutex);

    int trailer[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    int head[] = {1, 0, 1, 0,1};   // Header for synchronization
    int sOhead = sizeof(head) / sizeof(head[0]);
    int sOtrailer = sizeof(trailer) / sizeof(trailer[0]);
    int sOaddy = 8;


    sendFunction(sdata->pi_send, head, sOhead);
    sendFunction(sdata->pi_send, sdata->out_address, sOaddy);
    sendFunction(sdata->pi_send, sdata->our_address, sOaddy);
    sendFunction(sdata->pi_send, sdata->type, 1);
  // printf("Sending to address: %d\n", sdata->out_address);
    sendFunction(sdata->pi_send, sdata->msg, sdata->size);

    
    sendFunction(sdata->pi_send, trailer, sOtrailer);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);

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
	if (tick-last_tick > 35000) {
		bit_count = 0;
	}
        last_tick = tick;
    } else if (bit_count == 2 && level == 1) {
        // Third transition detected, calculate delta t
        bit_count++;
	if (tick-last_tick > 35000) {
		bit_count = 0;
	}
        last_tick = tick;
    } else if (bit_count == 3 && level == 0) {
        // Fourth transition detected, calculate second delta
        second_delta = tick - last_tick;
        //avg_read_rate = (first_delta + second_delta) / 2;
        bit_count++;
	if (tick-last_tick > 35000) {
		bit_count = 0;
	}
        last_tick = tick;
        valid_read_rate = 1;
       // printf("Second delta: %u microseconds\n", second_delta);
    
    } else if (bit_count == 4 && level == 1) {
        // Fifth transition detected, calculate third delta
        third_delta = tick - last_tick;
        avg_read_rate = (second_delta + third_delta) / 2;
        bit_count++;
    	bits = calloc(MAX_SIZE * 8, sizeof(int));
	if (tick-last_tick > 35000) {
		
		bit_count = 0;
		free(bits);
	}
        last_tick = tick;
        valid_read_rate = 1;
       // printf("Third delta: %u microseconds\n", third_delta);
       //printf("Average read rate: %u microseconds\n", avg_read_rate);
    }else if (bit_count > 4 && bit_count < 13){
	   
        uint32_t delta = tick - last_tick;

        uint32_t lower_bound = avg_read_rate - (avg_read_rate / 3);
        uint32_t upper_bound = avg_read_rate + (avg_read_rate / 3);
        if (delta >= lower_bound && delta <= upper_bound) {
    	
	    	last_tick = tick;

       //printf("Received bit: %d at index %d\n", level, bit_count-5);
        	in_address[bit_count - 5] = level;
		//printf("Bit count: %d", bit_count);
        if (bit_count == 12){
            //make sure decoding is correct
            uint8_t right_addy = bits_to_len(in_address);
	  //  printf("Incoming address\n");
	    for(int i = 0; i < 8; i++){
	   	// printf("%d", in_address[i]);
	    }
	  //  printf("Right ADDY: %d \n Our ADDY: %d\n", right_addy, address);		
            if(right_addy == address){
                forward = 0;
            }else{
		forward = 1;
            }
	    //printf("Forward: %d", forward);
        }
	    bit_count++;
    
	}}else if (bit_count > 12 && bit_count < 21){
        uint8_t right_addy = bits_to_len(in_address);

	   
        uint32_t delta = tick - last_tick;

        uint32_t lower_bound = avg_read_rate - (avg_read_rate / 3);
        uint32_t upper_bound = avg_read_rate + (avg_read_rate / 3);
        if (delta >= lower_bound && delta <= upper_bound) {
    	
	    	last_tick = tick;

      // printf("Received bit: %d at index %d\n", level, bit_count-5);
        	from_address[bit_count - 13] = level;
		//printf("Bit count: %d", bit_count);
        if (bit_count == 20){
            //make sure decoding is correct
            uint8_t this_addy = bits_to_len(from_address);

	  //  printf("Right ADDY: %d \n Our ADDY: %d\n", right_addy, address);		
            if(this_addy == address){
                printf("The address %d was not found\n", right_addy);
		terminate=true;;
            }else{
            }
	   // printf("Forward: %d", forward);
        }
	    bit_count++;
    }
    }else if(bit_count==21){
        uint32_t delta = tick - last_tick;
       uint32_t lower_bound = avg_read_rate - (avg_read_rate / 3);
        uint32_t upper_bound = avg_read_rate + (avg_read_rate / 3);

        if (delta >= lower_bound && delta <= upper_bound) {
		
		pthread_mutex_unlock(&mutex);    
            last_tick = tick;
	    ptr = calloc(1, sizeof(int));
	    *ptr=level;
           if(*ptr==1){
                file=false;

            }else {

                file =true;
          }
	    bit_count++;
       //printf("Received bit: %d at index %d\n", level, bit_count);
        }


    }else if(bit_count > 21) {
        // For subsequent transitions, validate based on average read rate
	    uint32_t delta = tick - last_tick;

        uint32_t lower_bound = avg_read_rate - (avg_read_rate / 3);
        uint32_t upper_bound = avg_read_rate + (avg_read_rate / 3);

        if (delta >= lower_bound && delta <= upper_bound) {
            uint8_t from_addy = bits_to_len(from_address);
        
            // Valid bit received within the acceptable timing range
            last_tick = tick;
          // printf("Valid bit received with delta: %u microseconds\n", delta);
	    bits[bit_count-22]=level;
    //   printf("Received bit: %d at index %d\n", level, bit_count-5);
        bit_count++;

        if(bit_count>=16 && checkLast16Zeroes(bits, bit_count-22)){
           if(!forward&&!terminate){
                if(!file){
                recv = calloc(MAX_SIZE + 1, sizeof(char));
                binToStr(bits, recv, MAX_SIZE);
                printf("Received message: %s From address: %d\n", recv, from_addy);
                free(recv);
                }else if(file){
                     if(!in_file){
                     in_file=true;
                   filen = calloc(MAX_SIZE + 1, sizeof(char));
                    binToStr(bits, filen, MAX_SIZE);
                    create_new_file(filen);
                    printf("file recpetion started %s\n", filen);
                    }else if(in_file && check_for_pattern(bits,bit_count-22)){
                     printf("file recpetion complete and saved to %s\n", filen);
                     in_file=false;

                     free(filen);
                     
                     } else{
                    printf("file chunk received %s\n", filen);
                    append_bits_to_file_as_characters(bits, bit_count-38, filen);
                    }

                    
                     

                }
            }else{
            //TODO: ensure this 37 is correct
        	int sObits = bit_count-38;
   	        //printf("size of bits %d\n", sObits);
		//printf("\n ---we have entered the forwarding process---\n");
		pthread_mutex_unlock(&mutex);    
                pthread_t forward_thread;
                struct send_stuff *fdata = (struct send_stuff*)calloc(1, sizeof(struct send_stuff));
                fdata->pi_send = pi;
                fdata->out_address = in_address;
                fdata->msg = bits;
		fdata->size = sObits;
		fdata->our_address = from_address;
		printf("%d", *ptr);
        	fdata->type=ptr;
	    
    if (pthread_create(&forward_thread, NULL, oursend,(void *) fdata)) {
        fprintf(stderr, "Error creating thread\n");
        return;
    }
    void *thread_return_value;
    //parody bit stuff saved for later
    // Stop listener thread after communication is done//int *bit_ptr = bits;
    //int sOmess = sizeof(bits) / sizeof(bits[0]);
    //check_parity_bit(bit_ptr, sOmess);
    
    //sleep so dont free before recove 

   // printf("\n ---we are just before the call to join forward_thread--- \n"); 
   pthread_join(forward_thread, &thread_return_value);
  
    //sleep(10);
    //printf("\n ---we have joined forward thread--- \n");
   //wait before freeing for threa dot exit
    if(bits != NULL){
    free(fdata);
    free(ptr);
    }
           }


        //(int m=0; m<MAX_SIZE; m++){

	        //printf("%c", recv[m]);
        //}
         bit_count = 0;
	    if(bits != NULL){
            free(bits);
	    }
        }
        } else {
            // Ignore invalid transitions
            //printf("Invalid bit received (outside timing window) with delta: %u microseconds\n", delta);


        }

    }
		pthread_mutex_unlock(&mutex);    
}





// Listener function to detect Manchester-encoded messages (receiver)
void *listen(void* i) {

    struct thread_stuff *data=(struct thread_stuff*) i;
    

    set_mode(data->pi_thread, 23, PI_OUTPUT);  // GPIO 23 used for transmitting data

    // Set up the callback on GPIO 26 to trigger on both rising and falling edges
    callback(data->pi_thread, 26, EITHER_EDGE, gpio_state_change_callback);

    printf("System listening\n");
    
    // Keep the program running to process callbacks
    while (1) {
        time_sleep(1);  // Sleep to keep the listener active
	if(thread_term){
		pthread_exit(NULL);
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





int main(int argc, char *argv[]) {  
    int pi = pigpio_start(NULL, NULL);
    //make stricts for threads 
    struct thread_stuff *data= (struct thread_stuff*)calloc(1,sizeof(struct thread_stuff));
    struct send_stuff *sdata = (struct send_stuff*)calloc(1, sizeof(struct send_stuff));


    //p[i for send listen thread
    data->pi_thread=pi;

    pthread_mutex_init(&mutex, NULL);
    ssize_t addsize = 0;
    printf("\nEnter a single integer to represent your address. ");
    int input_address_size = scanf("%d", &address);
    //printf("%d", input_address_size);
    if(input_address_size != 1){
        printf("Invalid address length.\n");
        return 1;
        
    }
    int* our_address = calloc(8, sizeof(int));
    len_to_bits(our_address, address);


    //regularize(pi);  // Regularize GPIO pins before sending
    //time_sleep(2); //sleep so dont read regularize
   //declare threads
    pthread_t listener_thread;
    pthread_t sender_thread;
    // Start listener thread to listen for Manchester-encoded messages on GPIO 26
    if (pthread_create(&listener_thread, NULL, listen,(void *) data)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    clear_ts();
     in_address = calloc(8, sizeof(int));
    from_address = calloc(8, sizeof(int));

    while(1){

    char* app=NULL;
    ssize_t bufsize = 0;
    int in_size=0;
    printf("Select app by typing message or filetransfer and hitting enter type exit to terminate \n");
    in_size = getline(&app, &bufsize, stdin);
     if(strncmp(app, "exit", 4) == 0){
    free(app);
	thread_term=true;
    void *thread_return_value;
    pthread_join(listener_thread, &thread_return_value);
	break;
    }else if(strncmp(app, "filetransfer", 12) == 0){

    printf("Please select a number from below (type in and press enter)\n");
    printf("1 - Write a file.\n");
    printf("2 - Read from a file **file must already exist**.\n");
    printf("3 - Send a file.\n");
    printf("4 - Exit.\n");
    int choice;
    FILE* filePointer;
    printf("Enter choice: \n");
    scanf("%d", &choice);

size_t input_size;
uint8_t out_int;
    
    switch (choice) {
        case 1:
            printf("1 - Write a file has been selected.\n");
            file_buf = calloc(100, sizeof(char));
            filePointer = writeToFile(file_buf);
            free(file_buf);
	    break;

        case 2:
            printf("2 - Read from a file has been selected.\n");
            read_buf = calloc(100, sizeof(char));
            char* readed_file = readFromFile(read_buf);
           /* for (int i = 0; i < sizeof(readed_file)/sizeof(char); i++) {
                printf("%c", readed_file[i]);
            }*/
	        printf("\n");
            free(read_buf);
	    break;
            

        case 3:
            
            out_address = calloc(8, sizeof(int));
            printf("\nEnter the address you'd like to send to. \n");
            int output_address_size = scanf("%d", &out_int);
    
            if(output_address_size != 1){
                printf("Invalid address length.\n");
                free(out_address);
             break;      
            }

            printf("sending to addy: %d", out_int);
            len_to_bits(out_address, out_int);
            printf("\nFile path:\n");
            clear_ts();
            //get user input
            input_size = getline(&buf, &bufsize, stdin);  // Use getline for flexible inputreturn 0;}
            if (buf[input_size - 1] == '\n') {
                buf[input_size - 1] = '\0';
            }   
            if(strncmp(buf, "exit", 4) == 0){
                free(out_address);
	            break;
            }
            size_t binary_length;
            int *binary_data = file_to_binary_array(buf, &binary_length);
    //    for(int i=0; i<binary_length; i++){
    //     printf("%d", binary_data[i]);
    //    }
            int num_sends=(binary_length/160)+1;
            int* bin=calloc((input_size-1)*8, sizeof(int));
            strToBin(bin, (input_size-1)*8, (input_size-1), buf);
            int out_type=0;
            sdata->pi_send = pi;
            sdata->size = (input_size-1)*8 ;
            sdata->out_address = out_address;
            sdata->our_address = our_address;
            sdata->msg = bin;
            sdata->type=&out_type;
            if (pthread_create(&sender_thread, NULL, oursend,(void *) sdata)) {
                fprintf(stderr, "Error creating thread\n");
                return 1;
            }

            void *thread_return_value;
            for(int i=0; (i/160)<num_sends;i+=160){
                time_sleep(5);
                sdata->pi_send = pi;
                if(((i/160)+1)<num_sends){
                    sdata->size = 160;
                }else{
                    sdata->size = (binary_length%160);
                }

                sdata->out_address = out_address;
                sdata->our_address = our_address;


                sdata->msg = &binary_data[i];
	
                sdata->type=&out_type;

                if (pthread_create(&sender_thread, NULL, oursend,(void *) sdata)) {
                    fprintf(stderr, "Error creating thread\n");
	                return 1;
                }

                pthread_join(sender_thread, &thread_return_value);
            }
            time_sleep(5);
            int ender[]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1};

            sdata->pi_send = pi;
            sdata->size = 25;
            sdata->out_address = out_address;
            sdata->our_address = our_address;
            sdata->msg = ender;
            sdata->type=&out_type;
            if (pthread_create(&sender_thread, NULL, oursend,(void *) sdata)) {
                fprintf(stderr, "Error creating thread\n");
                return 1;
            }
            pthread_join(sender_thread, &thread_return_value);

            out_int = 0b00000000;
            free(binary_data);
            free(out_address);
            // free(buf);
            free(bin);
        case 4:
	        break;


        default:
            printf("Please correct your input.\n");



    }


    


    }else if(strncmp(app, "message", 7) == 0){

    while(1){
   
    out_address = calloc(8, sizeof(int));
    ssize_t bufsize = 0;
    ssize_t input_size;
    uint8_t out_int;

    ssize_t out_size = 0;
    //ask user for address
    printf("\nEnter the address you'd like to send to. \n");
    int output_address_size = scanf("%d", &out_int);
    if(output_address_size != 1){
        printf("Invalid address length.\n");
        free(out_address);
        break;      
    }

    printf("sending to addy: %d", out_int);
    len_to_bits(out_address, out_int);
    
    clear_ts(); 

    //ask user for input
    printf("\nEnter a string (500 charcter limit), type 'exit' to quit:\n");
    //get user input
    input_size = getline(&buf, &bufsize, stdin);  // Use getline for flexible inputreturn 0;}
    bin = calloc((input_size) * 8, sizeof(int));
   //if user input is exit end program
    void *thread_return_value;
    if(strncmp(buf, "exit", 4) == 0){
    free(out_address);
	    break;
    }
    //print string going to send
    //printf("string is: %s", buf);

    //fill bin with 0s?????
    for (int k = 0; k < input_size*8; k++) {
         bin[k]=0;
    }
   
    //convert string to binary
    strToBin(bin, (input_size-1)*8, (input_size-1), buf);
    
      //print binary before send
    // printf("\nBinary is: ");
    // for (int k = 0; k < (input_size-1)*8; k++) {
    //     printf("%d", bin[k]);
    // }
    
    int sObin = (input_size-1) * 8;
    int *bin_arr = bin;
    // Send Manchester-encoded header and message
    int out_type=1;
    
    //add_parity_bit(bin_arr, sObin);
    sdata->pi_send = pi;
    sdata->size = sObin;
    sdata->out_address = out_address;
    sdata->our_address = our_address;
    sdata->msg = bin;
    sdata->type=&out_type;
    
    if (pthread_create(&sender_thread, NULL, oursend,(void *) sdata)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
    //parody bit stuff saved for later
    // Stop listener thread after communication is done//int *bit_ptr = bits;
    //int sOmess = sizeof(bits) / sizeof(bits[0]);
    //check_parity_bit(bit_ptr, sOmess);
    
    //sleep so dont free before recove 
    //time_sleep(input_size/3);
     
    pthread_join(sender_thread, &thread_return_value);
    out_int = 0b00000000;
    free(bin);
    free(out_address);
    free(buf);
    
}
    }
    free(app);
    }
    void *thread_return_value;
    //free(*data->pi_thread);
    free(data);
    //free(bin);

    pthread_mutex_destroy(&mutex);

    free(our_address);
    free(in_address);
    free(from_address);
    free(out_add_char);
    //free(buf);
    pigpio_stop(pi);

    //pthread_join(sender_thread, &thread_return_value);
    //free(*sdata->pi_thread);
    //free(*sdata->size);
    //free(*sdata->msg);
    free(sdata);
    return 0;
    }

