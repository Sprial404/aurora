#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "aurora_types.h"

static struct termios old_termios;

static void aurora_restore_terminal_settings(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_termios);
}

static void aurora_enable_raw_mode(void) {
  tcgetattr(STDIN_FILENO, &old_termios);
  atexit(aurora_restore_terminal_settings);

  struct termios raw = old_termios;

  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(void) {
  aurora_enable_raw_mode();

  while (1) {
    u8 ch;
    read(STDIN_FILENO, &ch, 1);

    if (iscntrl(ch)) {
      printf("%d\r\n", ch);
    } else {
      printf("%d ('%c')\r\n", ch, ch);
    }

    if (ch == 'q') {
      break;
    }
  }
  return 0;
}
