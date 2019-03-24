#include "errors.h"
#include "commandlinereader.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#define MAXPAR 4
#define MAXARGS 7
#define BUFFER_SIZE 256
#define _THREAD_SAFE /* thread handling compiler flag */

/* numChildren: Stores the total number of child processes created
   isActive: Flag that signals if the monitoring thread should keep going if there are no more processes
   lock: To be used with pthread_mutex (locking and unlocking critical areas)
*/

int numChildren,isActive,totalIters,totalProcTime;
static volatile int keepRunning = 1;
pthread_t monitorThread;
pthread_mutex_t lock;
pthread_cond_t cond_max,cond_sleep;
FILE *flog;
char *parshellin;

typedef struct lst_iitem {
   int pid,status;
   time_t starttime;
   time_t endtime;
   struct lst_iitem *next;
} lst_iitem_t;

typedef struct {
   lst_iitem_t * first;
} list_t;

list_t *list,*pid_list;

list_t* lst_new();
void lst_destroy(list_t *);
void insert_new_process(list_t *list, int pid, time_t starttime);
void update_terminated_process(list_t *list, int pid,int status);
void lst_print(list_t *list);
void kill_pids(list_t *list);
void sig_handler(int signo);
void leave(pthread_t thread,list_t *list);
void *monitorProcs(void *ptr);
void updateLog(int pid,int duration);
void printLog();
int getStoredVars();