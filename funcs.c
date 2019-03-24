#include "errors.h"
#include "funcs.h"

/* Creates new list and initializes it by allocating memory */
list_t* lst_new()
{
   list_t *list;
   list = (list_t*) malloc(sizeof(list_t));
   list->first = NULL;
   return list;
}

/* Frees the list and all its items */
void lst_destroy(list_t *list)
{
	struct lst_iitem *item, *nextitem;
	item = list->first;

	/* Browse through our linked list,also used in other functions */
	while (item != NULL) {
		nextitem = item->next;
		free(item);
		item = nextitem;
	}

	free(list);
}

/* Same as lst_destroy but instead sends SIGINT's to all terminals*/
void kill_pids(list_t *list)
{
	struct lst_iitem *item, *nextitem;
	item = list->first;

	/* Browse through our linked list,also used in other functions */
	while (item != NULL) {
		kill(item->pid,SIGINT);
		nextitem = item->next;
		free(item);
		item = nextitem;
	}

	free(list);
}

/* Prints all successful processes duration and exit status */
void lst_print(list_t *list)
{
	lst_iitem_t *item;
	item = list->first;

	while (item != NULL) {
		if((item->endtime) != 0) {
			printf("Process %d ran for %0.2f seconds and ended with status %d\n",
			item->pid,difftime(item->endtime,item->starttime),item->status);
		}
		item = item->next;
	}
}

/* Searches the list for a certain process and saves its endtime and exit status */
void insert_new_process(list_t *list, int pid, time_t starttime)
{

	/* Need to protect the list variable to avoid messing up values */
	lst_iitem_t *item;

	item = (lst_iitem_t *) malloc (sizeof(lst_iitem_t));
	item->pid = pid;
	item->starttime = starttime;
	item->endtime = 0;
	item->next = list->first;
	list->first = item;

}

/* Scans each line of log.txt and updates global variables (totalProcTime and totalIters) */
int getStoredVars()
{
	int localProcTime = 0; /* Default values for global variables */
	int localIters = -1;
	char buffer[BUFFER_SIZE]; /* Creates buffer for fgets */

	while(fgets(buffer, BUFFER_SIZE, flog) != NULL) {
		sscanf(buffer,"iteracao %d",&localIters); /* filters each line of text */
		sscanf(buffer,"total execution time: %d",&localProcTime);
		
	}

	if(localIters >= 0) {
		totalProcTime = localProcTime; 
		totalIters = ++localIters;
	}
	
	return 0;
}

/* Writes to the log file the process count,pid,*/
void updateLog(int pid,int duration)
{
	fprintf(flog, "iteracao %d\n", totalIters);
	fprintf(flog, "pid: %d execution time: %d s\n", pid, duration);
	fprintf(flog, "total execution time: %d s\n", totalProcTime);

	if(fflush(flog) == EOF) {
		perror("Critical error flushing log file: ");
		exit(EXIT_FAILURE);
	}
}

/* Searches the list for a certain process and saves its endtime and exit status */
void update_terminated_process(list_t *list, int pid,int status)
{
	/* Protects our list from being written/read while working on it */
	lst_iitem_t *item;
	item = list->first;

	while(item != NULL) {
		if((item->pid) == pid) {
			item->endtime = time(NULL);
			item->status = status;
			totalProcTime += difftime(item->endtime,item->starttime);
			updateLog(pid,difftime(item->endtime,item->starttime));
			break;

		}
		item = item->next;
	}
}

void sig_handler(int signo)
{
	leave(monitorThread,list);
}

/* Ends all processes and exits the shell after printing all ran processes info */
void leave(pthread_t thread,list_t *list)
{
	kill_pids(pid_list);
	pthread_mutex_lock(&lock);	// Need to protect the variable to avoid messing up values
	isActive = 0;	// Sets flag to stop monitor thread 

	if(pthread_cond_signal(&cond_sleep) != 0){
		exit(EXIT_FAILURE);
	}
	pthread_mutex_unlock(&lock);
	
	/* Waits for the secondary thread to finish,by signaling it with pthread_join */
	pthread_join(thread,NULL);
	lst_print(list);
	lst_destroy(list);

	if (pthread_mutex_destroy(&lock) != 0 || pthread_cond_destroy(&cond_max) != 0 || pthread_cond_destroy(&cond_sleep) != 0) {
		puts("Critical error destroying mutex or condition variables");
		exit(EXIT_FAILURE);
	}

	if(fclose(flog) != 0) {
		puts("Critical error closing file");
		exit(EXIT_FAILURE);
	}

	unlink(parshellin);
	exit(EXIT_SUCCESS);
}

/* Secondary thread which monitors child processes duration */
void *monitorProcs(void *ptr) 
{ 
	int waitPid,status;
	list_t *list = (list_t *)ptr;
	/* Update process list if there are child processes running or
	   if the main thread hasn't signaled its termination yet.
	*/
	pthread_mutex_lock(&lock);	// Protects our variables isActive and numChildren
	while(isActive || numChildren > 0) {
		if (numChildren > 0){
			pthread_mutex_unlock(&lock);
			waitPid = wait(&status);

			if(WIFEXITED(status)) {	// If we got a legit exit status, write on the list its info 
				pthread_mutex_lock(&lock);	// Protects our list from being written/read while working on it 
				update_terminated_process(list,waitPid,WEXITSTATUS(status));
				numChildren--; // Decreases number of processes being run. Also needs to be protected to avoid wrong writings
				totalIters++;

				pthread_mutex_unlock(&lock);
				if (pthread_cond_signal(&cond_max) != 0) {
					exit(EXIT_FAILURE);
				}
			}
		} else {
			while(isActive && numChildren == 0) {
				if(pthread_cond_wait(&cond_sleep, &lock) != 0) { // If the processes limit has been reached , waits for pthread_cond_signal(&cond_sleep)
					exit(EXIT_FAILURE);
				} 
				pthread_mutex_unlock(&lock);
				if (!isActive) return NULL;
			}	
		}
	pthread_mutex_lock(&lock); 
	}
	pthread_mutex_unlock(&lock);
	return NULL;
}
