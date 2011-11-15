#include "header.h"

int main(int argc, char* argv[])
{
  /* vars */
  stage* currComm;
  int endCheck;
  FILE* inFile = stdin;
  int pipes[MAX_COMMANDS - 1][2];
  int statuses[MAX_COMMANDS];
  pid_t children[MAX_COMMANDS];
  int i, j;
  int childInput;
  int childOutput;
  /*struct sigaction newSig, oldSig;*/
  sigset_t newMask, oldMask;
  /* end vars */


  /* check args to see if there's an inFile */
  if(argc > 2)
    {
      fprintf(stderr, "usage: mush [input file]\n");
      exit(1);
    }
  if(argc == 2)
    {
      if(!(inFile = fopen(argv[1], "r")))
	{
	  perror("fopen");
	  exit(-1);
	}
    }

  /* set up procmask */
  if(sigemptyset(&newMask) == -1)
    {
      perror("Setting up procmask");
      exit(-1);
    }
  if(sigaddset(&newMask, SIGINT) == -1)
    {
      perror("Setting up procmask");
      exit(-1);
    }
  if(sigprocmask(SIG_BLOCK, &newMask, &oldMask) == -1)
    {
      perror("Setting up procmask");
      exit(-1);
    }

  /* print prompt. Remember to print this again @ end of while */
  if(isatty(fileno(inFile)) && isatty(fileno(stdout)))
    {
      printf("8=P ");
    }

  /* while the user doesn't enter ^D */
  while((endCheck = getc(inFile)) != EOF)
    {
      if(ungetc(endCheck, inFile) == EOF)
	{
	  perror("ungetc");
	  exit(-2);
	}

      /* parse the line into stages, and poitn stage* to stages */
      currComm = parseline(inFile);

      /* if there was an error in parseline() */
      if(currComm == NULL)
	{
	  if(isatty(fileno(inFile)) && isatty(fileno(stdout)))
	    printf("8=P ");
	  continue;
	}


      /* check for CD call */
      if(strcmp(currComm[0].argv[0], "cd") == 0)
	{
	  if(currComm[0].argc != 2)
	    {
	      fprintf(stderr, "usage: cd <directory>\n");
	    }
	  else
	    {
	      if(chdir(currComm[0].argv[1]) == -1)
		{
		  perror(currComm[0].argv[1]);
		}
	    }
	  free(currComm);
	  if(isatty(fileno(inFile)) && isatty(fileno(stdout)))
	    printf("8=P ");
	  continue;
	}

      /* for each stage past stage 0, create a pipe. 
       * fork() in a loop, with the conditions being:
       * -I am parent; -There are more children to make;
       * If still parent, close all pipes and wait in a
       * for-loop, once for each child.
       */

      /* If child, dup2 over the correct pipes, close the
       * rest of them, exec, and error check
       */

      /* clear pipes to 0 */
      for(i = 0; i < MAX_COMMANDS - 1; i++)
	{
	  pipes[i][READ_END] = 0;
	  pipes[i][WRITE_END] = 0;
	}

      /* clear children */
      for(i = 0; i < MAX_COMMANDS; i++)
	{
	  children[i] = -1;
	}

      /* for each stage */
      for(i = 0; (i < MAX_COMMANDS) && (currComm[i].line != NULL); i++)
	{
	  /* if there's a next stage to pipe to */
	  if((i+1 < MAX_COMMANDS) && (currComm[i+1].line != NULL))
	    {
	      /* create a pipe */
	      if(pipe(pipes[i]) == -1)
		{
		  perror("piping error");
		  exit(-1);
		}
	    }
	}

      /* for each stage */
      for(i = 0; (i < MAX_COMMANDS) && (currComm[i].line != NULL); i++)
	{
	  /* create a child */
	  children[i] = fork();
	  if(children[i] == -1)
	    {
	      perror("forking error");
	      exit(-5);
	    }

	  if(children[i] == 0)
	    {
	      /* child things */
	      /* NOTE: in children, errors cause exit() */

	      /* clear procmask */
	      if(sigprocmask(SIG_SETMASK, &oldMask, NULL) == -1)
		{
		  perror("Clearing procmask");
		  exit(-21);
		}

	      /* if input isn't from original stdin */
		if(currComm[i].input != NULL)
		{
		  /* if it came from a pipe */
		  if(strncmp(currComm[i].input, "pipe from", 9) == 0)
		    {
		      /* dup the pipe into stdin */
		      if(-1 == dup2(pipes[i-1][READ_END], STDIN_FILENO))
			{
			  perror("dup2 error");
			  exit(-7);
			}
		    }
		  /* otherwise, it came from a file redirect */
		  else
		    {
		      /* open the infile for reading */
		      childInput = open(currComm[i].input, O_RDONLY);
		      if(childInput == -1)
			{
			  perror(currComm[i].input);
			  exit(-10);
			}
		      /* set stdin to this file */
		      if(dup2(childInput, STDIN_FILENO) == -1)
			{
			  perror(currComm[i].input);
			  exit(-11);
			}
		    }
		}
	      /* done setting up stdin */

	      /* repeat for stdout */
	      if(currComm[i].output != NULL)
		{
		  /* if it came from a pipe */
		  if(strncmp(currComm[i].output, "pipe to", 7) == 0)
		    {
		      /* dup the pipe into stdout */
		      if(-1 == dup2(pipes[i][WRITE_END], STDOUT_FILENO))
			{
			  perror("dup2 error");
			  exit(-78);
			}
		    }
		  /* otherwise, it came from a file redirect */
		  else
		    {
		      /* open the outfile for writing */
		      childOutput = open(currComm[i].output,
					 O_WRONLY|O_CREAT|O_TRUNC,
					 S_IRUSR|S_IWUSR|S_IRGRP|
					 S_IWGRP|S_IROTH|S_IWOTH);
		      if(childOutput == -1)
			{
			  perror(currComm[i].output);
			  exit(-12);
			}
		      /* set stdout to this new file */
		      if(dup2(childOutput, STDOUT_FILENO) == -1)
			{
			  perror("dup2 error");
			  exit(-13);
			}
		    }
		}
	      /* stdout set up */

	      /* close all of the pipes in child */
	      for(j = 0; j < MAX_COMMANDS -1; j++)
		{
		  if(pipes[j][READ_END] != 0)
		    {
		      if(close(pipes[j][READ_END]) == -1)
			{
			  perror("Closing child pipes");
			  exit(-15);
			}
		    }
		  if(pipes[j][WRITE_END] != 0)
		    {
		      if(close(pipes[j][WRITE_END]) == -1)
			{
			  perror("closing child pipes");
			  exit(-17);
			}
		    }
		}

	      /* execvp(), then error check. */
	      execvp(currComm[i].argv[0], currComm[i].argv);
	      perror(currComm[i].argv[0]);
	      /* fprintf(stderr, "%s failed to exec\n", currComm[i].argv[0]); */
	      exit(-30);
	    }
	  /* done with being child */
	}
      /* done creating children */

      /* close all of the pipes in parent */
      for(j = 0; j < MAX_COMMANDS -1; j++)
	{
	  if(pipes[j][READ_END] != 0)
	    {
	      if(close(pipes[j][READ_END]) == -1)
		{
		  perror("Closing parent pipes");
		  exit(-15);
		}
	    }
	  if(pipes[j][WRITE_END] != 0)
	    {
	      if(close(pipes[j][WRITE_END]) == -1)
		{
		  perror("closing parent pipes");
		  exit(-17);
		}
	    }
	}

      /* for-loop wait() for children to die */
      for(i = 0; i < MAX_COMMANDS; i++)
	{
	  if(children[i] != -1)
	    {
	      if(waitpid(children[i], &statuses[i], 0) == -1)
		{
		  if(errno != EINTR)
		    {
		      perror("waiting error");
		      exit(-123);
		    }
		}
	    }
	}

      if(fflush(stdout) == EOF)
	{
	  perror("fflush()ing streams");
	  exit(-1567);
	}

      /* free allocated memory */
      free(currComm);

      /* print prompt */
      if(isatty(fileno(inFile)) && isatty(fileno(stdout)))
	{
	  printf("8=P ");
	}
    }
  /* EOF was entered */

  printf("\n");

  return 0;
}

