#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
//#include "pigpiod_if2.h"

#define MAX_SIZE 5

int *bits = NULL;
int *inclen = NULL;
int binlength[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int inlen[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Function to convert message length to bits
void len_to_bits(int *binlength, int bit_length) {
    int i = 0;
    while(bit_length > 0) {
        binlength[7-i] = bit_length % 2;
        bit_length = bit_length / 2;
        i++;
    }
    //printf("%d, ", bit_length);
    for(int j = 0; j < 8; j++){
        printf("%d", binlength[j]);

    }

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

int main(int argc, char *argv[]){
    int length = 255;
    len_to_bits(binlength, length);
    printf("\n");
    uint8_t num = bits_to_len(binlength);
    printf("%d", num);
    return 0;
}
