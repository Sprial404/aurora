#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "aurora_types.h"

/*** SECTION: Defines */

#define CTRL_KEY(k) ((k) & 0x1f)

/*** SECTION: Global data */

static struct termios old_termios;

/*** SECTION: Output handling */

static void aurora_clear_screen(void) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
}


/*** SECTION: Terminal handling */

static void aurora_die(const char *s) {
  aurora_clear_screen();

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

static u8 aurora_get_char(void) {
  ssize_t nread;
  u8 c;

  // On Cygwin, when read() times out, it returns -1 and errno is set to EAGAIN instead of returning 0 like it is supposed to.
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) {
      aurora_die("read error");
    }
  }

  return c;
}

/*** SECTION: Input handling */

static void aurora_process_keypress(u8 c) {
  switch (c) {
    case CTRL_KEY('q'):
      aurora_clear_screen();
      exit(0);
      break;
  }
}

/*** SECTION: Main */

int main(void) {
  aurora_enable_raw_mode();

  while (1) {
    aurora_clear_screen();

    u8 c = aurora_get_char();
    aurora_process_keypress(c);
  }
  return 0;
}
