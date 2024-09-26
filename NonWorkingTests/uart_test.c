#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include "pigpiod_if2.h"
void *listen(void *arg){
	int pi = pigpio_start(NULL, NULL);
	set_mode(pi, 26, PI_INPUT);
	int last_read;
	while(1){	
    	int jread = gpio_read(pi, 26);
    	last_read=jread;
    	time_sleep(0.05);
    	jread = gpio_read(pi, 26);
    	//if a high signal is transmitted
    	if (jread != last_read ){
	    	last_read=jread;
			time_sleep(0.05);
        	jread = gpio_read(pi, 26);
        	//look for next bit
        		if (jread != last_read){
    				last_read=jread;
					time_sleep(0.05);
        			jread = gpio_read(pi, 26);
        			//look for next bit
        				if (jread != last_read){
    						last_read=jread;
							time_sleep(0.05);
        					jread = gpio_read(pi, 26);
        					//find last bit in header
        					if (jread != last_read){
    							last_read=jread;
								time_sleep(0.05);
           		 				//NRZ encoding
	    						while(1){
        							jread = gpio_read(pi, 26);
	    							if(last_read==jread){
										printf("0");
										last_read=jread;
	    							}else{
	       								printf("1");
										last_read=jread;
        							}
    								printf("\n");
									time_sleep(0.05);
						}
					}
    			}
    		}
    	}
    }
}

void regularize() {
    //toggle all receivers and transmitters
    int pi = pigpio_start(NULL, NULL);
    //port 3
    set_mode(pi, 23, PI_OUTPUT);
    set_mode(pi, 22, PI_OUTPUT);
    //port 1
    set_mode(pi, 27, PI_OUTPUT);
    set_mode(pi, 26, PI_OUTPUT);
    //port 2
    set_mode(pi, 25, PI_OUTPUT);
    set_mode(pi, 24, PI_OUTPUT);
    //port 4
    set_mode(pi, 21, PI_OUTPUT);
    set_mode(pi, 20, PI_OUTPUT);
    //set high
    gpio_write(pi, 23, 1);
    gpio_write(pi, 22, 1);
    gpio_write(pi, 27, 1);
    gpio_write(pi, 26, 1);
    gpio_write(pi, 25, 1);
    gpio_write(pi, 24, 1);
    gpio_write(pi, 21, 1);
    gpio_write(pi, 20, 1);
    //set low
    gpio_write(pi, 23, 0);
    gpio_write(pi, 22, 0);
    gpio_write(pi, 27, 0);
    gpio_write(pi, 26, 0);
    gpio_write(pi, 25, 0);
    gpio_write(pi, 24, 0);
    gpio_write(pi, 21, 0);
    gpio_write(pi, 20, 0);
}

void sendFunction(int bits[], int sObits)
{
    int pi = pigpio_start(NULL, NULL);
    set_mode(pi, 23, PI_OUTPUT);   


    for (int i = 0; i < sObits; i++)
    {
        if (bits[i] == 1)
        {
    
            int state = gpio_read(pi, 23);
            if (state == 1)
            {
                gpio_write(pi, 23, 0);
                time_sleep(0.05);
            }
            else
            {
                gpio_write(pi, 23, 1);
                time_sleep(0.05);
            }
        }
        	else if (bits[i] == 0)
        {
            time_sleep(0.05);
        }
    }
}



//-------------------------
	//----- SETUP USART 0 -----
	//-------------------------
	//At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
	int main(int argc, char *argv[]){
	int pi = pigpio_start(NULL, NULL);
	pthread_t *p1, *p2, *p3;
    //p1 = start_thread(listen, "thread 1");
	set_mode(pi, 14, PI_ALT0);
	set_mode(pi, 15, PI_ALT0);
    int head[] = {1, 1, 1, 1};
    int biit[] = {1, 1, 1, 0, 1, 0, 0, 1, 1 , 1, 0, 0, 0, 1, 0, 1};
    int sOhead = sizeof(head) / sizeof(head[0]);
    int sObits = sizeof(biit) / sizeof(biit[0]);
    regularize();
    /* sendFunction(head, sOhead);
    sendFunction(biit, sObits);
    time_sleep(1);
    stop_thread(p1); */
	
	//OPEN THE UART
	//The flags (defined in fcntl.h):
	//	Access modeuart0_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY);	s (use 1 of these):
	//		O_RDONLY - Open for reading only.
	//		O_RDWR - Open for reading and writing.
	//		O_WRONLY - Open for writing only.
	//
	//	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//											immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.

	
	//CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)
/* 	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options); */




    	//----- TX BYTES -----
/* 	unsigned char tx_buffer[20];
	gpioPulse_t *p_tx_buffer;
	
	p_tx_buffer = &tx_buffer[0];
	*p_tx_buffer++ = 'H';
	*p_tx_buffer++ = 'e';
	*p_tx_buffer++ = 'l';
	*p_tx_buffer++ = 'l';
	*p_tx_buffer++ = 'o'; */
	char str[5];
	str[0] = "H";
	str[1] = "e";
	str[2] = "l";
	str[3] = "l";
	str[4] = "o";
	
		//int count = write(uart0_filestream, &tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));		//Filestream, bytes to write, number of bytes to write
	wave_add_serial(pi, 14, 150, 40, 4, 0, 5, str);
	int wav = wave_create(pi);
	wave_send_once(pi, wav);
		/* for (int i = 0; i < sizeof(p_tx_buffer); i++) {
		//convert string from buffer to bytes
    		int numbers[8];
    		for (int j = 7; j >= 0; j--) {
        		int bit = (p_tx_buffer[i] >> j) & 1; // Extract the i-th bit using bitwise operations
        		printf("%d", bit);
        		numbers[j] = bit;
    }


			int size = sizeof(numbers) / sizeof(numbers[0]); // Calculate the size of the array
    		for (int k = 0; k < size; k++) {
        		gpio_write(pi, 14, numbers[k]);
    }
		} */



//----- CHECK FOR ANY RX BYTES -----
	
		// Read up to 255 characters from the port if they are there
		unsigned char rx_buffer[256];
		//int rx_length = read(uart0_filestream, (void*)rx_buffer, 255);		//Filestream, buffer to store in, number of bytes to read (max)
		bb_serial_read(pi, 15, rx_buffer, 5);
		int rx_length = sizeof(rx_buffer);
		
		
		
		if (rx_length < 0)
		{
			//An error occured (will occur if there are no bytes)
		}
		else if (rx_length == 0)
		{
			//No data waiting
		}
		else
		{
			//Bytes received
			rx_buffer[rx_length] = '\0';
			printf("%i bytes read : %s\n", rx_length, rx_buffer);
		}
	}