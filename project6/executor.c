/* Name: Ryan Henrikson */
/* Directory ID: rhenriks */
/* id number: 119011164 */
#include <stdio.h>
#include "command.h"
#include "executor.h"
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include<unistd.h>
#include<sys/wait.h>
#define OPEN_FLAGS (O_WRONLY | O_TRUNC | O_CREAT)
#define DEF_MODE 0664


void input_output_rdirection(struct tree * t);
int execute_aux(struct tree * t, int input, int output);
/* static void print_tree(struct tree *t); */

int child_right_pipe(struct tree * t, int fd[2]);
int child_left_pipe(struct tree * t, int fd[2]);

int execute(struct tree *t) {
  return execute_aux(t, STDIN_FILENO, STDOUT_FILENO);

}

int execute_aux(struct tree * t, int input, int output){

 if(t == NULL){
   return 0;
   }
   /* Conjunction == NONE first */
   /* Exit and CD first. */

if(t->conjunction == NONE){
   if(!strcmp(t->argv[0], "exit")){
      exit(0);
   }
   else if(!strcmp(t->argv[0], "cd")){
      if(t->argv[1] == NULL){
         t->argv[1] = getenv("HOME");
      }
      if(chdir(t->argv[1]) != 0){
         perror("Something happened while changing directories.");
      }

   }
   else{
      int pid = 0;
      if((pid = fork()) > 0){      /* Parent */
         int status = 0;
         waitpid(pid, &status, 0);
         if(WIFEXITED(status)){
            return WEXITSTATUS(status);
         }
         else{
            return WIFEXITED(status);
         }
         
      }
      else if(pid < 0){    /* Fork Error */
         perror("Fork failed.");
         exit(EX_OSERR);
      }
      else{       /* Child */
         input_output_rdirection(t);
         execvp(t->argv[0], t->argv);
         fprintf(stderr, "Failed to execute %s\n", t->argv[0]);
         fflush(stdout);
         exit(EX_OSERR);

      }
   }
}
/* And tree */

else if(t->conjunction == AND){
   if(execute_aux(t->left, input, output) == 0){
      return execute_aux(t->right, input, output);
   }
   return -1;
}
else if (t->conjunction == SUBSHELL){
   int pid = 0, status = 0;
   if((pid = fork()) > 0){       /* Parent */
      waitpid(pid, &status, 0);
      if(WIFEXITED(status)){
            return WEXITSTATUS(status);
         }
         else{
            return WIFEXITED(status);
         }
   }
   else if(pid < 0){
      perror("Fork Failed In Subshell.");
      exit(EX_OSERR);
   }
   else{
      
      input_output_rdirection(t);
      exit(execute_aux(t->left, input, output));
   }
}

else if(t->conjunction == PIPE){          /* Now Pipe */

   int fd[2];
   int pid1 = 0;
   int pid2 = 0;
   int status1 = 0;
   int status2 = 0;
   if(t->right && t->right->input != NULL){       /* Ambiguous stuff */

      fprintf(stdout, "Ambiguous input redirect.\n");
      return -1;
   }
   
   else if( t->left && t->left->output != NULL ){

      fprintf(stdout, "Ambiguous output redirect.\n");
      return -1;
   }
   
   if(pipe(fd) < 0){
      perror("Pipe fail");
      exit(EX_OSERR);
   }
   pid1 = child_left_pipe(t, fd);      /* Run aux methods */
   pid2 = child_right_pipe(t, fd);
        if (close(fd[0]) != 0) {
            perror("Closing Input Error.");
            fflush(stdout);
            exit(EX_OSERR);
        }        /* Close parent pipe */
        if (close(fd[1]) != 0) {
            perror("Closing Input Error.");
            fflush(stdout);
            exit(EX_OSERR);
        }

   waitpid(pid1, &status1, 0);      /* Wait for children. */
   waitpid(pid2, &status2, 0);

   

   if (WIFEXITED(status1) && WIFEXITED(status2)) {
      return WEXITSTATUS(status2); 
   } else {
      return -1;
   }
   
   
}
return 0;
}

int child_left_pipe(struct tree * t, int fd[2]) {
    int pid = fork();
    if (pid < 0) {
        perror("Fork failed for left child");
        exit(EX_OSERR);
    } else if (pid == 0) { 
        if (close(fd[0]) != 0) {
            perror("Closing Input Error.");
            fflush(stdout);
            exit(EX_OSERR);
        }
        if (dup2(fd[1], STDOUT_FILENO) < 0) {
            perror("dup2 failed for left child");
            exit(EX_OSERR);
        }
        if (close(fd[1]) != 0) {
            perror("Closing Input Error.");
            fflush(stdout);
            exit(EX_OSERR);
        }
        execute_aux(t->left, STDIN_FILENO, STDOUT_FILENO);
        exit(0);
    }
    return pid; /* child pid */
}




int child_right_pipe(struct tree * t, int fd[2]) {
     int pid = fork();
    if (pid < 0) {
        perror("fork failed for right child");
        exit(EX_OSERR);
    } else if (pid == 0) { 
        if (close(fd[1]) != 0) {
            perror("Closing Input Error.");
            fflush(stdout);
            exit(EX_OSERR);
        }
        if (dup2(fd[0], STDIN_FILENO) < 0) {
            perror("dup2 failed for right child");
            exit(EX_OSERR);
        }
        if (close(fd[0]) != 0) {
            perror("Closing Input Error.");
            fflush(stdout);
            exit(EX_OSERR);
        }
        execute_aux(t->right, STDIN_FILENO, STDOUT_FILENO);
        exit(0);
    }
    return pid;      /* child pid */
}



void input_output_rdirection(struct tree * t){
   /* Reading from input */
   if(t->input != NULL){
      int fd = open(t->input, O_RDONLY);
      if(fd < 0){
         perror("error opening input.");
         fflush(stdout);
         exit(EX_OSERR);
      }
      if(dup2(fd, STDIN_FILENO) < 0){
         perror("Dup 2 error.");
         fflush(stdout);
         exit(EX_OSERR);
      }
      if(close(fd) != 0){
         perror("Closing Input Error.");
         fflush(stdout);
         exit(EX_OSERR);
      }
   }
   if(t->output != NULL){
      int fd = open(t->output, OPEN_FLAGS, DEF_MODE);
      if(fd < 0){
         perror("error opening output redirect.");
         fflush(stdout);
         exit(EX_OSERR);
      }
      if(dup2(fd, STDOUT_FILENO) < 0){
         perror("Dup 2 error.");
         fflush(stdout);
         exit(EX_OSERR);
      }
      if(close(fd) != 0){
         perror("Closing Input Error.");
         fflush(stdout);
         exit(EX_OSERR);
      }
   }
   return;
}

/*
static void print_tree(struct tree *t) {
   if (t != NULL) {
      print_tree(t->left);

      if (t->conjunction == NONE) {
         printf("NONE: %s, ", t->argv[0]);
      } else {
         printf("%s, ", conj[t->conjunction]);
      }
      printf("IR: %s, ", t->input);
      printf("OR: %s\n", t->output);

      print_tree(t->right);
   }
}


*/
