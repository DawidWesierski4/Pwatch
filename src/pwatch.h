#ifndef PWATCH_H
#define PWATCH_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define PWATCH_LINE_LIMIT 30
#define BIT(n) (0x1U << (n))

enum pwatch_errNum {
    PWATCH_SUCCESS,
    PWATCH_ERR_NULL_TERMINATOR,
    PWATCH_ERR_FAILED_TO_OPEN,
    PWATCH_ERR_FAILED_TO_READ,
    PWATCH_ERR_FAILED_TO_ACQUIRE_MEMORY,
    PWATCH_ERR_FAILED_TO_PROCESS_ADMIN_FLAG,
    PWATCH_ERR_FAILED_TO_PARSE,
    PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL,
    PWATCH_ERR_FAILED_TO_PROCESS_STATUS
};

enum pwatch_adminFlagsNum {
    PWATCH_ADMIN_INIT           = BIT(0),
    PWATCH_ADMIN_READ           = BIT(1),
    PWATCH_ADMIN_PARSE          = BIT(2),
    PWATCH_ADMIN_PARSE_CLOSED   = BIT(29),
    PWATCH_ADMIN_READ_CLOSED    = BIT(30),
    PWATCH_ADMIN_CLOSE          = BIT(31)
};

typedef struct pwatch_queueLineBuf
{
    char* line;
    void* next;
} pwatch_queueLineBuf;

typedef struct pwatch_cpuStat {
    unsigned int user;       /* normal processes executing in user mode */
    unsigned int nice;       /* niced processes executing in user mode */
    unsigned int system;     /* processes executing in kernel mode */
    unsigned int idle;       /* twiddling thumbs */
    unsigned int iowait;     /* waiting for I/O to complete */
    unsigned int irq;        /* servicing interrupts */
    unsigned int softirq;    /* servicing softirqs */
} pwatch_cpuStat;

typedef struct pwatch_cpuStatList {
    int number;
    void* node;
    void* next;
    void* back;
} pwatch_cpuStatList;

typedef struct pwatch_parsedInfo {
    pwatch_cpuStatList *cpuStatList;
    unsigned int ctxt;
    unsigned int btime;
    unsigned int processes;
    unsigned int procsRunning;
    unsigned int procsBlocked;
} pwatch_parsedInfo;

typedef struct pwatch_semaphore {
    int nmb;
    bool fullFlg;
    pthread_cond_t full;
    bool emptyFlg;
    pthread_cond_t empty;
} pwatch_semaphore;

extern pwatch_parsedInfo *parsedInfo;

extern pwatch_semaphore *readSemaphore;

extern pwatch_queueLineBuf *readBuff;

extern uint32_t adminFlags;

extern pthread_mutex_t lock;

void pwatch_parseInfoPtr(unsigned int **Ptr);

const char *procStatPath = "/proc/stat";

int pwatch_parseStat (void);

int pwatch_readStat (void);

const char* pwatch_errToString (int errEnum);

#endif /* PWATCH_H header guard */
