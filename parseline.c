#include "header.h"

stage* parseline(FILE* inFile)
{
  /* vars */
  char input[MAX_CHARS + 2];
  stage* commands;
  char* words[MAX_CHARS];
  char* strHolder = NULL;
  int i = 0;
  int j = 0;
  int count = 0;
  int args;
  /* end vars */

  if(!(commands = (stage*)malloc(sizeof(stage) * MAX_COMMANDS)))
    {
      perror("malloc");
      exit(-1);
    }

  /* initialize the list of commands/stages */
  for(i = 0; i < MAX_COMMANDS; i++)
    {
      commands[i].line = NULL;
      commands[i].input = NULL;
      commands[i].output = NULL;
      commands[i].argc = 0;
      for(j = 0; j < MAX_COMMANDS + 1; j++)
	{
	  commands[i].argv[j] = NULL;
	}
    }

  /* read in MAX_CHARS + 2, to have room for the NULL and
   * to check if command too long
   */
  if(!(fgets(input, MAX_CHARS + 2, inFile)))
    {
      perror("fgets");
      free(commands);
      return NULL;
    }

  /* ERROR CHECKING OF THE LINE TO BE PARSED */

  /* if the length is MAX_CHARS + 1 (adding one for the obligatory \n
   * at the end of every line) then command too long
   */
  if(strlen(input) >= MAX_CHARS + 1)
    {
      fprintf(stderr, "command too long\n");
      free(commands);
      return NULL;
    }

  /* if the command is empty */
  if((strHolder = malloc(MAX_CHARS + 2)) == NULL)
    {
      perror("malloc error");
      free(commands);
      return NULL;
    }
  strcat(strHolder, input);
  if((strtok(strHolder, " \n\t") == NULL))
    {
      /*fprintf(stderr, "invalid null command\n");*/
      free(commands);
      return NULL;
    }
  free(strHolder);


  /* read through input, counting up every |. */

  for(i = 0, count = 0, j = 1; i < strlen(input); i++)
    {
      if(input[i] == '|')
	{
	  count++;
	  while(isspace(input[i+j]))
	    {
	      j++;
	    }
	  /* if there's empty space between two pipes, error */
	  if(input[i+j] == '|')
	    {
	      fprintf(stderr, "invalid null command\n");
	      free(commands);
	      return NULL;
	    }
	}
      /* if there are more than 10 stages, error */
      if(count > 9)
	{
	  fprintf(stderr, "pipeline too deep\n");
	  free(commands);
	  return NULL;
	}
    }

  /* END ERROR CHECKING OF INPUT */

  /* first strtok sets up the rest of the uses of the function */
  commands[0].line = strtok(input, "|\n");

  /* while there's another token, break it off and assign
   * that new string to commands[i].line, then increment i
   */
  i = 1;
  while((commands[i++].line = strtok(NULL, "|\n")))
    /* do nothing */;

  /* for every command/set of args listed, separated by pipes */
  for(i = 0, count = 0; (commands[i].line != NULL) && (i < MAX_COMMANDS);
      i++, count = 0)
    {

      /* copy the line over so as not to destroy it */
      if(!(strHolder = malloc(strlen(commands[i].line))))
	{
	  perror("malloc error");
	  free(commands);
	  return NULL;
	}
      strcpy(strHolder, commands[i].line);

      /* tokenate the line by whitespace */
      words[count++] = strtok(strHolder, " \t");
      while((words[count++] = strtok(NULL, " \t")))
	    /* do nothing */;

      /* for each of these new words */
      for(count = 0, args = 0; (words[count] != NULL); count++)
	{
	  /* if it's "<", next word is input. Check for error, then 
	   * set the input to the following word.
	   */
	  if(strcmp(words[count], "<") == 0)
	    {
	      /* error checking */
	      if((i > 0) && (!(commands[i].input)))
		{
		  /* if this had a pipe leading to it */
		  fprintf(stderr, "%s: ambiguous input\n", commands[i].argv[0]);
		  free(commands);
		  return NULL;
		}

	      /* if there's no declared input file, or the symbols
	       * don't match, or input is already set by another "<"
	       */
	      if((!(words[count+1])) ||
		 (!(strcmp(words[count+1], "<"))) ||
		 (!(strcmp(words[count+1], ">"))) ||
		 ((commands[i].input)))
		{
		  fprintf(stderr, "%s: bad input redirection\n",
			  commands[i].argv[0]);
		  free(commands);
		  return NULL;
		}

	      /* set the next word in words[] to commands[i].input */
	      commands[i].input = words[++count];
	    }

	  /* if it's ">", next word is output. Check for errors, then
	   * set the output to the following word.
	   */
	  else if(strcmp(words[count], ">") == 0)
	    {
	      /* error checking */
	      if((i+1 < MAX_COMMANDS) && (!(commands[i].line)))
		{
		  /* if there's another stage in the pipeline */
		  fprintf(stderr, "%s: ambiguous output\n",
			  commands[i].argv[0]);
		  free(commands);
		  return NULL;
		}

	      /* if there's no declared output file, or the symbols
	       * don't match, or output is already set by another ">"
	       */
	      if((!(words[count+1])) ||
		 (!(strcmp(words[count+1], "<"))) ||
		 (!(strcmp(words[count+1], ">"))) ||
		 ((commands[i].output)))
		{
		  fprintf(stderr, "%s: bad output redirection\n", 
			  commands[i].argv[0]);
		  free(commands);
		  return NULL;
		}

	      /* set the next word in words[] to commands[i].output */
	      commands[i].output = words[++count];
	    }

	  else /* it's an argv */
	    {
	      /* if there's already MAX_ARGS, error
	       * (args is 0 based, MAX_ARGS is not)
	       */
	      if(args == MAX_ARGS - 1)
		{
		  fprintf(stderr, "%s: too many arguments\n", 
			  commands[i].argv[0]);
		  free(commands);
		  return NULL;
		}
	      /* add the arg to argv, and increment argc */
	      commands[i].argv[args++] = words[count];
	      commands[i].argc++;
	    }
	}
      /* done reading through words for this stage in the pipe */

      /* if there's a next commands[i], set up the pipeline */
      if((i+1 < MAX_COMMANDS) && (commands[i+1].line != NULL))
	{
	  /* if the current commands[i].output isn't NULL, error */
	  if(commands[i].output)
	    {
	      fprintf(stderr, "%s: ambiguous output\n", 
		      commands[i].argv[0]);
	      free(commands);
	      return NULL;
	    }

	  /* set the commands[i].output and commands[i+1].input for the pipe */
	  commands[i].output = malloc(PIPETO);
	  sprintf(commands[i].output, "pipe to stage %d", i+1);
	  commands[i+1].input = malloc(PIPEFROM);
	  sprintf(commands[i+1].input, "pipe from stage %d", i);
	}
    }
  /* done reading through every stage */

#ifdef SKIP
  /* print out each stage, and then we're done! */
  for(i = 0; (i < MAX_COMMANDS) && (commands[i].line != NULL); i++)
    {
      /* print out stage header */
      printf("\n--------\n");
      printf("Stage %d: \"%s\"\n--------\n", i, commands[i].line);

      /* print out input and output sources, and argc. If the input/output
       * are null, replace them with "Original std{in, out}"
       */
      printf("%10s: %s\n", "input",  (commands[i].input != NULL)?
	     (commands[i].input):("original stdin"));
      printf("%10s: %s\n", "output", (commands[i].output != NULL)?
	     (commands[i].output):("original stdout"));
      printf("%10s: %d\n", "argc", commands[i].argc);

      /* to print out argv's, loop through the argv array for however 
       * many argc's exist. Put a comma if there's a next element.
       */
      printf("%10s: ", "argv");
      for(count = 0; count < commands[i].argc; count++)
	{
	  printf("\"%s\"%s", commands[i].argv[count],
		 (count+1 < commands[i].argc)?(","):(""));
	}
      printf("\n");
    }
#endif
  return commands;
}
