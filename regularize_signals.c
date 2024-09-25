#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "pigpiod_if2.h"



int main(int argc, char *argv[]){
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
    printf("Signals regularized!\n");
    pigpio_stop(pi);

}
