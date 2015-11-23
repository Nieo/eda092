/* 
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file 
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive: 
      $> tar cvf lab1.tgz lab1/
 *
 * All the best 
 */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"


/*
 * Function declarations
 */
void launch(Command, int);
void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);
int isEqual(char *, char *);
void changeDir(char *);
void INThandler(int);
/* When non-zero, this global means the user is done using this program. */
int done = 0;

/*
 * Name: main
 *
 * Description: Gets the ball rolling...
 *
 */
int main(void)
{
  Command cmd;
  int n;

  signal(SIGINT, INThandler);

  while (!done) {
    char *line;
    line = readline("> ");

    if (!line) {
      /* Encountered EOF at top level */
      done = 1;
    }
    else {
      /*
       * Remove leading and trailing whitespace from the line
       * Then, if there is anything left, add it to the history list
       * and execute it.
       */
      stripwhite(line);

      if(*line) {
        add_history(line);
        /* execute it */
        n = parse(line, &cmd);
        PrintCommand(n, &cmd);

        if(isEqual(*(cmd.pgm->pgmlist), "exit")){
          exit(0);
        }else if(isEqual(*(cmd.pgm->pgmlist), "cd")){
          changeDir(cmd.pgm->pgmlist[1]);
        }else{
          launch(cmd, -1); 
        }        
      }
    }

    if(line) {
      free(line);
    }
  }
  return 0;
}
//compares 2 string and return 1 if equal else 0
int
isEqual(char *s1, char *s2){
  while(*s1 && *s2){
    if(*s1++ != *s2++)
      return 0;
  }
  if(*s1 != *s2){
    return 0;  
  }
  return 1;
}
void
changeDir(char * destination){
  if(isEqual(destination, "..") || *destination == '/'){ 
    chdir(destination);
  }else{
    char *cwd =malloc(1024*sizeof(char));
    getcwd(cwd, sizeof(cwd));
    printf("Current wd  %s\n", cwd);
    char * p = cwd;
    while(*p){
      *p++;
    }
    *p++ = '/';
    while(*destination){
      *p++ = *destination++;
    }
    printf("change wd  %s\n", cwd);
    chdir(cwd);
    free(cwd);
  }
}

void  INThandler(int sig)
{
     //signal(sig, SIG_IGN);
     printf("You hit Ctrl-C");
}


void
launch(Command cmd, int parentfd)
{
  Pgm * pgm = cmd.pgm;
  char **pgmlist = pgm->pgmlist;
  cmd.pgm = pgm->next;

  pid_t pid, wpid;
  int status;
  pid = fork();

  if(pid == 0){//child
    if(cmd.rstdin && !cmd.pgm){ //If last command and redirect stdin 
      //printf("Redirecting stdin to %s\n", cmd.rstdin );
      FILE * newin;
      newin = fopen(cmd.rstdin, "r");
      dup2(fileno(newin), 0);
      fclose(newin);
    }
    if(cmd.rstdout && parentfd == -1){ //If first command and redirect stdout
      //printf("Redirecting stdout to %s\n", cmd.rstdout);
      FILE * new;
      new = fopen(cmd.rstdout, "w");
      dup2(fileno(new), 1);
      fclose(new);

    }else if(parentfd != -1){ //Got a pipe redirect stdout to pipe
      //printf("Redirecting to pipe\n");
      dup2(parentfd, 1);
    }


    if(cmd.pgm){ //If there are more commands
      int fd[2];
      if(pipe(fd) == -1){
        perror("pipe");
        return ;
      }
      //Set stdin to pipe read 
      dup2(fd[0], 0);
      close(fd[0]);
      launch(cmd, fd[1]);
      close(fd[1]);
    }
    if(cmd.bakground){
      printf("I cant hanlde &\n");
    }
    if(execvp(*pgmlist, pgmlist) == -1){
     perror("lsh");
    }
 }else if(pid < 0){ //error
    perror("Error");
 }else if(!cmd.bakground){ //parent
    wpid = waitpid(pid, &status, 0);
    
 }else{
   printf("%d\n", pid);
 }
}
/*
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void
PrintCommand (int n, Command *cmd)
{
  printf("Parse returned %d:\n", n);
  printf("   stdin : %s\n", cmd->rstdin  ? cmd->rstdin  : "<none>" );
  printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>" );
  printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
  PrintPgm(cmd->pgm);
}

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s
 *
 */
void
PrintPgm (Pgm *p)
{
  if (p == NULL) {
    return;
  }
  else {
    char **pl = p->pgmlist;
    
     /* The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("    [");
    while (*pl) {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}

/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void
stripwhite (char *string)
{
  register int i = 0;

  while (whitespace( string[i] )) {
    i++;
  }
  
  if (i) {
    strcpy (string, string + i);
  }

  i = strlen( string ) - 1;
  while (i> 0 && whitespace (string[i])) {
    i--;
  }

  string [++i] = '\0';
}
