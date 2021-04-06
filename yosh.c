#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "parse.h"

#define CMD_SIZE 100
#define HISTORY_SIZE 10 // history size is 1 less than this number
#define JOBS_SIZE 100

int currPtr = 0;
int startPtr = -1;

char cmdHistory[HISTORY_SIZE][CMD_SIZE];
int bgJobs[JOBS_SIZE];
char jobName[JOBS_SIZE][CMD_SIZE];

int jbNum = 0;

int recordJob(int job, char* command) {

	bgJobs[jbNum] = job;
	strcpy(jobName[jbNum], command);
	jbNum++;
	
	return jbNum;
}


void handle_sigchld( int s ) {

	int pid;
	Loop:
	pid = waitpid(0, NULL, WNOHANG);
	if (pid <= 0) return;
	 
	for (int i = jbNum-1; i >= 0; i--) {
	
		if ( bgJobs[i] == pid) {
			if ( i == jbNum-1) jbNum--;
			else {
				bgJobs[i] = bgJobs[jbNum-1];
				strcpy(jobName[i], jobName[jbNum-1]);
				jbNum--;
			}
		}
	}
	goto Loop;
}

int recordHistory(char *cmdLine) {
	strcpy(cmdHistory[currPtr], cmdLine);
	if (startPtr == -1) startPtr = currPtr;
	currPtr++;
	currPtr %= HISTORY_SIZE;
	if (startPtr == currPtr) {
		startPtr = currPtr+1;
		startPtr %= HISTORY_SIZE;
	}
	return currPtr;
}

int isBuiltInCommand(parseInfo *ipinfo) {

	struct commandType * com;
	com = &(ipinfo->CommArray[0]);
	int job;
	
	if ( strcmp(com->command, "history") == 0) 
		return 1;
	if ( strcmp(com->command, "cd") == 0) 
		return 1;
	if ( strcmp(com->command, "jobs") == 0) 
		return 1;
	if ( strcmp(com->command, "help") == 0) 
		return 1;
	if ( strcmp(com->command, "exit") == 0) 
		return 1;
	if ( strcmp(com->command, "kill") == 0 && com->VarNum == 2) {
		if (sscanf(com->VarList[1], "%%%d", &job)==1)
			return 1; 
	}
	if ( sscanf(com->command, "!%d", &job) == 1 )
		return 1; 

	return 0;
}

int executeBuiltInCommand(parseInfo *ipinfo, char* cmdLine) {

	struct commandType * com;
	int job;
	com = &(ipinfo->CommArray[0]);
	
	if ( strcmp(com->command, "history") == 0) {
	
		if (startPtr == -1) {
			printf("History empty\n");
			return 0;
		}
		else {
			int num = 1;
			for (int i = startPtr; i != currPtr; i++, i%=HISTORY_SIZE) {
				printf(" %d  %s \n", num, cmdHistory[i]);
				num++;
			}
		}
	}
	else if ( strcmp(com->command, "cd") == 0 && com->VarNum == 2) {
	
		int ret = chdir( com->VarList[1]);
		if (ret == -1 ) printf("Invalid arg: errno %d\n", errno);
	}
	else if ( strcmp(com->command, "help") == 0) {
	
		FILE *fptr = fopen("HELP.txt", "r");
		char iLine[500];
		
		while (fgets(iLine, 500, fptr)) {
			printf("%s", iLine);
		}
		fclose(fptr);
	}
	else if ( strcmp(com->command, "exit") == 0) {
	
		if (jbNum > 0)
			printf("Cannot exit: %d background jobs\n", jbNum);
		else 
			exit(0);
	}
	else if ( strcmp(com->command, "jobs") == 0) {
		
		if (jbNum == 0)
			printf("No background jobs\n");
		for (int i=0; i<jbNum; i++)
			printf(" %d  %s \n", i+1, jobName[i]);
	}
	else if ( strcmp(com->command, "kill")==0 ) {
	
		sscanf(com->VarList[1], "%%%d", &job);
		job -= 1;
		if (job >= jbNum || job < 0)
			return 0;
			
		int h = waitpid(bgJobs[job], NULL, WNOHANG);
		
		if ( h == -1)
			printf("No such job in jobs list\n");
		else {
		
			int ret = kill (bgJobs[job], SIGKILL);
			if (ret == 0) {
				printf("Killing job# %d pid# %d\n", job+1, bgJobs[job]);
				while ( waitpid(bgJobs[job], NULL, WNOHANG) > 0)
					sleep(1);
			}
			else
				printf("Error kill: errno %d\n", errno);
		}		
	}
	else if ( sscanf(cmdLine, "!%d", &job) == 1) {
	
		if ( job < 0 ) {
			strcpy(cmdLine, cmdHistory[(currPtr+job) % HISTORY_SIZE]);
			return 2;
		}	
		else if ( job > 0) {
			strcpy(cmdLine, cmdHistory[(startPtr-1+job) % HISTORY_SIZE]);
			return 2;
		}		
	}

	return 0;
}

int executeCommand(parseInfo *ipinfo) {

	struct commandType * com;
	int i;
	
	
	if (ipinfo->boolInfile > 1) {
		printf("Ambiguous input redirect.\n");
		exit(1);
	}
	
	if (ipinfo->boolOutfile > 1)  {
		printf("Ambiguous output redirect.\n");
		exit(1);
	}
	
	if (ipinfo->boolInfile) {
		if (strlen(ipinfo->inFile) == 0) {
			printf("Missing name for redirect.\n");
			exit(1);
		}
		if (access(ipinfo->inFile, F_OK) == -1) {
			printf("%s: No such file or directory\n", ipinfo->inFile);
			exit(1);
		}
	}
	
	if (ipinfo->boolOutfile) {
		if (access(ipinfo->outFile, F_OK) == 0) {
			printf("%s: File exists\n", ipinfo->outFile);
			exit(1);
		}
		if (strlen(ipinfo->outFile) == 0) {
			printf("Missing name for redirect.\n");
			exit(1);
		}
	}
	
	if ( ipinfo->boolInfile) {
		int fd = open( ipinfo->inFile, 
			O_RDONLY, 
			S_IWUSR | S_IRUSR
		);
		if (fd == -1) {
			exit(1);
		}
		dup2(fd, 0);
					
	}
				
	if (ipinfo->pipeNum > 0) {
		for( i=0; i<ipinfo->pipeNum; i++)
		{
			int pd[2];
			pipe(pd);

			if (!fork()) {
				dup2(pd[1], 1);
				close(pd[0]);
				
				
				com = &(ipinfo->CommArray[i]);
				com->VarList[com->VarNum] = NULL;
				execvp(com->command, com->VarList);
				perror("exec");
				abort();
			}

			
			dup2(pd[0], 0);
			close(pd[1]);
		}
	}
	
	if ( ipinfo->boolOutfile ) {
	
		int fd = open( 
			ipinfo->outFile, 
			O_CREAT | O_WRONLY, 
			S_IWUSR | S_IRUSR
		);
		if (fd == -1) {
			exit(1);
		}
		dup2(fd, 1);
	}

	com = &(ipinfo->CommArray[i]);
	com->VarList[com->VarNum] = NULL;
	execvp(com->command, com->VarList);
	perror("exec");
	abort();
}


