#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

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

int isThreeCmds(char *cmdArgs[]) {
    if(strcmp(cmdArgs[0], "cd") == 0)
        printf("change directory!");
    else if(strcmp(cmdArgs[0], "status") == 0)
        printf("show status!");
    else if(strcmp(cmdArgs[0], "exit") == 0)
        printf("exit!");
    else
        return 0;

    return 1;
}

void createCmdLine() {
    char cmd [2049];
    char *args [513];
    int exit = 0;

    do{
        printf(": ");
        fgets(cmd, 2049, stdin);
        fflush(stdout);

        //check for blank lines or comments
        if(ignoreCmd(cmd))
            continue;
        else
            exit ++;

        parseCmd(cmd, args);
        if(!isThreeCmds(args))
            printf("\nnot a built in command");

        //make a process and call execvp()
    }while(exit < 1);

}


int main()
{
    createCmdLine();

    return EXIT_SUCCESS;
}