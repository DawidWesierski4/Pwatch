#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include "pwatch.h"

uint32_t adminFlags;
pthread_mutex_t lock;

pwatch_queueLineBuf* readBuff;

pwatch_semaphore *readSemaphore;

pwatch_cpuStatList cpuStats = {
    .number = 0,
    .node = NULL,
    .next = NULL,
    .back = NULL
};

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
        case PWATCH_ERR_FAILED_TO_ACQUIRE_MEMORY:
            return "PWATCH_ERR_FAILED_TO_ACQUIRE_MEMORY";
        case PWATCH_ERR_FAILED_TO_PROCESS_ADMIN_FLAG:
            return "PWATCH_ERR_FAILED_TO_PROCESS_ADMIN_FLAG";
        case PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL:
            return "PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL";
        default:
            return "PWATCH_ERR_UNKNOWN_ERROR";
    }
}

/**
  * pwatch_parseStat
  *     parse / fill the pwatch_cpuStat
 **/
int pwatch_parseStat(const char* line, int len)
{
    printf("line %s length %d \n", line, len);
    // pwatch_queueLineBuf aux;
    // while (!pwatch_semaphore.empty) {

    //     printf("line %s length %d \n", line, len);
    // }
    return PWATCH_SUCCESS;
}

/**
  * releaseQueueLineBuf
  *     release the reading queue
 **/
void pwatch_releaseQueueLineBuf(void)
{
    pwatch_queueLineBuf *aux;
    while (readBuff != NULL) {
        aux = readBuff;
        readBuff = readBuff->next;
        free(aux);
    }
}

/**
  * pwatch_readStat
  *     skeleton of a function which purpose is to read /proc/stat file
 **/
int pwatch_readStat(void)
{
    pwatch_queueLineBuf *lineStruct = readBuff;
    FILE *procStat = fopen(procStatPath, "r");
    size_t len = 0;
    ssize_t ret_line_size; // we set it to to went into while reading

    while (true) {
        if(adminFlags & PWATCH_ADMIN_CLOSE) {
            fclose(procStat);
            pwatch_releaseQueueLineBuf();
            adminFlags &= ~PWATCH_ADMIN_READ;
            adminFlags &= ~PWATCH_ADMIN_READ_CLOSED;
            return PWATCH_SUCCESS;
        }

        /* it comes here when you stop reading in case of timeout f.e.*/
        if(!(adminFlags & PWATCH_ADMIN_READ))
            continue;

        if (procStat == NULL)
            return PWATCH_ERR_FAILED_TO_OPEN;

        // ret_line_size = getline(&line, &len, procStat);

        // while (ret_line_size < 0)
        // free(line);
        // line = NULL;
        // len = 0;
        // ret_line_size = getline(&line, &len, procStat);
        // fclose(procStat);

        ret_line_size = getline(&lineStruct->line, &len, procStat);
        if (feof(procStat)) {
            fclose(procStat);
            procStat = fopen(procStatPath, "r");
            continue;
        } else if (ret_line_size < 0)
            return PWATCH_ERR_FAILED_TO_READ;

        lineStruct->next = (pwatch_queueLineBuf*)malloc(sizeof(pwatch_queueLineBuf));
        if (lineStruct->next == NULL)
            return PWATCH_ERR_FAILED_TO_ACQUIRE_MEMORY;

        lineStruct = (pwatch_queueLineBuf*) lineStruct->next;
        lineStruct->line = NULL;
        lineStruct->next = NULL;

        if (ret_line_size <= 0)
            return PWATCH_ERR_FAILED_TO_READ;

        pthread_mutex_lock(&lock);
        readSemaphore->nmb++;
        if (readSemaphore->nmb >= PWATCH_LINE_LIMIT) {
            readSemaphore->fullFlg = true;
            if (!pthread_cond_wait(&readSemaphore->full, &lock))
                return PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL;
        } else if (readSemaphore->nmb == 1){
            readSemaphore->emptyFlg = false;
            pthread_cond_signal(&readSemaphore->empty);
        }
        pthread_mutex_unlock(&lock);
    }
}

int main(int argc, char** argv)
{
    int ret;
    readBuff = malloc(sizeof(pwatch_queueLineBuf));
    readBuff->line = NULL;
    readBuff->next = NULL;
    readSemaphore = malloc(sizeof(pwatch_semaphore));
    readSemaphore->nmb = 0;
    readSemaphore->fullFlg = false;
    readSemaphore->emptyFlg = true;

    adminFlags = 0;


    if(argc == 2 && strcmp(argv[0], "debug"))
        printf("Initialization \n");

    adminFlags |= PWATCH_ADMIN_READ;
    adminFlags |= PWATCH_ADMIN_PROCESS;
    ret = pthread_cond_init(&readSemaphore->empty, NULL);
    ret = pthread_cond_init(&readSemaphore->full, NULL);

    if ((ret = pwatch_readStat())) {
        fprintf(stderr, "%s \n", pwatch_errToString(ret));
        return ret;
    }

    return 0;
}
