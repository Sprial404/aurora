#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "aurora_types.h"

/*** SECTION: Defines */

#define CTRL_KEY(k) ((k) & 0x1f)

/*** SECTION: Constants */

/*** SECTION: Global data */

struct aurora_state {
  struct termios orig_termios;
  v2u screen_size;
};

struct aurora_state g_state;

/*** SECTION: Buffering */

struct aurora_buffer {
  char *buf;
  unsigned int len;
};

#define AURORA_BUFFER_INIT {NULL, 0}

void aurora_bappend(struct aurora_buffer *buf, const char *s, unsigned int len) {
  char *new_buf = realloc(buf->buf, buf->len + len);
  if (new_buf == NULL) {
    return;
  }

  memcpy(&new_buf[buf->len], s, len);
  buf->buf = new_buf;
  buf->len += len;
}

void aurora_bfree(struct aurora_buffer *buf) {
  free(buf->buf);
}

/*** SECTION: Output handling */

static void aurora_editor_draw_rows(struct aurora_buffer *buf) {
  int nrows = g_state.screen_size.y;

  for (int y = 0; y < nrows; y++) {
    aurora_bappend(buf, "~", 1);

    aurora_bappend(buf, "\x1b[K", 3);
    if (y < nrows - 1) {
      aurora_bappend(buf, "\r\n", 2);
    }
  }
}

static void aurora_redraw_screen(void) {
  struct aurora_buffer buf = AURORA_BUFFER_INIT;

  aurora_bappend(&buf, "\x1b[?25l", 6);
  aurora_bappend(&buf, "\x1b[H", 3);

  aurora_editor_draw_rows(&buf);

  aurora_bappend(&buf, "\x1b[H", 3);
  aurora_bappend(&buf, "\x1b[?25h", 6);

  write(STDOUT_FILENO, buf.buf, buf.len);
  aurora_bfree(&buf);
}

/*** SECTION: Terminal handling */

static void aurora_die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

static void aurora_restore_terminal_settings(void)
{
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_state.orig_termios) == -1) {
    aurora_die("restoring terminal settings");
  }
}

static void aurora_enable_raw_mode(void) {
  if (tcgetattr(STDIN_FILENO, &g_state.orig_termios) == -1) {
    aurora_die("getting terminal settings");
  }

  atexit(aurora_restore_terminal_settings);

  struct termios raw = g_state.orig_termios;

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

static int aurora_get_cursor_position(v2u *pos) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
    return -1;
  }

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) {
      break;
    }
    if (buf[i] == 'R') {
      break;
    }
    i += 1;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') {
    return -1;
  }

  if (sscanf(&buf[2], "%d;%d", &pos->y, &pos->x) != 2) {
    return -1;
  }

  return 0;
}

static int aurora_get_window_size(v2u *size) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
      return -1;
    }

    return aurora_get_cursor_position(size);
  }

  size->width = ws.ws_col;
  size->height = ws.ws_row;
  return 0;
}

/*** SECTION: Input handling */

static void aurora_process_keypress(u8 c) {
  switch (c) {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);

      exit(0);
      break;
  }
}

/*** SECTION: Initialization */

static void aurora_editor_init(void) {
  if (aurora_get_window_size(&g_state.screen_size) == -1) {
    aurora_die("getting window size");
  }
}

/*** SECTION: Main */

int main(void) {
  aurora_enable_raw_mode();
  aurora_editor_init();

  while (1) {
    aurora_redraw_screen();

    u8 c = aurora_get_char();
    aurora_process_keypress(c);
  }
  return 0;
}
