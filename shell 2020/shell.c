#include "parser/ast.h"
#include "shell.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#define STDIN  0
#define STDOUT 1
#define PIPE_RD 0
#define PIPE_WR 1

static void handle_signal(int signal)
{
  printf("Caught signal %d\n", signal);
}

void initialize(void)
{
    /* This code will be called once at startup */
    if (prompt)
        prompt = "vush$ ";
}

void handle_cmd(node_t *node)
{
  pid_t pid;
  int status;

  signal(SIGINT, handle_signal);

  if (strcmp(node -> command.program, "cd")==0)
  {
    if (chdir(node->command.argv[1]) < 0)
    {
      fprintf(stderr, "Error, dir not found\n");
    }
  }
  else if (strcmp(node->command.program, "exit")==0)
  {
			   exit(atoi(node->command.argv[1]));
  }
  else if (strcmp(node->command.program, "set") ==0)
  {
        putenv(node->command.argv[1]);
  }
  else if(strcmp(node->command.program,"unset") ==0)
  {
        unsetenv(node->command.argv[1]);
  }
  else
  {
    pid = fork();
    if(pid==0)
    {
      if(execvp(node->command.program, node->command.argv))
      {
        perror("error");
      }
      if(pid < 0)
      {
        printf( "Fork() error\n");
        _exit(1);
      }
      _exit(1);
    }
    else
    {
      waitpid(pid, &status, 0);
    }
  }
}

void handle_sequence(node_t *node)
{

  run_command(node->sequence.first);
  run_command(node->sequence.second);

}

void handle_pipe(node_t *node)
{
    pid_t pid_1, pid_2;
    int fd[2], status;
    pipe(fd);

    pid_1 = fork();
    if ( pid_1==0)
    {
        close(fd[PIPE_RD]);
        close(STDOUT);
        dup(fd[PIPE_WR]);
        handle_cmd(node->pipe.parts[0]);
        _exit(1);
    }

    pid_2 = fork();
    if ( pid_2==0)
    {
        close(fd[PIPE_WR]);
        close(STDIN);
        dup(fd[PIPE_RD]);
        handle_cmd(node->pipe.parts[1]);
        _exit(1);
    }

    close(fd[PIPE_RD]);
    close(fd[PIPE_WR]);

    waitpid(pid_1, &status, 0);
    waitpid(pid_2, &status, 0);

    return;
}

void handle_detach(node_t *node)
{
  pid_t pid = fork();
  switch(pid){
      case -1:
              printf( "Fork() error\n");
              break;
      case 0:
              run_command(node ->detach.child);
              _exit(1);

  }
}

void handle_subshell(node_t *node)
{
  int status;
  pid_t pid;
  pid = fork();
  switch(pid){
      case -1:
              printf( "Fork() error\n");
              break;
      case 0:
              run_command(node->subshell.child);
              _exit(1);
      default:
              waitpid(pid, &status, 0);
  }
}

void run_command(node_t *node)
{
    /* For testing: */
    // print_tree(node);

    switch(node->type)
    {
        case NODE_COMMAND:
            handle_cmd(node);
            break;

        case NODE_PIPE:
            handle_pipe(node);
            break;

        case NODE_REDIRECT:
            break;

        case NODE_SUBSHELL:
            handle_subshell(node);
            break;

        case NODE_DETACH:
            handle_detach(node);
            break;

        case NODE_SEQUENCE:
            handle_sequence(node);
            break;
    }

    if (prompt)
        prompt = "vush$ ";
}
