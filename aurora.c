#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "aurora_types.h"

static struct termios old_termios;

static void aurora_die(const char *s) {
    perror(s);
    exit(1);
}

static void aurora_restore_terminal_settings(void)
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_termios) == -1) {
        aurora_die("restoring terminal settings");
    }
}

static void aurora_enable_raw_mode(void) {
  if (tcgetattr(STDIN_FILENO, &old_termios) == -1) {
    aurora_die("getting terminal settings");
  }

  atexit(aurora_restore_terminal_settings);

  struct termios raw = old_termios;

  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    aurora_die("setting terminal settings");
  }
}

int main(void) {
  aurora_enable_raw_mode();

  while (1) {
    u8 ch;

    // On Cygwin, when read() times out, it returns -1 and errno is set to EAGAIN instead of returning 0 like it is supposed to.
    if (read(STDIN_FILENO, &ch, 1) == -1 && errno != EAGAIN) {
      aurora_die("reading from stdin");
    }

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
