#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int ignoreCmd(char *usrCmd) {
    char firstChar = usrCmd[0];
    if(firstChar == '#' || strcmp(usrCmd, "\n") == 0){
        printf("Sorry, blank lines and comments are ignored\n");
        fflush(stdout);
        return 1;
    }
    
    return 0;
}

void parseCmd(char *usrCmd, char *cmdArgs[]) {
    char *token;
    char *savePtr = usrCmd;
    int i = 0;

    while((token = strtok_r(savePtr, " ", &savePtr))) {
        //remove trailing 'new line' character
        token[strcspn(token, "\n")] = 0;
        //store the argument in array
        cmdArgs[i] = token;
        i++;
    }
}

void changeDir(char *dirPath) {
    if(chdir(dirPath) == -1){
        perror("Error: ");
        fflush(stdout);
    }
}

void handleCd(char *cmdArgs[]) {
    char *path;
    if(cmdArgs[1] == NULL)
        path = getenv("HOME");
    else
        path = cmdArgs[1];

    changeDir(path);
}

void exitPrgm(int *pids[]) {
    int i = 0;
    while(pids[i] != NULL) {
        int killValue = kill(*pids[i], SIGTERM);
        if(killValue == -1 && errno == ESRCH)
            continue;
        else{
            perror("ERROR: ");
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
        i++;
    }

    exit(EXIT_SUCCESS);
}

void showStatus(int *status) {
    printf("exit value %d\n", *status);
    fflush(stdout);
}

int isThreeCmds(char *cmdArgs[], int *pids[], int *exitStatus) {
    if(strcmp(cmdArgs[0], "cd") == 0)
        handleCd(cmdArgs);
    else if(strcmp(cmdArgs[0], "status") == 0)
        showStatus(exitStatus);
    else if(strcmp(cmdArgs[0], "exit") == 0)
        exitPrgm(pids);
    else
        return 0;
    return 1;
}

int findLength(char *args[]) {
    int i = 0;
    while(args[i] != NULL) {
        i++;
    }
    int length = i;
    return length; 
}

int findNumOfPids(int *pids[]) {
    int i = 0;
    while(pids[i] != NULL) {
        i++;
    }

    int length = i - 1;
    return length; 
}

int isBackground(char *cmdArgs[], int lastArg) {
    lastArg--;
    if(strcmp(cmdArgs[lastArg], "&") == 0){
        cmdArgs[lastArg] = NULL;
        return 1;
    }

    return 0;
}

void copyToExec(char *execArgs[], char *cmdArgs[], int length) {
    for(int i = 0; i < length; i++) {
        execArgs[i] = cmdArgs[i];
    }
    execArgs[length] = NULL;
}

int isRedirect(char *cmdArgs[], int length) {
    for(int i = 0; i < length; i++) {
        if(strcmp(cmdArgs[i], "<") == 0 || strcmp(cmdArgs[i], ">") == 0)
            return 1;
    }

    return 0;
}

char * getIO(char *cmdArgs[], int length, char *symbol) {
    char *defaultName = "/dev/null";
    for(int i = 0; i < length; i++) {
        if(strcmp(cmdArgs[i], symbol) == 0){
	   return cmdArgs[++i];
	}
    }

    return defaultName;
}

void redirectIO(char *input, char*output){
    //use dup2 to redirect input
	int inputFD = open(input, O_RDONLY);
	if (inputFD == -1) {
		perror("open()");
	}
    int fdStatus = dup2(inputFD, 0);

	if (fdStatus == -1) {
		perror("error: ");
	}

	// Use dup2 to redirect output
    int outputFD = open(output, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (outputFD == -1) {
		perror("dup2"); 
	}
    fdStatus = dup2(outputFD, 1);
    if (fdStatus == -1) {
	    perror("error: ");
	}
}

void backgroundIO(char *input, char *output, int background) {
    if(background && output == NULL) {
        output = "/dev/null";
    }
    if(background && input == NULL) {
        input = "/dev/null";
    }
}

void modifyArgsIO(char *cmdArgs[], int length) {
    for(int i = 1; i < length; i++) {
        cmdArgs[i] = NULL;
    }
}

void forkCmds(char *cmdArgs[], int *pids[], int *exitStatus) {
    int length = findLength(cmdArgs);
    int numOfPids = findNumOfPids(pids);
    int bckgrndMode = isBackground(cmdArgs, length);
    int childStatus, redirect;
    char *execArgs [length];
    char *outputFile, *inputFile;

    //Set up Redirection if needed
    redirect = isRedirect(cmdArgs, length);
    if(redirect){
        inputFile = getIO(cmdArgs, length, "<");
        outputFile = getIO(cmdArgs, length, ">");
        redirectIO(inputFile, outputFile);
        backgroundIO(inputFile, outputFile, bckgrndMode);
        modifyArgsIO(cmdArgs, length);
    }
    copyToExec(execArgs, cmdArgs, length);
    printf("%s", inputFile);

	// Fork a new process
	pid_t spawnPid = fork();
	switch(spawnPid){
        case -1:
            perror("fork()\n");
            exit(1);
            break;
        case 0:
            //Childs Process
            *exitStatus = execvp(execArgs[0], execArgs);
	    if(*exitStatus == -1){
	    	perror("execvp: ");
		exit(EXIT_FAILURE);
	    }
        default:
            // In the parent process
            if(bckgrndMode){
	        pids[numOfPids] = &spawnPid;
                printf("background pid is %d\n", spawnPid);
                spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
                fflush(stdout);
            } 
            else{
                spawnPid = waitpid(spawnPid, &childStatus, 0);
                if(WIFEXITED(childStatus))
                    *exitStatus = WIFEXITED(childStatus);
            }
            break;
	} 
    
}

void clearArgs(char *args[]) {
    int i = 0;
    while(args[i] != NULL) {
        args[i] = NULL;
        i++;
    }
}

//void checkProcesses(char

void createCmdLine() {
    char cmd [2049];
    char *args [513] = {NULL};
    int exit = 0;
    int *pidArr [200] = {NULL};
    int exitStatus = 0;

    do{
        printf(": ");
        fgets(cmd, 2049, stdin);
        fflush(stdout);

        //check for blank lines or comments
        if(ignoreCmd(cmd))
            continue;

        parseCmd(cmd, args);

        if(!isThreeCmds(args, pidArr, &exitStatus)) {
            forkCmds(args, pidArr, &exitStatus);
        }

        clearArgs(args);

        //check processes

    }while(exit < 1);

}

int main()
{
    createCmdLine();

    return EXIT_SUCCESS;
}
