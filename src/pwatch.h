#ifndef PWATCH_H
#define PWATCH_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define PWATCH_USEC_TIMEOUT 10000
#define PWATCH_LINE_LIMIT 30
#define BIT(n) (0x1U << (n))
#define B

enum pwatch_errEnum {
	PWATCH_ERR_NULL_TERMINATOR = 1,
	PWATCH_ERR_FAILED_TO_OPEN,
	PWATCH_ERR_FAILED_TO_READ,
	PWATCH_ERR_FAILED_TO_ACQUIRE_MEMORY,
	PWATCH_ERR_FAILED_TO_PROCESS_ADMIN_FLAG,
	PWATCH_ERR_FAILED_TO_PARSE,
	PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL,
	PWATCH_ERR_FAILED_TO_PROCESS_STATUS,
	PWATCH_ERR_READ_TIMEOUT,
	PWATCH_ERR_NUMBER
};

enum pwatch_enum {
	PWATCH_SUCCESS,
	PWATCH_EXIT = PWATCH_ERR_NUMBER + 1 /* to avoid conflict with errEnums */
};

enum pwatch_adminFlagsNum {
	PWATCH_ADMIN_INIT		= BIT(0),
	PWATCH_ADMIN_READ		= BIT(1),
	PWATCH_ADMIN_PARSE		= BIT(2),
	PWATCH_ADMIN_PARSE_CLOSE	= BIT(29),
	PWATCH_ADMIN_READ_CLOSE		= BIT(30),
	PWATCH_ADMIN_CLOSE		= BIT(31)
};

typedef struct pwatch_queueLineBuf
{
	char* line;
	void* next;
} pwatch_queueLineBuf;

typedef struct pwatch_cpuStat {
	uint32_t user;		/* normal processes executing in user mode */
	uint32_t nice;		/* niced processes executing in user mode */
	uint32_t system;	/* processes executing in kernel mode */
	uint32_t idle;		/* twiddling thumbs */
	uint32_t iowait;	/* waiting for I/O to complete */
	uint32_t irq;		/* servicing interrupts */
	uint32_t softirq;	/* servicing softirqs */
} pwatch_cpuStat;

typedef struct pwatch_cpuStatList {
	int number;
	void* node;
	void* next;
	void* back;
} pwatch_cpuStatList;

typedef struct pwatch_parsedInfo {
	pwatch_cpuStatList *cpuStatList;
	uint32_t ctxt;
	uint32_t btime;
	uint32_t processes;
	uint32_t procsRunning;
	uint32_t procsBlocked;
} pwatch_parsedInfo;

typedef struct pwatch_semaphore {
	uint32_t nmb;
	bool fullFlg;
	pthread_cond_t full;
	bool emptyFlg;
	pthread_cond_t empty;
} pwatch_semaphore;

typedef struct pwatch_timeouts {
	uint32_t read;
	uint32_t parse;
} pwatch_timeouts;

const char *procStatPath = "/proc/stat";

extern pwatch_parsedInfo *parsedInfo;
extern pwatch_semaphore *readSemaphore;
extern pwatch_queueLineBuf *readBuff;

extern uint32_t adminFlags;

extern pthread_mutex_t lock;

int pwatch_initialize(void);

void pwatch_parseInfoPtr(unsigned int **Ptr);

int pwatch_watchDog(void);

int pwatch_parseStat (void);

int pwatch_readStat (void);

const char* pwatch_errToString (int errEnum);

#endif /* PWATCH_H header guard */
