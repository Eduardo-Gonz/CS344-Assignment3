#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

int ignoreCmd(char usrCmd []) {
    char firstChar = usrCmd[0];
    if(firstChar == '#' || strcmp(usrCmd, "\n") == 0){
        printf("Sorry, blank lines and comments are ignored\n");
        return 1;
    }
    

    return 0;
}

void createCmdLine() {
    char cmd [2049];
    char args [513];
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

        //check for three commands
        //make a process and call execvp()
    }while(exit < 1);

}


int main()
{
    createCmdLine();

    return EXIT_SUCCESS;
}