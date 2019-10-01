/* ----------------------------------------------------------------- */
/* PROGRAM  shell.c                                                  */
/*    This program reads in an input line, parses the input line     */
/* into tokens, and use execvp() to execute the command.             */
/* ----------------------------------------------------------------- */

#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <sys/wait.h>
#define MAX_LINE 80 /* The maximum length command */
/* ----------------------------------------------------------------- */
/* FUNCTION  parse:                                                  */
/*    This function takes an input line and parse it into tokens.    */
/* It first replaces all white spaces with zeros until it hits a     */
/* non-white space character which indicates the beginning of an     */
/* argument.  It saves the address to argv[], and then skips all     */
/* non-white spaces which constitute the argument.                   */
/* ----------------------------------------------------------------- */

void  parse(char *line, char **argv, char** output, char** input)
{
     int get = 0;
     char* curLine;
     /* remove space on the head of line */
     while (*line == ' ' || *line == '\t' || *line == '\n')
          line++;

     while (*line != '\0') {       /* if not the end of line ....... */ 
          *argv++ = line;          /* save the argument position*/
          curLine = line;
          while (*line != '\0' && *line != ' ' && 
                 *line != '\t' && *line != '\n') 
               *line++;             /* skip the argument until ...    */

          while (*line == ' ' || *line == '\t' || *line == '\n')
               *line++ = '\0';     /* replace white spaces with 0    */

          if (strcmp(curLine, ">")==0)/* Has redirected output          */
          {
               get = 1;
               *argv--;
          }
          else 
          if (strcmp(curLine, "<")==0)/* Has redirected input           */
          {
               get = 2;
               *argv--;
          }
          else if(get)             /* Get this argv to redirected    */
          {
               if (get == 1)       /* Get this argv to output        */
                    *output = curLine;
               if (get == 2)       /* Get this argv to input         */
                    *input = curLine;
               get = 0;
               *argv--;
          }
          
          /* if use for redirected then remove this argv from list*/               

     }
     *argv = '\0';                 /* mark the end of argument list  */
}

/* ----------------------------------------------------------------- */
/* FUNCTION execute:                                                 */
/*    This function receives a commend line argument list with the   */
/* first one being a file name followed by its arguments.  Then,     */
/* this function forks a child process to execute the command using  */
/* system call execvp().                                             */
/* ----------------------------------------------------------------- */
     
void  execute(char **argv)
{
     pid_t  pid;
     int    status;
     pid = fork(); /*fork a process*/
     if (pid < 0) {     /* fork failed           */
          printf("*** ERROR: forking child process failed\n");
          exit(1);
     }
     else if (pid == 0) {          /* for the child process:         */
          if (execvp(*argv, argv) < 0) {     /* execute the command  */
               printf("*** ERROR: exec failed\n");
               exit(1);
          }
     }
     else {                                  /* for the parent:      */
          while (wait(&status) != pid)       /* wait for completion  */
               ;
     }
}

/* ----------------------------------------------------------------- */
/*                  The main program starts here                     */
/* ----------------------------------------------------------------- */
     
void  main(void)
{
     char  line[MAX_LINE];             /* the input line                 */
     char  *argv[MAX_LINE/2 +1];              /* the command line argument      */
     char backupLine[MAX_LINE];
     int backup = 0;
     while (1) { 
          /* repeat until done ....         */
          char* input;
          char* output;
          input = output = NULL;                  
          printf("osh> ");     /*   display a prompt             */
          fgets(line, MAX_LINE, stdin);       /*   read in the command line     */
          line[strlen(line)-1] = 0; /* remove LF char */
          /*Check history */
          if(strcmp(line,"!!")==0) /*use previous command*/
            if (backup)
                strcpy(line,backupLine);
            else 
                printf("No commands inhistory.");
          else 
            {
                strcpy(backupLine,line); /* if not then backup current command */
                backup = 1;
            }
          parse(line, argv, &output, &input);       /*   parse the line               */
          if(input) printf("%s\n", input);
          if(output) printf("%s\n", output);

          if (strcmp(argv[0], "exit") == 0)  /* is it an "exit"?     */
               exit(0);            /*   exit if it is                */
          execute(argv);           /* otherwise, execute the command */
     }
}