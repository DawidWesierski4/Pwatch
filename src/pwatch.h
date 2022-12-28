#ifndef PWATCH_H
#define PWATCH_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define PWATCH_LINE_LIMIT 26
#define BIT(n) (0x1U << (n))

enum pwatch_errNum {
    PWATCH_SUCCESS,
    PWATCH_ERR_FAILED_TO_OPEN,
    PWATCH_ERR_FAILED_TO_READ,
    PWATCH_ERR_FAILED_TO_ACQUIRE_MEMORY,
    PWATCH_ERR_FAILED_TO_PROCESS_ADMIN_FLAG,
    PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL
};

enum pwatch_adminFlagsNum {
    PWATCH_ADMIN_READ           = BIT(0),
    PWATCH_ADMIN_PROCESS        = BIT(1),
    PWATCH_ADMIN_READ_CLOSED    = BIT(30),
    PWATCH_ADMIN_CLOSE          = BIT(31)
};

typedef struct pwatch_queueLineBuf
{
    char* line;
    void* next;
} pwatch_queueLineBuf;

typedef struct pwatch_cpuStat {
    int user;       /* normal processes executing in user mode */
    int nice;       /* niced processes executing in user mode */
    int system;     /* processes executing in kernel mode */
    int idle;       /* twiddling thumbs */
    int iowait;     /* waiting for I/O to complete */
    int irq;        /* servicing interrupts */
    int softirq;    /* servicing softirqs */
} pwatch_cpuStat;

typedef struct pwatch_cpuStatList {
    int number;
    void* node;
    void* next;
    void* back;
} pwatch_cpuStatList;

typedef struct pwatch_semaphore {
    int nmb;
    bool fullFlg;
    pthread_cond_t full;
    bool emptyFlg;
    pthread_cond_t empty;
} pwatch_semaphore;

extern pwatch_semaphore *readSemaphore;

extern pwatch_queueLineBuf *readBuff;

extern uint32_t adminFlags;

extern uint32_t cpuStatList;

extern pthread_mutex_t lock;

const char *procStatPath = "/proc/stat";

int pwatch_parseStat (const char* line, int len);

int pwatch_readStat (void);

const char* pwatch_errToString (int errEnum);

#endif /* PWATCH_H header guard */
