#ifndef PWATCH_H
#define PWATCH_H

typedef struct pwatch_cpuStat{
    int user;       /* normal processes executing in user mode */
    int nice;       /* niced processes executing in user mode */
    int system;     /* processes executing in kernel mode */
    int idle;       /* twiddling thumbs */
    int iowait;     /* waiting for I/O to complete */
    int irq;        /* servicing interrupts */
    int softirq;     /* servicing softirqs */
} pwatch_cpuStat;

int pwatch_readStat(void);


#endif /* PWATCH_H header guard */
