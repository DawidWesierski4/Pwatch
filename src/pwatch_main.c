#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pwatch.h"

/*
 *  pwatch_readStat
 *      skeleton of a function which purpose is to read /proc/stat file
 */
int pwatch_readStat(void)
{
    struct pwatch_cpuStat cpu1 = {
        .user =    10,
        .nice =    11,
        .system =  12,
        .idle =    13,
        .iowait =  14,
        .irq =     15,
        .softirq = 16
    };

    printf("user == %i \n", cpu1.user);

    return 0;
}

int main(int argc, char** argv)
{
    if(argc == 2 && strcmp(argv[0], "debug"))
        printf("Initialization \n");

    pwatch_readStat();
    return 0;
}
