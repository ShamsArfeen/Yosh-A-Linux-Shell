#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "parse.h"
#include "yosh.h"

int main() {

	signal( SIGCHLD, handle_sigchld );
	while( 1 ) {
		int childPid, job;
		char *cmdLine;
		char prompt[1000];
		parseInfo *ipinfo;
		
		getcwd(prompt, 1000);
		strcat(prompt, "> ");

		cmdLine = readline(prompt);
		
		parseCmd:
		ipinfo = parse(cmdLine);
 		
 		if (sscanf(cmdLine, "!%d", &job) != 1) recordHistory(cmdLine);
 		
		if ( isBuiltInCommand( ipinfo )) {
			int ret = executeBuiltInCommand( ipinfo, cmdLine);
			if (ret == 2) goto parseCmd;
		} 
		else {		
			childPid = fork();
			if( childPid == 0 ) {
				executeCommand( ipinfo );
			} 
			else {
				if( ipinfo->boolBackground ) {
			        	recordJob( childPid, ipinfo->CommArray[0].command );
				} 
				else {
					waitpid ( childPid, NULL, 0 );
				}		
			}
		}
		free(cmdLine);
		free_info(ipinfo);
	}
	return 0;
}

