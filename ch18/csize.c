/*
 * This program demonstrates the use of masks to extract and set a value.  It
 * prints the mask size of the CSIZE control mode flag, which specifies the 
 * number of bits per type for both transmission and reception.  This size
 * does not include theparity bit, if any.  The values for the field defined
 * by this mask are CS5, CS6, CS7 and CS8, for 5, 6, 7, and 8 bits/byte,
 * respectively.
 */
#include "apue.h"
#include <termios.h>

int main(void)
{
  struct termios term;

  if (tcgetattr(STDIN_FILENO, &term) < 0) {
    err_sys("tcgetattr() error");
  }

  switch (term.c_cflag & CSIZE) {
  case CS5:
    printf("5 bits/byte\n");
    break;
  case CS6:
    printf("6 bits/byte\n");
    break;
  case CS7:
    printf("7 bits/byte\n");
    break;
  case CS8:
    printf("8 bits/byte\n");
    break;
  default:
    printf("Unknown bits/byte\n");
  }

  /*
   * To use the mask, first zero the bits using the mask, and then set a value.
   */
  term.c_cflag &= ~CSIZE;    /* zero out the bits */
  term.c_cflag |= CS8;       /* set 8 bits/byte */
  if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
    err_sys("tcsetattr() error");
  }
  exit(0);
}
