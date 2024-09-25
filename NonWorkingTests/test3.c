#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "pigpiod_if2.h"

void sendFunction(int bits[])
{
    size_t sObits = sizeof(bits) / sizeof(bits[0]);
    for (size_t i = 0; i < sObits; i++)
    {
        if (bits[i] = 1)
        {
            int pi = pigpio_start(NULL, NULL);
            int state = gpio_read(pi, 23);
            if (state = 1)
            {
                gpio_write(pi, 23, 0);
                sleep(0.5);
            }
            else
            {
                gpio_write(pi, 23, 1);
                sleep(0.5);
            }
        }
        else if (bits[i] = 0)
        {
            sleep(0.5);
        }
    }
}

int main(int argc, char *argv[])
{
    int biit[] = {1, 1, 1, 0, 1, 0, 0, 1};
    sendFunction(biit);
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