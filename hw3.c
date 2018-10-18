#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "signal.h"
#include <setjmp.h>

static jmp_buf jumper;

void sigint_handler(int signal) {
        longjmp(jumper, 42);
}

void sigtstp_handler(int signal) {
        longjmp(jumper, 30);
}

void siginChildKill(int signal){
        kill(getpid(), SIGINT);
}

void sigtstpChildKill(int signal){
        kill(getpid(), SIGTSTP);
}


char** initArray(){
        int j;
        char** argsarray;
        argsarray = (char** )malloc(sizeof(char* ) * 20);
        for(j = 0; j < 20; j++) {
                argsarray[j] = (char* )malloc(sizeof(char) * 100);
        }
        return argsarray;
}


void deleteArray(char **argsarray){
        int j;
        for(j = 0; j < 20; j++) {
                if (argsarray[j] != NULL) {
                        free (argsarray[j]);
                }
        }
        free(argsarray);
}


void execFunction(char **argsarray, int redir, char *fileName){
        int statusChild;
        pid_t pid = fork();
        int fileCreate;
        printf("%d", redir);

        if(pid == 0) { //if it is child

                printf("%d", redir);
                if (redir == 2) {
                        fileCreate = open(fileName, O_TRUNC | O_RDWR | O_CREAT, 0777);
                        if (fileCreate < 0) {
                                perror("Error, check STDOUT_file.");
                        }
                        dup2(fileCreate, STDOUT_FILENO);
                }
                if (redir == 1) {
                        fileCreate = open(fileName, O_RDONLY);
                        if (fileCreate < 0) {
                                perror("Error, check STDIN_file.");
                        }
                        dup2(fileCreate, STDIN_FILENO);
                        close(fileCreate);
                }
                signal(SIGINT, siginChildKill);
                signal(SIGTSTP, sigtstpChildKill);
                execvp(argsarray[0], argsarray);
                close(fileCreate);
                exit(0);
        }else{  //parent
                wait(&statusChild);
                printf("pid:%d status:%d\n", pid, WEXITSTATUS(statusChild));
        }
}

int redirArray(char **argsarray, char *line, char *fileName){

        int i = 0;
        int isINorOut = 0; // 1 = in, 2 = out

        char *word = strtok(line, " ");
        while (word) {
                if(strcmp(word, "<") == 0 || strcmp(word, ">") == 0) {
                        if(strcmp(word, "<") == 0) {
                                isINorOut = 1;
                        }
                        else if(strcmp(word, ">") == 0) {
                                isINorOut = 2;
                        }
                        word = strtok(NULL, " ");
                        if (word == NULL) {
                                return -1;
                        }
                        else{
                                strcpy(fileName, word);
                        }
                }else if(strcmp(word, ";") == 0) {
                        for(; i < 20; i++) {
                                free (argsarray[i]);
                                argsarray[i] = NULL;
                        }
                        execFunction(argsarray, isINorOut, fileName);
                        isINorOut = 0;
                        deleteArray(argsarray);
                        argsarray = initArray();
                        i = 0;
                }
                else{
                        strcpy(argsarray[i], word);
                        i = i + 1;
                }
                word = strtok(NULL, " ");

        }
        for(; i < 20; i++) {
                free (argsarray[i]);
                argsarray[i] = NULL;
        }
        return isINorOut;
}

int main(int argc, char **argv){

        char line[500];
        char** argsarray;
        char fileName[100];
        int redir;

        signal(SIGINT, sigint_handler);
        signal(SIGTSTP, sigtstp_handler);

        while(1) {
                switch (setjmp(jumper)) {
                case 42:
                        printf("\ncaught sigint\n");
                        break;

                case 30:
                        printf("\ncaught sigstp\n");
                        break;

                default:
                        break;
                }

                printf("CS361 > ");
                fgets(line, 500, stdin);
                if(strncmp(line, "exit", (strlen(line)-1))==0) break;
                if(line[strlen(line)-1] == '\n') {
                        line[strlen(line)-1] = '\0';
                }

                argsarray = initArray();
                redir = redirArray(argsarray, line, fileName);
                fflush(stdout);
                execFunction(argsarray, redir, fileName);
                deleteArray(argsarray);
        }
}
