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
pwatch_parsedInfo parsedInfo;
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
        case PWATCH_ERR_FAILED_TO_PROCESS_STATUS:
            return "PWATCH_ERR_FAILED_TO_PROCESS_STATUS";
        default:
            return "PWATCH_ERR_UNKNOWN_ERROR";
    }
}

void pwatch_parseLine(const char* line)
{
    printf("%s\n",line);
}

/**
  * pwatch_parseStat
  *     Analyzing thread that should take the elements of the queue
 **/
int pwatch_parseStat(void)
{
    pwatch_queueLineBuf *aux;

    while (true) {
        if (readBuff == NULL)
            return PWATCH_ERR_FAILED_TO_READ;
        if (adminFlags & PWATCH_ADMIN_CLOSE) {
            adminFlags &= ~PWATCH_ADMIN_PARSE;
            adminFlags &= ~PWATCH_ADMIN_PARSE_CLOSED;
            break;
        }

        if (!(adminFlags & PWATCH_ADMIN_PARSE)) {
            continue;
        }

        // printf("Parse taking lock \n");
        pthread_mutex_lock(&lock);

        if (readBuff->next != NULL) {
            aux = readBuff;
            readBuff = (pwatch_queueLineBuf*) readBuff->next;
            pwatch_parseLine(aux->line);
            free(aux);
            readSemaphore->nmb--;
        } else if (readSemaphore->nmb <= 1) {
            readSemaphore->emptyFlg = true;
            printf("Parse waiting for empty flag \n");
            if(pthread_cond_wait(&readSemaphore->empty, &lock))
                return PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL;
            readSemaphore->emptyFlg = false;
        } else {
            return PWATCH_ERR_FAILED_TO_PROCESS_STATUS;
        }

        if (readSemaphore->fullFlg && readSemaphore->nmb <= PWATCH_LINE_LIMIT) {
            if (pthread_cond_broadcast(&readSemaphore->full))
                return PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL;
        }

        pthread_mutex_unlock(&lock);
        printf("parse releasing lock \n");
    }
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
    //printf("debug pwatch_readStat 1\n");
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
        //printf("debug pwatch_readStat 2\n");

        /* it comes here when you stop reading in case of timeout f.e.*/
        if(!(adminFlags & PWATCH_ADMIN_READ))
            continue;

        if (procStat == NULL)
            return PWATCH_ERR_FAILED_TO_OPEN;


        ret_line_size = getline(&lineStruct->line, &len, procStat);

        //printf("%d\n",(int)ret_line_size);
        if (feof(procStat)) {
            fclose(procStat);
            procStat = fopen(procStatPath, "r");
            continue;
        } else if (ret_line_size < 0)
            return PWATCH_ERR_FAILED_TO_READ;

        // printf("read -> taking lock \n");
        pthread_mutex_lock(&lock);

        lineStruct->next =
                    (pwatch_queueLineBuf*)malloc(sizeof(pwatch_queueLineBuf));
        if (lineStruct->next == NULL) {
            pthread_mutex_unlock(&lock);
            return PWATCH_ERR_FAILED_TO_ACQUIRE_MEMORY;
        }

        lineStruct = (pwatch_queueLineBuf*) lineStruct->next;
        lineStruct->line = NULL;
        lineStruct->next = NULL;
        readSemaphore->nmb++;

        if (readSemaphore->nmb > PWATCH_LINE_LIMIT) {
            readSemaphore->fullFlg = true;
            printf("read -> semaphore full \n");
            pthread_mutex_unlock(&lock);
            if (pthread_cond_wait(&readSemaphore->full, &lock))
                return PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL;
            readSemaphore->fullFlg = false;
        } else if (readSemaphore->emptyFlg) {
            // printf("read ->semaphore empty \n");
            if (pthread_cond_broadcast(&readSemaphore->empty))
                return PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL;
        }

        pthread_mutex_unlock(&lock);
        // printf("read -> release lock\n");
    }
}

static void *pwatch_readStatThreadWrap(void *argument)
{
    int ret = pwatch_readStat();
    if (ret)
        printf(" In read thread :%s %d \n", pwatch_errToString(ret), ret);
    return argument;
}

static void *pwatch_parseStatThreadWrap(void *argument)
{
    printf("parse debug\n");
    int ret = pwatch_parseStat();
    if (ret)
        printf(" In pwatch thread :%s %d \n", pwatch_errToString(ret), ret);
    return argument;
}

int main(int argc, char** argv)
{
    int ret;
    readBuff = malloc(sizeof(pwatch_queueLineBuf));
    readBuff->line = "NULL";
    readBuff->next = NULL;
    readSemaphore = malloc(sizeof(pwatch_semaphore));
    readSemaphore->nmb = 1;
    readSemaphore->fullFlg = false;
    readSemaphore->emptyFlg = true;
    adminFlags |= PWATCH_ADMIN_READ;
    adminFlags |= PWATCH_ADMIN_PARSE;
    ret = pthread_cond_init(&readSemaphore->empty, NULL);
    ret = pthread_cond_init(&readSemaphore->full, NULL);
    pthread_t readThread, parseThread;

    ret = pthread_create(&readThread, NULL, pwatch_readStatThreadWrap, NULL);
    if (ret) {
        printf("%s - %d \n", pwatch_errToString(ret), ret);
        return ret;
    }


    ret = pthread_create(&parseThread, NULL, pwatch_parseStatThreadWrap, NULL);
    if (ret) {
        printf("%s - %d \n", pwatch_errToString(ret), ret);
        return ret;
    }
    sleep(1);

    printf("DONE\n");
    if(argc == 2 && strcmp(argv[0], "debug\n"))
        printf("Initialization \n");

    return 0;
}
