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
void INThandler(int);
/* When non-zero, this global means the user is done using this program. */
int done = 0;
pid_t rPid; //Currently running pid

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

  // Redirects standard SIGINT signal to use function INThandler
  signal(SIGINT, INThandler);

  while (!done) {
    waitpid(-1, 0, WNOHANG);

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

        if(isEqual(*(cmd.pgm->pgmlist), "exit")){ // Received command exit, closes lsh
          exit(0);
        }else if(isEqual(*(cmd.pgm->pgmlist), "cd")){ //If lsh received command cd, the directory will change accordingly
          if (cmd.pgm->pgmlist[1]){ // Change directory to the name following cd
            int i = chdir(cmd.pgm->pgmlist[1]);
            // If name following cd is not a directory, print error message
            if(i){
              printf("No such directory\n");
            }
          }else{ // If no directory was received, go to HOME (using enviromental variable)
            chdir(getenv("HOME"));
          } 
        }else{ // Run command through launch function 
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

/*
 * Name: isEqual
 *
 * Description: Compares two strings and return 1 if equal else 0
 *
 */
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

/*
 * Name: INThandler
 *
 * Description: redirect of signal recieved from CTRL-C
 * This will kill any ongoing parent process with its child
 *
 */
void  INThandler(int sig)
{
  if(rPid != 0) {
    kill(rPid, SIGINT);
    rPid = 0;
  }
}

/*
 * Name: launch
 *
 * Description: Launches parent and child processes to execute commands
 * This function can handle redirects of stdin, stdout and pipes
 * 
 *
 */
void
launch(Command cmd, int parentfd)
{
  Pgm * pgm = cmd.pgm;
  char **pgmlist = pgm->pgmlist;
  cmd.pgm = pgm->next;

  pid_t pid, wpid;
  int status;
  // Forks the process between parent and child
  pid = fork();

  if(pid == 0){ // Child process if pid equals 0
    if(cmd.rstdin && !cmd.pgm){ // If last command and redirect stdin 
      FILE * newin;
      newin = fopen(cmd.rstdin, "r");
      dup2(fileno(newin), 0);
      fclose(newin);
    }
    if(cmd.rstdout && parentfd == -1){ // If first command and redirect stdout
      // Creates a new file and redirects stdout to it
      FILE * new;
      new = fopen(cmd.rstdout, "w");
      dup2(fileno(new), 1);
      fclose(new);

    }else if(parentfd != -1){ // Got a pipe redirect stdout
      dup2(parentfd, 1);
      close(parentfd);
    }
    if(cmd.bakground){
      /* Changes pgid to make sure background processes are not terminated when receiving
         Signal Ctrl-C */
      setpgid(0 ,0);
    }


    if(cmd.pgm){ // If there are more commands
      int fd[2];
      if(pipe(fd) == -1){
        perror("pipe");
        return ;
      }
      // Launches the process that will write to pipe
      launch(cmd, fd[1]);
      close(fd[1]);
      // Set stdin to pipe read 
      dup2(fd[0], 0);
      close(fd[0]);
    }
    if(execvp(*pgmlist, pgmlist) == -1){
      // If error was received from execvp, print error and call exit
      perror("lsh");
      exit(status);
    }
 }else if(pid < 0){ //error
    perror("Error");
 }else if(!cmd.bakground){ // Parent that is not a background process
    // If not background, pid is saved so we can kill it when receiving Ctrl-C signal
    rPid = pid;
    // Parent will wait for child
    wpid = waitpid(pid, &status, 0);
    rPid = 0;
    
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
