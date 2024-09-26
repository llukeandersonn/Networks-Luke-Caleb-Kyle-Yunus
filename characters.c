#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "pigpiod_if2.h"

int main(int argc, char *argv[]){
    char c = 'k';
    int numbers[8];
    for (int i = 7; i >= 0; i--) {
        int bit = (c >> i) & 1; // Extract the i-th bit using bitwise operations
        printf("%d", bit);
        numbers[i] = bit;
    }
    //use transmitter to toggle receiver
    int pi = pigpio_start(NULL, NULL);

    set_mode(pi, 23, PI_OUTPUT);
    set_mode(pi, 26, PI_INPUT);
    

    int reading[8];
    int size = sizeof(numbers) / sizeof(numbers[0]); // Calculate the size of the array
    for (int i = 0; i < size; i++) {
        gpio_write(pi, 23, numbers[i]);
        int read = gpio_read(pi, 26);
        reading[i] = read;
    }

    printf("\n");

    for (int i = 0; i < size; i++) {
        printf("%d", reading[i]);
    }

    pigpio_stop(pi);

}