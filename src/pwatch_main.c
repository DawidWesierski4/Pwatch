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
pwatch_parsedInfo *parsedInfo;
pwatch_semaphore *readSemaphore;

pwatch_cpuStatList cpuStats = {
	.number = -1,
	.node = NULL,
	.next = NULL,
	.back = NULL
};

pwatch_timeouts timeouts;

/**
  * pwatch_errToString
  * @errNum -> pwatch_errNum value
  *	 takes the enum and returns the enum name as string
 **/
const char* pwatch_errToString (int errEnum)
{
	switch (errEnum) {
		case PWATCH_ERR_NULL_TERMINATOR:
			return "PWATCH_ERR_NULL_TERMINATOR";
		case PWATCH_ERR_FAILED_TO_OPEN:
			return "PWATCH_ERR_FAILED_TO_OPEN";
		case PWATCH_ERR_FAILED_TO_READ:
			return "PWATCH_ERR_FAILED_TO_READ";
		case PWATCH_ERR_FAILED_TO_ACQUIRE_MEMORY:
			return "PWATCH_ERR_FAILED_TO_ACQUIRE_MEMORY";
		case PWATCH_ERR_FAILED_TO_PROCESS_ADMIN_FLAG:
			return "PWATCH_ERR_FAILED_TO_PROCESS_ADMIN_FLAG";
		case PWATCH_ERR_FAILED_TO_PARSE:
			return "PWATCH_ERR_FAILED_TO_PARSE";
		case PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL:
			return "PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL";
		case PWATCH_ERR_FAILED_TO_PROCESS_STATUS:
			return "PWATCH_ERR_FAILED_TO_PROCESS_STATUS";
		case PWATCH_ERR_READ_TIMEOUT:
			return "PWATCH_ERR_READ_TIMEOUT";
		default:
			return "PWATCH_ERR_UNKNOWN_ERROR";
	}
}

int pwatch_moveParsedInfo(int cpuNmb)
{

	if(!parsedInfo || !parsedInfo->cpuStatList )
		return PWATCH_ERR_NULL_TERMINATOR;

	while (parsedInfo->cpuStatList->number != cpuNmb) {
		if (cpuNmb < parsedInfo->cpuStatList->number) {
			parsedInfo->cpuStatList =
			     (pwatch_cpuStatList*)parsedInfo->cpuStatList->back;

		} else if (cpuNmb > parsedInfo->cpuStatList->number ||
			   parsedInfo->cpuStatList->next == NULL) {
			parsedInfo->cpuStatList->next =
			(pwatch_cpuStatList*)malloc(sizeof(pwatch_cpuStatList));

			if(!(pwatch_cpuStatList*)parsedInfo->cpuStatList->next)
				return PWATCH_ERR_FAILED_TO_ACQUIRE_MEMORY;

			parsedInfo->cpuStatList =
			     (pwatch_cpuStatList*)parsedInfo->cpuStatList->next;

		} else if (cpuNmb > parsedInfo->cpuStatList->number) {

			parsedInfo->cpuStatList =
			     (pwatch_cpuStatList*)parsedInfo->cpuStatList->next;
		} else {
			return PWATCH_ERR_FAILED_TO_PARSE;
		}
	}

	return PWATCH_SUCCESS;
}


inline void pwatch_parseInfoPtr(unsigned int **Ptr)
{
	Ptr[0] = &((pwatch_cpuStat*)parsedInfo->cpuStatList->node)->user;
	Ptr[1] = &((pwatch_cpuStat*)parsedInfo->cpuStatList->node)->nice;
	Ptr[2] = &((pwatch_cpuStat*)parsedInfo->cpuStatList->node)->system;
	Ptr[3] = &((pwatch_cpuStat*)parsedInfo->cpuStatList->node)->idle;
	Ptr[4] = &((pwatch_cpuStat*)parsedInfo->cpuStatList->node)->iowait;
	Ptr[5] = &((pwatch_cpuStat*)parsedInfo->cpuStatList->node)->irq;
	Ptr[6] = &((pwatch_cpuStat*)parsedInfo->cpuStatList->node)->softirq;
}

int pwatch_parseLine(const char* line)
{
	int ret, i, cpuNmb;
	unsigned int *Ptr[7];

	ret = strchr(line, ' ') - line;
	if (ret < 3)
		return PWATCH_ERR_FAILED_TO_PARSE;


	if (!strncmp("cpu", line, 3)) {
		pwatch_parseInfoPtr(Ptr);


		if (line[4] == ' ')
			cpuNmb = -1;
		else if (line[4] >= '0' && line[4] <= '9')
			cpuNmb = line[4] - '0';
		else
			return PWATCH_ERR_FAILED_TO_PARSE;

		if (!(pwatch_cpuStat*)parsedInfo->cpuStatList->node)
			return PWATCH_ERR_NULL_TERMINATOR;

		ret = pwatch_moveParsedInfo(cpuNmb);

		if(ret)
			return ret;

		pwatch_parseInfoPtr(Ptr);

		/* move pointer past cpun */
		line += 4;

		for (i = 0; i < 7; i++) {

			if(*line == ' ')
				line++;

			if(!Ptr[i])
				return PWATCH_ERR_NULL_TERMINATOR;


			*(Ptr[i]) = atoi(line);
			line = strchr(line, ' ');
		}
	} else if (!strncmp("ctxt", line, ret)) {
		line = strchr(line, ' ');
		line++;
		parsedInfo->ctxt = atoi(line);
	} else if (!strncmp("btime", line, ret)) {
		line = strchr(line, ' ');
		line++;
		parsedInfo->ctxt = atoi(line);
	} else if (!strncmp("processes", line, ret)) {
		line = strchr(line, ' ');
		line++;
		parsedInfo->ctxt = atoi(line);
	} else if (!strncmp("procsRunning", line, ret)) {
		line = strchr(line, ' ');
		line++;
		parsedInfo->ctxt = atoi(line);
	} else if (!strncmp("procsBlocked", line, ret)) {
		line = strchr(line, ' ');
		line++;
		parsedInfo->ctxt = atoi(line);
	}

	return PWATCH_SUCCESS;
}


/**
  * pwatch_parseStat
  *	 Analyzing thread that should take the elements of the queue
 **/
int pwatch_parseStat(void)
{
	int ret;
	pwatch_queueLineBuf *aux;

	while (true) {
		if (readBuff == NULL)
			return PWATCH_ERR_FAILED_TO_READ;
		if (adminFlags & PWATCH_ADMIN_CLOSE) {
			adminFlags &= ~PWATCH_ADMIN_PARSE;
			adminFlags &= ~PWATCH_ADMIN_PARSE_CLOSE ;
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
			ret = pwatch_parseLine(aux->line);
			if(ret)
				return ret;

			free(aux);
			readSemaphore->nmb--;
		} else if (readSemaphore->nmb <= 1) {
			readSemaphore->emptyFlg = true;
			if(pthread_cond_wait(&readSemaphore->empty, &lock))
				return PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL;
			readSemaphore->emptyFlg = false;
		} else {
			return PWATCH_ERR_FAILED_TO_PROCESS_STATUS;
		}

		if (readSemaphore->fullFlg &&
		    readSemaphore->nmb <= PWATCH_LINE_LIMIT) {
			if (pthread_cond_broadcast(&readSemaphore->full))
				return PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL;
		}

		pthread_mutex_unlock(&lock);
	}

	return PWATCH_SUCCESS;
}

/**
  * releaseQueueLineBuf
  *	 release the reading queue
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
  *	 skeleton of a function which purpose is to read /proc/stat file
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
			adminFlags &= ~PWATCH_ADMIN_READ_CLOSE ;
			return PWATCH_SUCCESS;
		}
		//printf("debug pwatch_readStat 2\n");

		/* it comes here when you stop reading in case of timeout f.e.*/
		if(!(adminFlags & PWATCH_ADMIN_READ))
			continue;

		if (procStat == NULL)
			return PWATCH_ERR_FAILED_TO_OPEN;

		ret_line_size = getline(&lineStruct->line, &len, procStat);

		if (feof(procStat)) {
			fclose(procStat);
			procStat = fopen(procStatPath, "r");
			continue;
		} else if (ret_line_size <= 0) {
			return PWATCH_ERR_FAILED_TO_READ;
		} else {
			/* null terminating the string */
			lineStruct->line[ret_line_size - 1] = 0;
		}

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
			pthread_mutex_unlock(&lock);
			if (pthread_cond_wait(&readSemaphore->full, &lock))
				return PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL;
			readSemaphore->fullFlg = false;
		} else if (readSemaphore->emptyFlg) {
			if (pthread_cond_broadcast(&readSemaphore->empty))
				return PWATCH_ERR_FAILED_TO_PROCESS_SIGNAL;
		}

		pthread_mutex_unlock(&lock);
	}
}

static void *pwatch_readStatThreadWrap(void *argument)
{
	int ret = pwatch_readStat();
	if (ret) {
		printf(" In read thread :%s %d \n", pwatch_errToString(ret), ret);
	}

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

int pwatch_initialize(void)
{
	int ret;
	adminFlags &= ~PWATCH_ADMIN_INIT;

	parsedInfo = malloc(sizeof(pwatch_parsedInfo));
	readBuff = malloc(sizeof(pwatch_queueLineBuf));
	parsedInfo->cpuStatList = malloc(sizeof(pwatch_cpuStatList));
	parsedInfo->cpuStatList->node = malloc(sizeof(pwatch_cpuStat));
	if(!parsedInfo || !readBuff || !parsedInfo->cpuStatList ||
		!parsedInfo->cpuStatList->node) {
		return PWATCH_ERR_FAILED_TO_ACQUIRE_MEMORY;
	}

	parsedInfo->cpuStatList->number = -1;
	readBuff->line = NULL;
	readBuff->next = NULL;

	readSemaphore = malloc(sizeof(pwatch_semaphore));
	readSemaphore->nmb = 1;
	readSemaphore->fullFlg = false;
	readSemaphore->emptyFlg = true;
	adminFlags |= PWATCH_ADMIN_READ;
	adminFlags |= PWATCH_ADMIN_PARSE;

	ret = pthread_cond_init(&readSemaphore->empty, NULL);
	if (ret)
		return ret;

	ret = pthread_cond_init(&readSemaphore->full, NULL);
	if (ret)
		return ret;


	return PWATCH_SUCCESS;
}

int pwatch_watchDog(void)
{
	int ret;

	ret = 0;

	if (adminFlags & PWATCH_ADMIN_INIT)
		ret = pwatch_initialize();

//	 if (!ret && (adminFlags & PWATCH_ADMIN_READ) &&
//		 (timeouts->read > PWATCH_USEC_TIMEOUT)) {

//		 if (ret)
//			 printf("%s - %d \n", pwatch_errToString(ret), ret);
//		 else
//			 adminFlags &= ~PWATCH_ADMIN_READ;
//	 }

	// if (adminFlags & PWATCH_ADMIN_PARSE) {
	//	 ret = pthread_create(&readThread, NULL, pwatch_parseStatThreadWrap, NULL);
	//	 if (ret) {
	//		 printf("%s - %d \n", pwatch_errToString(ret), ret);
	//	 } else {

	//		 adminFlags &= ~PWATCH_ADMIN_READ;
	//	 }
	// }

	if (adminFlags & PWATCH_ADMIN_CLOSE && !ret) {
		while (adminFlags != PWATCH_ADMIN_CLOSE) {
			if (adminFlags & PWATCH_ADMIN_READ)
				adminFlags &= PWATCH_ADMIN_PARSE_CLOSE;

			if (adminFlags & PWATCH_ADMIN_PARSE)
				adminFlags &= PWATCH_ADMIN_READ_CLOSE;
		}
	}

	return PWATCH_EXIT;
	return 0;
}


int main(int argc, char** argv)
{
	int ret = 1;
	adminFlags = PWATCH_ADMIN_INIT;

	while (ret)
		ret = pwatch_watchDog();

	if (ret != PWATCH_EXIT) {
		fprintf(stderr, pwatch_errToString(ret));
		return ret;
	}

	return 0;
}
