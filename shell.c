#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <sys/wait.h>
#include  <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#define MAX_LINE 80 /* The maximum length command */
/* Convert command to argv list */
/* Get input, output, pipe mark position from the command */ 
void  parse(char *line, char **argv, char** output, char** input, char*** pipePos)
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
               /*Remove output filename from then argv */
          }
          else 
          if (strcmp(curLine, "<")==0)/* Has redirected input           */
          {
               get = 2;
               *argv--;
               /*Remove input filename from then argv */
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
          
          if(strcmp(curLine,"|")==0)
               *pipePos = argv - 1; 
     }
     *argv = '\0';                /* The end of the argv is \0*/
}
/* Execute the comand */     
void  execute(char **argv,char* output, char* input, char** pipePos)
{
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
                    if (execvp(*argv, argv) < 0) {     /* execute the command  */
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
               /* Devide argv list into 2 argv list for each child */
               *pipePos = '\0';
               char** argvPipe = pipePos +1;
               pid_t childPid = fork();
               if (childPid == 0)
               {
                    //Child-child process
                    close(fd[0]);
                    //Redirected output to fd[0]
                    dup2(fd[1], 1);
                    if (execvp(*argv, argv) < 0) {     /* execute the command  in child's child*/
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
                    if (execvp(*argvPipe, argvPipe) < 0) {     /* execute the command in child */
                         printf("*** ERROR: exec failed\n");
                         exit(1);
                    }

               }
          }
     }
     else {                                  /* parent process      */
          while (wait(&status) != pid)       /* wait for child completion  */
               ;
     }
}
     
void  main(void)
{
     char  line[MAX_LINE];             /* the input command                 */
     char  *argv[MAX_LINE/2 +1];              /* the command -> argument      */
     char backupLine[MAX_LINE];
     int backup = 0;
     while (1) { 
          /* repeat until done ....         */
          char** pipePos;
          char* input;
          char* output;
          input = output = NULL;
          pipePos = NULL;                  
          printf("osh> ");     /*   display prompt             */
          fgets(line, MAX_LINE, stdin);       /*   read the command    */
          line[strlen(line)-1] = 0; /* remove LF char */
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
          parse(line, argv, &output, &input, &pipePos);       /*   parse the line to argv               */
          if (strcmp(argv[0], "exit") == 0)  /* if user type exit then exit     */
               exit(0);            
          execute(argv,output,input,pipePos);           /* execute command */
     }
}