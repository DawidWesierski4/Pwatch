#ifndef PWATCH_H
#define PWATCH_H

#define PWATCH_LINE_LIMIT 256

enum pwatch_errNum {
    PWATCH_OPERATION_SUCCESSFUL,
    PWATCH_ERR_FAILED_TO_OPEN,
    PWATCH_ERR_FAILED_TO_READ
};

const char *pwatch_procStatPath = "/proc/stat";

typedef struct pwatch_cpuStat {
    int user;       /* normal processes executing in user mode */
    int nice;       /* niced processes executing in user mode */
    int system;     /* processes executing in kernel mode */
    int idle;       /* twiddling thumbs */
    int iowait;     /* waiting for I/O to complete */
    int irq;        /* servicing interrupts */
    int softirq;    /* servicing softirqs */
} pwatch_cpuStat;

int pwatch_readStat(void);

const char* pwatch_errToString (int errEnum);

#endif /* PWATCH_H header guard */
