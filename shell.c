#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <sys/wait.h>
#include  <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#define MAX_LINE 80 /* The maximum length command */
/* Convert command to args list */
/* Get input, output, pipe mark position from the command */ 
void  parse(char *line, char **args, char** output, char** input, char*** pipePos)
{
     int get = 0;
     char* curLine;
     /* remove space on the head of line */
     while (*line == ' ' || *line == '\t' || *line == '\n')
          line++;

     while (*line != '\0') {       /* if not the end of line ....... */ 
          *args++ = line;          /* save the argument position*/
          curLine = line;
          while (*line != '\0' && *line != ' ' && 
                 *line != '\t' && *line != '\n') 
               *line++;             /* skip the argument until ...    */

          while (*line == ' ' || *line == '\t' || *line == '\n')
               *line++ = '\0';     /* replace white spaces with 0    */

          if (strcmp(curLine, ">")==0)/* Has redirected output          */
          {
               get = 1;
               *args--; 
               /*Remove output filename from then args */
          }
          else 
          if (strcmp(curLine, "<")==0)/* Has redirected input           */
          {
               get = 2;
               *args--;
               /*Remove input filename from then args */
          }
          else if(get)             /* Get this args to redirected    */
          {
               if (get == 1)       /* Get this args to output        */
                    *output = curLine;
               if (get == 2)       /* Get this args to input         */
                    *input = curLine;
               get = 0;
               *args--;
          }
          
          if(strcmp(curLine,"|")==0)
               *pipePos = args - 1; 
     }
     *args = '\0';                /* The end of the args is \0*/
}
/* Execute the comand */     
void  execute(char **args,char* output, char* input, char** pipePos, int waitForChild)
{
     // Check for & in the end of command
     pid_t  pid;
     int    status;
     pid = fork(); /*fork a process*/
     if (pid < 0) {     /* fork failed           */
          printf("*** ERROR: forking child process failed\n");
          exit(1);
     }
     else if (pid == 0) {          /* for the child process:         */
          //Redirected output
          if(output)
          {
               int fd = open(output, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
               dup2(fd,1);
          }
          else
          //Redirected input
          if(input){
               int fd = open(input, O_RDONLY);
               dup2(fd,0);
          }
          //If there is not a pipe mark then execute the command
          if (!pipePos)
               {
                    if (execvp(*args, args) < 0) {     /* execute the command  */
                         printf("*** ERROR: exec failed\n");
                         exit(1);
                    }
               }
          else
          {
               int fd[2];
               if(pipe(fd)<0)
                    {
                         printf("Pipe filed\n");
                         exit(1);
                    }
               /* Devide args list into 2 args list for each child */
               *pipePos = '\0';
               char** argsPipe = pipePos +1;
               pid_t childPid = fork();
               if (childPid == 0)
               {
                    //Child-child process
                    close(fd[0]);
                    //Redirected output to fd[0]
                    dup2(fd[1], 1);
                    if (execvp(*args, args) < 0) {     /* execute the command  in child's child*/
                         printf("*** ERROR: exec failed\n");
                         exit(1);
                    }
               } 
               else
               {
                    //Child process
                    int childStatus;
                    //Redirected input to fd[1]
                    close(fd[1]);
                    while (wait(&status) != childPid)       /* wait for completion  */
                         ;
                    dup2(fd[0],0);
                    if (execvp(*argsPipe, argsPipe) < 0) {     /* execute the command in child */
                         printf("*** ERROR: exec failed\n");
                         exit(1);
                    }

               }
          }
     }
     else {                                   /* parent process      */
          if(waitForChild)
          while (wait(&status) != pid);       /* wait for child completion  */
     }
}
     
void  main(void)
{
     char  line[MAX_LINE];             /* the input command                 */
     char  *args[MAX_LINE/2 +1];              /* the command -> argument      */
     char backupLine[MAX_LINE];
     int backup = 0;
     while (1) { 
          /* repeat until done ....         */
          char** pipePos;
          char* input;
          char* output;
          int waitForChild;
          input = output = NULL;
          pipePos = NULL;      
          waitForChild = 1;            
          printf("osh> ");     /*   display prompt             */
          fgets(line, MAX_LINE, stdin);       /*   read the command    */
          line[strlen(line)-1] = 0; /* remove LF char */
          /*Check for & in the end */
          if(line[strlen(line)-1] == '&') {
               waitForChild = 0; /*Not wait for Child */
               line[strlen(line)-1]=' '; /*Remove & from the command */
               }
          if(strlen(line)==0) continue;/*Empty command*/
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
          parse(line, args, &output, &input, &pipePos);       /*   parse the line to args               */
          if (strcmp(args[0], "exit") == 0)  /* if user type exit then exit     */
               exit(0);            
          execute(args,output,input,pipePos,waitForChild);           /* execute command */
     }
}