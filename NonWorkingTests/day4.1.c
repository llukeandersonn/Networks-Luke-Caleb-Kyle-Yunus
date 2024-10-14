#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

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

int main(int argc, char *argv[])
{   
    pthread_t *p1, *p2, *p3;
    p1 = start_thread(listen, "thread 1");
    int head[] = {1, 1, 1, 1};
    int biit[] = {1, 1, 1, 0, 1, 0, 0, 1, 1 , 1, 0, 0, 0, 1, 0, 1};
    int sOhead = sizeof(head) / sizeof(head[0]);
    int sObits = sizeof(biit) / sizeof(biit[0]);
    regularize();
    sendFunction(head, sOhead);
    sendFunction(biit, sObits);
    time_sleep(1);
    stop_thread(p1);
    /*
    // use transmitter to toggle receiver
    int pi = pigpio_start(NULL, NULL);

    int i = 0;

    while (i < 54)
    {
        int up = set_pull_up_down(pi, i, PI_PUD_UP);
        int down = set_pull_up_down(pi, i, PI_PUD_DOWN);
        i++;
    }

    i = 0;

    clock_t start, end;
    double time_used;
    set_mode(pi, 23, PI_OUTPUT);
    set_mode(pi, 26, PI_INPUT);

    start = clock();
    gpio_write(pi, 23, 1);
    gpio_write(pi, 22, 1);
    gpio_write(pi, 24, 1);
    /*
    int read = gpio_read(pi, 26);
    gpio_write(pi, 23, 0);
    int read2 = gpio_read(pi, 26);
    gpio_write(pi, 23, 1);
    int read3 = gpio_read(pi, 26);
    gpio_write(pi, 23, 0);
    int read4 = gpio_read(pi, 26);
    gpio_write(pi, 23, 1);
    int read5 = gpio_read(pi, 26);
    gpio_write(pi, 23, 0);
    int read6 = gpio_read(pi, 26);
    end = clock();
    time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken to write is %f.\n", time_used);
    start = clock();

    end = clock();
    time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken to read is %f.\n", time_used);
    //see 1 with transmitter on, receiver off
    printf("Signal of GPIO 26 is %d.\n", read);
    printf("Signal of GPIO 26 is %d.\n", read2);
    printf("Signal of GPIO 26 is %d.\n", read3);
    printf("Signal of GPIO 26 is %d.\n", read4);
    printf("Signal of GPIO 26 is %d.\n", read5);
    printf("Signal of GPIO 26 is %d.\n", read6);
    gpio_write(pi, 23, 0);
    time_sleep(0.005);
    */
    /*
     int read1 = gpio_read(pi, 26);
     // see 0 with transmitter off, receiver on
     printf("Signal of GPIO 26 is %d.\n", read1);
     pigpio_stop(pi);*/
}