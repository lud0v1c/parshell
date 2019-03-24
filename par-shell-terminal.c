#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 50

int main(int args,char **argv)
{

	int parshellfd;
	char command[BUFFER_SIZE];
	char pid[BUFFER_SIZE];

	if((parshellfd = open(argv[1], O_WRONLY)) == -1) {
		perror(ERR_OPEN);
		exit(EXIT_FAILURE);
	}

	/* Message par-shell its pid so it can know when sending something via name */
	sprintf(pid,"PID %d",getpid());
	strcat(pid,"\n"); // This is needed because we have an fgets on the other side and it needs an '\n'

	if(write(parshellfd,pid,strlen(pid)) == -1) {
		perror(ERR_WRITE);
		exit(EXIT_FAILURE);
	}

	while(1) {
		/* Get input */
		if(fgets (command,BUFFER_SIZE,stdin) == NULL) {
			continue;
		}
		/* Local exit, simply closes the pipe descriptor and exits the terminal */
		if((strncmp(command,"exit\n",sizeof(command))) == 0) {
			if(close(parshellfd) == -1)
				perror(ERR_CLOSE);
	
			exit(EXIT_SUCCESS);
		}

		/* Creates a temporary pipe wich will receive the number of active processes and total execution time */
		if((strncmp(command,"stats\n",sizeof(command))) == 0) {
			int statsfd;
			char statspid[BUFFER_SIZE] = "stats ";
			char pipename[BUFFER_SIZE],output[BUFFER_SIZE];

			/* par-shell will receive "stats" command plus the name of the temporary pipe (e.g: stats /tmp/par-shell-stats-1234) */
			sprintf(pipename,"/tmp/par-shell-stats-%d", getpid());
			strcat(statspid, pipename);
			strcat(statspid,"\n");

			if (mkfifo(pipename, 0666) !=0) {
				perror(ERR_MKFIFO);
				exit(EXIT_FAILURE);
			}
		
			if(write(parshellfd, statspid, strlen(statspid)) == -1) {
				perror(ERR_WRITE);
				exit(EXIT_FAILURE);
			}

			if((statsfd = open(pipename,O_RDONLY)) == -1) {
				perror(ERR_OPEN);
				exit(EXIT_FAILURE);
			}

			if(read(statsfd, output,BUFFER_SIZE) == -1) {
				perror(ERR_READ);
				exit(EXIT_FAILURE);
			}

			puts(output);

			/* Since it's not needed anymore close it */
			if(close(statsfd) == -1) {
				perror(ERR_CLOSE);
				exit(EXIT_FAILURE);
			}

			if(unlink(pipename) == -1) {
				perror(ERR_UNLINK);
				exit(EXIT_FAILURE);
			}

			continue;
		}
		/* Send to par-shell */
		if(write(parshellfd, command, strlen(command)) == -1) {
			perror(ERR_WRITE);
			exit(EXIT_FAILURE);
		}
	}	
}

