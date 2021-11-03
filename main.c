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

int isBackground(char *cmdArgs[], int lastArg) {
    lastArg--;
    if(strcmp(cmdArgs[lastArg], "&") == 0)
        return 1;

    return 0;
}

void copyToExec(char *execArgs[], char *cmdArgs[], int length) {
    for(int i = 0; i < length; i++) {
        execArgs[i] = cmdArgs[i];
    }
    execArgs[length] = NULL;
}

void forkCmds(char *cmdArgs[], int *pids[], int *exitStatus) {
    int length = findLength(cmdArgs);
    int bckgrndMode = isBackground(cmdArgs, length);
    char *execArgs [length];
    copyToExec(execArgs, cmdArgs, length);

    int childProcess = 0;
    int childStatus;

	// Fork a new process
	pid_t spawnPid = fork();
	switch(spawnPid){
        case -1:
            perror("fork()\n");
            exit(1);
            break;
        case 0:
            // In the child process
            printf("CHILD(%d) running command\n", getpid());
            // Replace the current program with "/bin/ls"

            *exitStatus = execvp(execArgs[0], execArgs);
            // exec only returns if there is an error
            perror("execvp: ");
            exit(2);
            break;
        default:
            // In the parent process
            // Wait for child's termination
            spawnPid = waitpid(spawnPid, &childStatus, 0);
            printf("PARENT(%d): child(%d) terminated. Exiting\n", getpid(), spawnPid);
            break;
	} 
    
}

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

        //check processes

        //make a process and call execvp()
    }while(exit < 1);

}

int main()
{
    createCmdLine();

    return EXIT_SUCCESS;
}
