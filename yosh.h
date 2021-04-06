

int recordJob(int job, char *command);

int recordHistory(char *cmdLine);

void handle_sigchld( int s );

int isBuiltInCommand(parseInfo *ipinfo);

int executeBuiltInCommand(parseInfo *ipinfo, char* cmdLine);

int executeCommand(parseInfo *ipinfo);
