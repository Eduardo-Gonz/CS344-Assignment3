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

/*
 * Specifies whether program is in foreground mode or not.
 * --------------------
 *       1 -> foreground mode activated
 *      -1 -> NOT in foreground mode and 1 
 */
int foregroundMode = -1;

/*
 * Function: handle_SIGTSTP()
 * --------------------
 * Enters or exits foreground mode when ctr^z is entered.
 *
 */
void handle_SIGTSTP() {
    if(foregroundMode == -1){
        char *message = "Entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, strlen(message));
        fflush(stdout);
        foregroundMode = 1;
    }
    else{
        char *message = "Exitingforeground-only mode\n";
        write(STDOUT_FILENO, message, strlen(message));
        fflush(stdout);
        foregroundMode = 1;       
    }
}

/*
 * expandVar(char *cmd)
 * --------------------
 * Parses through command and replaces all instances of '$$' with pid of parent process.
 *
 */
void expandVar(char *cmd) {
    char buffer[100] = {"\0"};
    char *p = cmd;
    pid_t pid = getpid();
    char replace [2049];
    sprintf(replace, "%d", pid);

    while ((p = strstr(p, "$$"))) {
        strncpy(buffer, cmd, p - cmd);
        strcat(buffer, replace);
        strcat(buffer, p+strlen("$$"));
        strcpy(cmd, buffer);
        p++;
    }
  
}

/*
 * ignoreCmd(char *usrCmd)
 * --------------------
 * Checks if command starts with '#' or if its a blank line. Returns 1 if true and 0 if false.
 * If true then the command is ignored.
 *
 */
int ignoreCmd(char *usrCmd) {
    char firstChar = usrCmd[0];
    if(firstChar == '#' || strcmp(usrCmd, "\n") == 0){
        return 1;
    }
    
    return 0;
}

/*
 * parseCmd(char *usrCmd, char *cmdArgs[])
 * --------------------
 * Parses the command and splits up each argument and stores them in an array.
 *
 */
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

/*
 * findLength(char *args[])
 * --------------------
 * Finds length of the argument array.
 *
 */
int findLength(char *args[]) {
    int i = 0;
    while(args[i] != NULL) {
        i++;
    }
    int length = i;
    return length; 
}

/*
 * findNumOfPids(int *pids)
 * --------------------
 * Finds the number of background pids
 *
 */
int findNumOfPids(int *pids) {
    int i = 0;
    while(pids[i] != 0) {
       i++;
    }
       
    return i; 
}

/*
 * changeDir(char *dirPath)
 * --------------------
 * Changes directory when 'cd' command is entered
 *
 */
void changeDir(char *dirPath) {
    if(chdir(dirPath) == -1){
        perror("Error: ");
        fflush(stdout);
    }
}

/*
 * handleCd(char *cmdArgs[])
 * --------------------
 * Grabs the 'cd' and its arguments and calls changeDir()
 *
 */
void handleCd(char *cmdArgs[]) {
    char *path;
    if(cmdArgs[1] == NULL)
        path = getenv("HOME");
    else
        path = cmdArgs[1];

    changeDir(path);
}

/*
 * exitPrgm(int *pids)
 * --------------------
 * Exits the program and kills any remaining background processes
 *
 */
void exitPrgm(int *pids) {
    int i = 0;
    while(pids[i] != 0) {
        if(pids[i] != -2){
        int killValue = kill(pids[i], SIGKILL);
            if(killValue == -1 && errno == ESRCH)
            	continue;
       	    else{
           	 perror("ERROR: ");
           	 fflush(stdout);
           	 exit(EXIT_FAILURE);
       	    }
	}
        i++;
    }

    exit(EXIT_SUCCESS);
}

/*
 * showStatus(int *status)
 * --------------------
 * Shows the exit status of the last foreground process ran.
 *
 */
void showStatus(int *status) {
    printf("exit value %d\n", *status);
    fflush(stdout);
}

/*
 * isThreeCmds(char *cmdArgs[], int *pids, int *exitStatus)
 * --------------------
 * Checks if command is 'cd', 'status' or 'exit' and calls the appropriate functions to handle the commands if true.
 *
 */
int isThreeCmds(char *cmdArgs[], int *pids, int *exitStatus) {
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

/*
 * clearArgs(char *args[])
 * --------------------
 * All arguments are removed from the argument array to have it clean and ready for next command.
 *
 */
void clearArgs(char *args[]) {
    int i = 0;
    while(args[i] != NULL) {
        args[i] = NULL;
        i++;
    }
}

/*
 * copyToExec(char *execArgs[], char *cmdArgs[], int length)
 * --------------------
 * Copies arguments of a command into execArgs[] to be used in execvp()
 *
 */
void copyToExec(char *execArgs[], char *cmdArgs[], int length) {
    for(int i = 0; i < length; i++) {
        execArgs[i] = cmdArgs[i];
    }
    execArgs[length] = NULL;
}

/*
 * isBackground(char *cmdArgs[], int lastArg)
 * --------------------
 * Checks if command contains '&' at the end.
 * Returns 1 if true and 0 if false
 *
 */
int isBackground(char *cmdArgs[], int lastArg) {
    lastArg--;
    if(strcmp(cmdArgs[lastArg], "&") == 0){
        cmdArgs[lastArg] = NULL;
        return 1;
    }

    return 0;
}

/*
 * backgroundInput(char *input, int background)
 * --------------------
 * Sets input file path to '/dev/null' if IO redirection is requied, background mode enabled, and no input specified.
 *
 */
char * backgroundInput(char *input, int background) {
    char *defaultName = "/dev/null";
    if(background && input == NULL)
        return defaultName;

    return NULL;
}

/*
 * backgroundOutput(char *input, int background)
 * --------------------
 * Sets output file path to '/dev/null' if IO redirection is requied, background mode enabled, and no output specified.
 *
 */
char * backgroundOutput(char *output, int background) {
    char *defaultName = "/dev/null";
    if(background && output == NULL)
        return defaultName;

    return NULL;
}

/*
 * isRedirect(char *cmdArgs[], int length, int background)
 * --------------------
 * Loops through argument array to determine whether or not file redirection will be needed.
 * Returns 1 if true and 0 if false.
 *
 */
int isRedirect(char *cmdArgs[], int length, int background) {
    if(background)
        length--;
    for(int i = 0; i < length; i++) {
        if(strcmp(cmdArgs[i], "<") == 0 || strcmp(cmdArgs[i], ">") == 0)
            return 1;
    }

    return 0;
}

/*
 * getIO(char *cmdArgs[], char *symbol)
 * --------------------
 * Returns input or output needed for IO redirection.
 * 
 * 
 */
char * getIO(char *cmdArgs[], char *symbol) {
    int i = 0; 
    while(cmdArgs[i] != NULL) {
        if(strcmp(cmdArgs[i], symbol) == 0){
	        return cmdArgs[++i];
	    }
        i++;
    }

    return NULL;
}

/*
 * redirectIO(char *input, char*output)
 * --------------------
 * Redirects stdin and stdout.
 * 
 * 
 */
void redirectIO(char *input, char*output){
   	int fdStatus;
    //use dup2 to redirect input
    if(input != NULL){
	    int inputFD = open(input, O_RDONLY);
	    if (inputFD == -1) {
	        perror("open()");
            exit(EXIT_FAILURE);
	    }
    	fdStatus = dup2(inputFD, 0);

        if (fdStatus == -1) {
            perror("error: ");
        }		
	}

	// Use dup2 to redirect output
	if(output != NULL){
        int outputFD = open(output, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        if (outputFD == -1) {
            perror("dup2"); 
            exit(EXIT_FAILURE);
        }
            fdStatus = dup2(outputFD, 1);
            if (fdStatus == -1) {
            perror("error: ");
        }
	}
}

/*
 * modifyArgsIO(char *cmdArgs[], int length)
 * --------------------
 * All arguments are removed from the argument array except for the initial command.
 * ex: [ls, >, junk] ==> [ls]
 * Note: only used if IO redirection is needed.
 *
 */
void modifyArgsIO(char *cmdArgs[], int length) {
    for(int i = 1; i < length; i++) {
        cmdArgs[i] = NULL;
    }
}

/*
 * checkProcesses(int *pids)
 * --------------------
 * Loops through array of background pids and prints exit status if a background process is done.
 *
 */
void checkProcesses(int *pids) {
    pid_t bgPid = -1;
    int bgExitStatus;
    int i = 0;
    while(pids[i] != 0) {

        if(pids[i] != -2 && waitpid(pids[i], &bgExitStatus, WNOHANG) > 0) {
            if(WIFSIGNALED(bgExitStatus)) {
                printf("background pid terminated is %d\n", pids[i]);
                fflush(stdout);
                printf("terminated by signal %d\n", WTERMSIG(bgExitStatus));
                fflush(stdout);
            }
            if(WIFEXITED(bgExitStatus)){
              printf("background pid %d is done: exit value %d\n", pids[i], WEXITSTATUS(bgExitStatus));
              fflush(stdout);
            }

            //set completed processes to -2
           pids[i] = -2;
        }
        i++;
    }
}

/*
 * forkCmds(char *cmdArgs[], int *pids, int *exitStatus, struct sigaction SIGINT_action, struct sigaction SIGTSTP_action)
 * --------------------
 * Handles all other commands that are not 'cd', 'exit', or 'status' by forking a new process and callinge execvp().
 *
 */
void forkCmds(char *cmdArgs[], int *pids, int *exitStatus, struct sigaction SIGINT_action, struct sigaction SIGTSTP_action) {
    int length = findLength(cmdArgs);
    int numOfPids = findNumOfPids(pids);
    int bckgrndMode = isBackground(cmdArgs, length);
    int childStatus, redirect;
    char *execArgs [length];
    char *outputFile, *inputFile;

    //Set up Redirection if needed
    redirect = isRedirect(cmdArgs, length, bckgrndMode);
    if(redirect){
        inputFile = getIO(cmdArgs, "<");
        outputFile = getIO(cmdArgs, ">");
        modifyArgsIO(cmdArgs, length);
    }
    if(redirect && bckgrndMode){
        inputFile = backgroundInput(inputFile, bckgrndMode);
        outputFile = backgroundInput(outputFile, bckgrndMode);
    }

    //execArgs will only the arguments
    copyToExec(execArgs, cmdArgs, length);

	// Fork a new process
	pid_t spawnPid = fork();
	switch(spawnPid){
        case -1:
            perror("fork()\n");
            exit(1);
            break;
        case 0:
            //Childs Process

            //Reset signal handlers for child
            sigfillset(&SIGINT_action.sa_mask);
            SIGINT_action.sa_flags = 0;
            SIGINT_action.sa_handler = SIG_DFL; 

            sigfillset(&SIGTSTP_action.sa_mask);
            SIGTSTP_action.sa_flags = 0;
            SIGTSTP_action.sa_handler = SIG_IGN;

            sigaction(SIGINT, &SIGINT_action, NULL);
            sigaction(SIGTSTP, &SIGTSTP_action, NULL);

            if(redirect)
                redirectIO(inputFile, outputFile);
            *exitStatus = execvp(execArgs[0], execArgs);
            if(*exitStatus == -1){
                perror("execvp: ");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            // In the parent process
            if(bckgrndMode && foregroundMode == -1){
	            pids[numOfPids] = spawnPid;
                printf("background pid is %d\n", spawnPid);
                fflush(stdout);
                //do not wait for child in background
                spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
            } 
            else{
                //wait for child to finish in foreground
                spawnPid = waitpid(spawnPid, &childStatus, 0);
                if(WIFEXITED(childStatus))
                    *exitStatus = WIFEXITED(childStatus);
                if(WTERMSIG(childStatus))
                    printf("terminated by signal %d\n", WTERMSIG(childStatus));
                fflush(stdout);
            }
            break;
	} 
    
}

/*
 * createCmdLine()
 * --------------------
 * Runs the command line and promps user for commands.
 *
 */
void createCmdLine() {
    char cmd [2049];
    char *args [513] = {NULL};
    int exit = 0;
    int pidArr [200] = {0};
    int exitStatus = 0;

    //Set up signal handlers
    struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};

    SIGINT_action.sa_flags = 0;
    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    sigaction(SIGINT, &SIGINT_action, NULL);

    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    do{
        printf(": ");
        fgets(cmd, 2049, stdin);
        fflush(stdout);

        expandVar(cmd);

        //check for blank lines or comments
        if(ignoreCmd(cmd))
            continue;

        parseCmd(cmd, args);

        if(!isThreeCmds(args, pidArr, &exitStatus)) {
            forkCmds(args, pidArr, &exitStatus, SIGINT_action, SIGTSTP_action);
        }

        clearArgs(args);

        checkProcesses(pidArr);

    }while(exit < 1);

}

int main()
{

    createCmdLine();

    return EXIT_SUCCESS;
}
