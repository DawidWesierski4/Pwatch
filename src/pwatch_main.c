#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pwatch.h"

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

    if (procStat == NULL)
        return PWATCH_ERR_FAILED_TO_OPEN;

    while (scanf("%d",&cpus->idle)==1) {
        ret = getline(&line, &len, procStat);
        printf("%s -- \n", line);

        if (fseek(procStat, ret, SEEK_SET) != 0) {
            fclose(procStat);
            return PWATCH_ERR_FAILED_TO_READ;
        }
    }

    fclose(procStat);
    return PWATCH_OPERATION_SUCCESSFUL;
}

int main(int argc, char** argv)
{
    int ret;

    if(argc == 2 && strcmp(argv[0], "debug"))
        printf("Initialization \n");

    ret = pwatch_readStat();
    if (!ret) {
        fprintf(stderr, pwatch_errToString(ret), "\n");
        return ret;
    }

    return 0;
}
