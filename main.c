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

    printf("Exiting!");

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

        if(!isThreeCmds(args, pidArr, &exitStatus))
            printf("\nnot a built in command");

        //make a process and call execvp()
    }while(exit < 1);

}

int main()
{
    createCmdLine();

    return EXIT_SUCCESS;
}