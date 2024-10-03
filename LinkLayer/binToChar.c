#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

//#include "pigpiod_if2.h"
#define MAX_SIZE 5

void binToStr(int *bin_arr, int bin_size, char *char_arr, int char_size) {
    for (int i = 0; i < char_size; i++) {
        char car = 0b00000000;
        for (int j = 7; j >= 0; j--) {
            car |= (bin_arr[8*i+j] << 7-j); 
        }
        char_arr[i] = car;
        for (int k = 7; k >= 0; k--) {
        printf("%d", (char_arr[i] >> k) & 1);
    }
    puts("");
    }
}
int main(int argc, char *argv[]){

    char c = 0b00000000;
    printf("%c", c);
    //01101000 01101001
    int bin[MAX_SIZE*8] = {0,1,1,0,1,0,0,0,0,1,1,0,0,1,0,1,0,1,1,0,1,1,0,0,0,1,1,0,1,1,0,0,0,1,1,0,1,1,1,1};
    char buf[MAX_SIZE] = {};
    binToStr(bin, MAX_SIZE*8, buf, MAX_SIZE);
    printf("\n%c", buf[0]);
    printf("%c", buf[1]);
    printf("%c", buf[2]);
    printf("%c", buf[3]);
    printf("%c", buf[4]);
    return 0;
}
