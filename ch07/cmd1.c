/*
 * Program skeleton for command processing.  Consists of a main loop that reads
 * lines from stdin and calls do_line() to process each line.  This function
 * then calls get_token() to fetch the next token from the input line.  The
 * first token of a line is assumed to be a command of some form, and a switch
 * statement selects each command.  For the single command shown, the function
 * cmd_add() is called.  This is a skeleton for a typical program that reads
 * commands, determines the command type, and then calls functions to process
 * each command.
 */
#include "apue.h"

#define TOK_ADD 5

void do_line(char *);
void cmd_add(void);
int get_token(void);

int main(void) {
  char line[MAXLINE];

  /* main loop reads lines from stdin & calls do_line() to process each line */
  while (fgets(line, MAXLINE, stdin) != NULL) {
    do_line(line);
  }
  exit(0);
}

char *tok_ptr; /* global pointer for get_token() */

/* Process one line of input */
void do_line(char *ptr) {
  int cmd;

  tok_ptr = ptr;
  while ((cmd = get_token()) > 0) {
    switch (cmd) { /* one case for each command */
    case TOK_ADD:
      cmd_add();
      break;
    }
  }
}

void cmd_add(void) {
  int token;

  token = get_token();
  /* rest of processing for this command */
}

int get_token(void) {
  /* fetch next token from line pointed to by tok_ptr */
}
