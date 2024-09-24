#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "pigpiod_if2.h"



int main(int argc, char *argv[]){
    //use transmitter to toggle receiver
    int start = pigpio_start(NULL, NULL);
    set_mode(start, 23, PI_OUTPUT);
    set_mode(start, 26, PI_INPUT);
    gpio_write(start, 23, 1);
    time_sleep(5);
    gpio_write(start, 23, 0);
    time_sleep(5);

}
