/**********************
Projeto SO - Parte 5
Grupo 03
(Names omitted)
**********************/

#include "funcs.h"

int main(int args,char **argv)
{

	int readargs,pid,parshellfd;
	char *clargs[MAXARGS];
	char buffer[BUFFER_SIZE];

	totalIters = totalProcTime = numChildren = 0;
	list = lst_new();
	pid_list = lst_new(); // Will hold the pids of all par-shell-terminals active 
	isActive = 1; // Our secondary thread needs to start monitoring right away
		
	/* Open log.txt in append mode */
	if ((flog = fopen("log.txt","a+")) == NULL) {
		perror(ERR_FOPEN);
       	exit(EXIT_FAILURE);
    }

   	/* Updates variables needed to write on the log file */
	getStoredVars(flog); 

	/* Creates two semaphores, else prints error message */
	if(pthread_cond_init(&cond_max, NULL) || pthread_cond_init(&cond_sleep, NULL)) {
        perror(ERR_COND);
       	exit(EXIT_FAILURE);
   	}

    /* Creates one mutex, else prints error message */
	if(pthread_mutex_init(&lock,NULL) != 0) {
       	perror(ERR_MTXINIT);
        exit(EXIT_FAILURE);
   	}

	/* Creates and starts the monitoring thread, else prints error message */
	if(pthread_create(&monitorThread,NULL,monitorProcs,list) != 0) {
		perror(ERR_MTHREAD);
		exit(EXIT_FAILURE);
	}

	if (mkfifo(argv[1], 0666) !=0) {
		perror(ERR_MKFIFO);
		exit(EXIT_FAILURE);
	}

	if((parshellfd = open(argv[1],O_RDONLY)) == -1) {
		perror(ERR_OPEN);
		exit(EXIT_FAILURE);
	}

	if(close(0) == -1) {
		perror(ERR_CLOSE);
		exit(EXIT_FAILURE);
	}

	if(dup(parshellfd) == -1) {
		perror(ERR_DUP);
		exit(EXIT_FAILURE);
	}

	if(signal(SIGINT,sig_handler) == SIG_ERR) {
		perror(ERR_SIGN);
		exit(EXIT_FAILURE);
	}
	
	while(1){
		
		readargs = readLineArguments(clargs, MAXARGS, buffer, BUFFER_SIZE);

		if(readargs <= 0) {
			/* If a signal was received (like SIGINT) ignore it to avoid repeated output and try to restablish pipe*/
			if (readargs == -1) {
				if(close(parshellfd) == -1){
					perror(ERR_CLOSE);
					exit(EXIT_FAILURE);
				}
				if((parshellfd = open(parshellin,O_RDONLY)) == -1){
					perror(ERR_OPEN);
					exit(EXIT_FAILURE);
				}
			} else{

				fprintf(stderr, "Invalid input\n");
			}
			continue;
		}

		/* Store the pid from every report message we get from new terminals */
		if(strcmp("PID",clargs[0]) == 0) {
			insert_new_process(pid_list,atoi(clargs[1]),0);
			continue;
		}

		if(strcmp("exit-global",clargs[0]) == 0) {
			leave(monitorThread,list);
		}
			
		if(strcmp("stats",clargs[0]) == 0) {
			
			int statsfd;
			char outStats[BUFFER_SIZE];

			if((statsfd = open(clargs[1],O_WRONLY)) == -1) {
				perror(ERR_OPEN);
				exit(EXIT_FAILURE);
			}

			sprintf(outStats, "Active processes: %d\nTotal execution time: %d", numChildren, totalProcTime);

			if(write(statsfd, outStats, sizeof(outStats)) == -1) {
				perror(ERR_WRITE);
				exit(EXIT_FAILURE);
			}

			if(close(statsfd) == -1) {
				perror(ERR_CLOSE);
				exit(EXIT_FAILURE);
			}
		
			continue;
		}
		
		pthread_mutex_lock(&lock);

		while(numChildren >= MAXPAR)
			if(pthread_cond_wait(&cond_max,&lock) != 0) // If the processes limit has been reached , waits for pthread_cond_signal(&cond_max)
				exit(EXIT_FAILURE);
			
		pthread_mutex_unlock(&lock);

		if(((pid = fork()) == 0)){	// If everything went right,proceed to launch the child process's executable

			char name[BUFFER_SIZE];
			int pfd;
			
			if(signal(SIGINT,SIG_IGN) == SIG_ERR) {
				perror(ERR_SIGN);
				exit(EXIT_FAILURE);
			}
			
			snprintf(name,BUFFER_SIZE,"par-shell-out-%d.txt",getpid());

			if ((pfd = open(name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR)) == -1) {
    			perror(ERR_OPEN); 
    			exit(EXIT_FAILURE);
			}
			
			/* Close stdout and replace with the newly created file descriptor */
			if(close(1) == -1) {
				perror(ERR_CLOSE);
				exit(EXIT_FAILURE);
			}

			if(dup(pfd) == -1) {
				perror(ERR_DUP);
				exit(EXIT_FAILURE);
			}

			execv(clargs[0],clargs);

			perror(ERR_CPROC);
			exit(EXIT_FAILURE); // To avoid repeated output after printing the error message 

		} else if(pid > 0) {	// Parent Process

			pthread_mutex_lock(&lock); // Need to protect the list variable to avoid messing up values
			numChildren++;	// Increments each time a new process is created
			insert_new_process(list,pid,time(NULL));
			pthread_mutex_unlock(&lock);

			if(pthread_cond_signal(&cond_sleep) != 0){
				exit(EXIT_FAILURE);
			}

		} else	// Invalid pid,forking went wrong
			puts(ERR_FORK);
	}
}
