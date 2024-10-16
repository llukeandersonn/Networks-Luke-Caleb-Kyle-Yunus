#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

//#include "pigpiod_if2.h"
#define MAX_SIZE 20

/*
void charToBinary(int *bin_arr, int bin_size, char *char_arr, int char_size) {
    int c;
    printf("bin size is: %d\n", bin_size);
    printf("char size is: %d\n", char_size);
    for (int j = 0; j < char_size; j++) {
        c = char_arr[j];
        for (int i = 7; i >= 0; i--) {
            bin_arr[i] = (c >> i) & 1;
            printf("char is: %0b\n", c);
        }
    }
}
*/
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

int main(int argc, char *argv[]){
    //char c[] = {"hello"};
    char buf[MAX_SIZE] = {};
    int bin[MAX_SIZE*8];
    int i;
    printf("Enter a string...\n");
    fgets(buf, MAX_SIZE, stdin);
    printf("string is: %s", buf);
    // for (i = 0; i < MAX_SIZE; i++) {
    //     buf[i] = "\0";
    // }
    for (i = 0; i < MAX_SIZE*8; i++) {
        bin[i] = 0;
    }
    printf("Binary is: ");
    strToBin(bin, MAX_SIZE*8, MAX_SIZE, &buf);
    for (int k = 0; k < MAX_SIZE*8; k++) {
        printf("%d", bin[k]);
    }
    return 0;
}
