//
//  main.c
//  smallsh
//
//  Created by Michael Bamesberger on 2/23/16.
//  Copyright (c) 2016 Michael Bamesberger. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LENGTH 2048

int beginShell(char *homeVar);
void handler(int signum);
char *strip_dots(const char *s);
char *strip_copy(const char *s);


int main(int argc, const char * argv[], char *envp[]) {
    
    pid_t waitOutput;
    int exterm = -15;
    int status;
    char *home;
    char homeVar[100];
    home = getenv("HOME");
    strcpy(homeVar, home);

    struct sigaction act, childCatch;
    act.sa_handler = SIG_IGN;
    childCatch.sa_handler= SIG_DFL;
    sigaction(SIGINT, &act, NULL);
    
    
    int exitFlag = 0;
    while (exitFlag != 1){
    
        
        
        int exstatus;
        int isBackground = 0;
        char inputString[MAX_LENGTH];
        char inputArray[512][MAX_LENGTH];
        char * pch;
        for (int x = 0; x < 512; x++){
            strcpy(inputArray[x], "\0");
        }
        printf("\n: ");
        fflush(stdout);
        fgets(inputString, MAX_LENGTH, stdin);
        int count = 0;
        
        pch = strtok(inputString, " ");
        while (pch != NULL){
            
            strcpy(inputArray[count], pch);
            pch = strtok(NULL, " ");
            count++;
        }
        
        
        
        while((waitOutput=waitpid(-1,&status,WNOHANG))>0)
        {
            printf("background process: %d finished.\n", waitOutput);
            if(WIFEXITED(status))
            {
                printf("Exit Status: %d\n",WEXITSTATUS(status));
                exstatus = WEXITSTATUS(status);
            }
            else if(WIFSIGNALED(status))
            {
                printf("Terminated by signal: %d\n",WTERMSIG(status));
                exterm = WTERMSIG(status);
            }
        }
       
        
        
//________________BUILT_IN_COMMANDS_____________________________
        
        if (strcmp(inputArray[count - 1], "&\n") == 0){
            
            isBackground = 1;
        }
        
        if (strcmp(inputArray[0], "exit\n") == 0){                              // EXIT COMMAND
            exitFlag = 1;
            break;
            
        }
        else if (strcmp(inputArray[0], "cd\n") == 0){                           // CD - NO ARGUMENT
            int ret;
            ret = chdir(homeVar);
            if (ret == -1){
                printf("Error. Could not change directory");
                return 1;
            }
        }
        
        else if (strcmp(inputArray[0], "cd") == 0){                             // CD - ARGUMENT
            if (strcmp(inputArray[1], "\0") == 0){
                printf("Error. Please try again");
                return 1;
            }
            
            else{
                int ret;
                char *pathString = strip_copy(inputArray[1]);
                char firstchar = pathString[0];
                char secondchar = pathString[1];
            
                char cwd[1024];
                        // Test if input is a relative path, which begins with dots
                if (firstchar == '.' && secondchar == '.'){
                    char *relativePath = strip_dots(pathString);
                    if (getcwd(cwd, sizeof(cwd)) != NULL){
                        strcat(cwd, relativePath);
                    }
                    else{
                        perror("getcwd() error");
                    }
                    
                }
                        // Otherwise path is absolute
                else{
                    strcpy(cwd, pathString);
                }
                
                ret = chdir(cwd);                                               // Changes the directory
                if (ret == -1){
                    printf("Error. Could not change directory");
                    return 1;
                }
                
            }
        }
        
        else if (strcmp(inputArray[0], "status\n") == 0){
            if (exterm == -15){
                printf("exit status %d\n", exstatus);
                fflush(stdout);
            }
            else if (exterm != -15){
                printf("exit terminated %d\n", exterm);
                fflush(stdout);
            }
        }
        
        else if (strcmp(inputArray[0], "#") == 0){                               // COMMENTS

        }
        
        else if (strcmp(inputArray[0], " \n") == 0){                               // BLANK SPACE
        }
        
    
        
 //______________________OTHER_COMMANDS______________________________________________
        
        else
        {
            pid_t pid;
            int status;
            pid = fork();
            if (pid == 0){
                sigaction(SIGINT, &childCatch, NULL);
                if (isBackground == 1){
                    
                    char* digTest = strip_copy(inputArray[1]);
                    
                    if ((digTest[0]>=48 && digTest[0]<=57)){
                        
                        if (execlp(inputArray[0], inputArray[0], digTest, NULL) == -1){
                            printf("Error. Call to %s could not be made.\n", inputArray[0]);
                            return 1;
                        }
                        
                    
                    }
                    // Background Child
                    int fd = open("/dev/null", O_RDWR, 0);
                    if (fd == -1){
                        printf("/dev/null could not be opened");
                        fflush(stdout);
                        return 1;
                    }
                    int fd2 = dup2(fd, 0);
                    if (fd2 == -1){
                        printf("/dev/null could not be opened");
                        fflush(stdout);
                        return 1;
                    }
                    printf("The background pid is %d\n", getpid());
                    if (execlp(inputArray[0], inputArray[0], NULL) == -1){
                        
                        printf("Error. Call to %s could not be made.\n", inputArray[0]);
                        return 1;
                    }
                    
                }
                
                if (strcmp(inputArray[1], ">") == 0){                              // HANDLES > REDIRECTION
                    char *filetoOpen = strip_copy(inputArray[2]);
                    int fd = open(filetoOpen, O_WRONLY|O_CREAT|O_TRUNC, 0644);
                    if (fd == -1)
                    {
                        printf("Error. File %s could not be opened.\n", inputArray[2]);
                        fflush(stdout);
                        return 1;
                    }
                    
                    int fd2 = dup2(fd, 1);
                    close(fd);
                    if (fd2 == -1)
                    {
                        printf("Error. File %s could not be opened.\n", inputArray[2]);
                        fflush(stdout);
                        return 1;
                    }
                    
                    if (execlp(inputArray[0], inputArray[0], NULL) == -1){
                        
                        printf("Error. Call to %s could not be made.\n", inputArray[0]);
                        return 1;
                    }
                }
                
                else if (strcmp(inputArray[1], "<") == 0){                          // HANDLES < REDIRECTION
                    char *filetoOpen = strip_copy(inputArray[2]);
                    int fd = open(filetoOpen, O_RDONLY);
                    if (fd == -1)
                    {
                        printf("Error. File %s could not be opened.\n", inputArray[2]);
                        return 1;
                    }
                    
                    int fd2 = dup2(fd, 0);
                    close(fd);
                    if (fd2 == -1)
                    {
                        printf("Error. File %s could not be opened.\n", inputArray[2]);
                        return 1;
                    }
                    
                    if (execlp(inputArray[0], inputArray[0], NULL) == -1){
                        printf("Error. Call to %s could not be made.\n", inputArray[0]);
                        return 1;
                    }
                }
                
                else if (strcmp(inputArray[1], "\0") == 0){    // SINGLE COMMANDS

                    char *pathString = strip_copy(inputArray[0]);
                    if (execlp(pathString, pathString, NULL) == -1)
                    {
                        perror("Invalid command");
                        return 1;
                    }
                }

                else {
                    //SECOND ARGUMENT IS NOT REDIRECTION
                    char* digTest = strip_copy(inputArray[1]);
                
                    if (digTest[0]>=48 && digTest[0]<=57){
                        if (execlp(inputArray[0], inputArray[0], digTest, NULL) == -1){
                            printf("Error. Call to %s could not be made.\n", inputArray[0]);
                            return 1;
                        }
                        
                    }
                    char *filetoOpen = strip_copy(inputArray[1]);
                    int fd = open(filetoOpen, O_RDONLY);
                    if (fd == -1){
                        printf("Error. File %s could not be opened.\n", inputArray[1]);
                        return 1;
                    }
                    
                    int fd2 = dup2(fd, 0);
                    close(fd);

                    if (fd2 == -1){
                        printf("Error. File %s could not be opened.\n", inputArray[1]);
                        return 1;
                    }
                    
                    if (execlp(inputArray[0], inputArray[0], NULL) == -1){
                        printf("Error. Call to %s could not be made.\n", inputArray[0]);
                        return 1;
                    }
                    //return 0;
                }
            } // end of (if (pid == 0))
            
            else if (pid < 0)                                                       // FORK ERROR
            {
                printf("There was an error in the forking process");
                return 1;
            }
            
            else{                                                                   // PARENT PROCESS
                
                if(isBackground == 0){
                    
                    
                        waitpid(pid, &status, WUNTRACED);
                    if(WIFEXITED(status))
                    {
                        exstatus = WEXITSTATUS(status);
                        exterm = -15;
                    }
                        
                    else if(WIFSIGNALED(status))
                    {
                        exterm = WTERMSIG(status);
                        printf("Terminated by signal: %d\n",exterm);
                    
                    }
                    
                }
                
                if(isBackground == 1){
                    
                    printf("The background process is %d\n", pid);
                

                
                }
                
            }
        }
        
        
    }

}


char *strip_copy(const char *s) {
    char *p = malloc(strlen(s) + 1);
    if(p) {
        char *p2 = p;
        while(*s != '\0') {
            if(*s != '\t' && *s != '\n') {
                *p2++ = *s++;
            } else {
                ++s;
            }
        }
        *p2 = '\0';
    }
    return p;
}

char *strip_dots(const char *s) {
    char *p = malloc(strlen(s) + 1);
    if(p) {
        char *p2 = p;
        while(*s != '\0') {
            if(*s != '.') {
                *p2++ = *s++;
            } else {
                ++s;
            }
        }
        *p2 = '\0';
    }
    return p;
}

