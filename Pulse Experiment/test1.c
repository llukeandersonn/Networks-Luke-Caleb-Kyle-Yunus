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
    while (1){
        //one second intervals for pulsation
        gpio_write(start, 27, 1);
        time_sleep(1);
        gpio_write(start, 27, 0);
        time_sleep(1);
        gpio_write(start, 21, 1);
        time_sleep(1);
        gpio_write(start, 21, 0);
        time_sleep(1);
    }
    //gpioTerminate();
    }
