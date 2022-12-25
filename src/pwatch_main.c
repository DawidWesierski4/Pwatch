#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "pwatch.h"

uint32_t pwatch_adminFlags = 0;

/**
  * pwatch_errToString
  * @errNum -> pwatch_errNum value
  *     takes the enum and returns the enum name as string
 **/
const char* pwatch_errToString (int errEnum)
{
    switch (errEnum) {
        case PWATCH_ERR_FAILED_TO_OPEN:
            return "PWATCH_ERR_FAILED_TO_OPEN";
        case PWATCH_ERR_FAILED_TO_READ:
            return "PWATCH_ERR_FAILED_TO_READ";
        case PWATCH_ERR_WRONG_ADMIN_FLAG:
            return "PWATCH_ERR_WRONG_ADMIN_FLAG";
        default:
            return "PWATCH_ERR_UNKNOWN_ERROR";
    }
}

/**
  * pwatch_readStat
  *     skeleton of a function which purpose is to read /proc/stat file
 **/
int pwatch_readStat(void)
{
    FILE *procStat;
    pwatch_cpuStat* cpus = malloc(sizeof(pwatch_cpuStat));
    procStat = fopen(pwatch_procStatPath, "r");
    char *line = malloc(sizeof(char) * PWATCH_LINE_LIMIT);
    size_t len = 0;
    ssize_t ret;

    cpus->idle = 0; //TODO interface for the read function

    if (procStat == NULL)
        return PWATCH_ERR_FAILED_TO_OPEN;

    if (pwatch_adminFlags & PWATCH_ADMIN_READ) {
        ret = getline(&line, &len, procStat);
    else
        return PWATCH_ERR_WRONG_ADMIN_FLAG;

        if (fseek(procStat, ret, SEEK_SET) != 0) {
            fclose(procStat);
            return PWATCH_ERR_FAILED_TO_READ;
        }

        pwatch_parseMail(red, );

        
    } else {

    }

    fclose(procStat);
    return PWATCH_OPERATION_SUCCESSFUL;
}

int main(int argc, char** argv)
{
    int ret;

    if(argc == 2 && strcmp(argv[0], "debug"))
        printf("Initialization \n");

    pwatch_adminFlags |= PWATCH_ADMIN_READ;

    ret = pwatch_readStat();
    if (!ret) {
        fprintf(stderr, pwatch_errToString(ret), "\n");
        return ret;
    }

    return 0;
}
