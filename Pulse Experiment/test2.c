#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "pigpiod_if2.h"



int main(int argc, char *argv[]){
    //gpioInitialise();
    int start = pigpio_start(NULL, NULL);
    set_mode(start, 23, PI_OUTPUT);
    gpio_write(start, 23, 1);
    time_sleep(5);
    gpio_write(start, 23, 0);
    time_sleep(5);
    set_mode(start, 21, PI_OUTPUT);
    gpio_write(start, 21, 1);
    time_sleep(5);
    gpio_write(start, 21, 0);

    time_sleep(0.5);
    //test looping in 5 ten-thousandths of a second
    while (1) {
    gpio_write(start, 23, 1);
    time_sleep(0.0005);
    gpio_write(start, 23, 0);
    time_sleep(0.0005)
    }
    
}
