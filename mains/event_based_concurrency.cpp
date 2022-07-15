//
// Created by zy on 22-6-25.
//
#include <stdio.h>
#include <signal.h>

void handle(int arg) {
    printf("stop wakin' me up...\n");
}

int main(int argc, char* argv[]) {
    signal(SIGHUP, handle);
    while (1)
        ;
    return 0;
}