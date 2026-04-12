#include "vimmy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct winsize window_size;
struct termios orig_termios;

int cx, cy;

void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode); // restore terminal on exit

  struct termios raw = orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  // raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void refreshScreen(Buffer *buf) {
  write(STDOUT_FILENO, "\x1b[?25l", 6); // hide cursor
  write(STDOUT_FILENO, "\x1b[H", 3);    // move to top-left
  write(STDOUT_FILENO, "\x1b[2J", 4);   // clear screen

  for (int i = 0; i < window_size.ws_row; i++) {
    if (i < buf->num_rows) {
      write(STDOUT_FILENO, buf->rows[i].chars, buf->rows[i].len);
    } else {
      write(STDOUT_FILENO, "~", 1);
    }
    write(STDOUT_FILENO, "\r\n", 2);
  }

  char cursorBuf[32];
  int len = snprintf(cursorBuf, sizeof(cursorBuf), "\x1b[%d;%dH", cy + 1, cx + 1);
  write(STDOUT_FILENO, cursorBuf, len);    // cursor back to top-left
  write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
}

int getWindowSize() {
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size) != 0) {
    perror("ioctl failed!");
    return 1;
  }
  return 0;
}

void openFile(Buffer *buf, const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    perror("fopen");
    return;
  }

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    /* strip trailing newline */
    while (linelen > 0 &&
           (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
      linelen--;

    buf->rows = realloc(buf->rows, sizeof(Row) * (buf->num_rows + 1));
    Row *row = &buf->rows[buf->num_rows];
    row->len = linelen;
    row->chars = malloc(linelen + 1);
    memcpy(row->chars, line, linelen);
    row->chars[linelen] = '\0';
    buf->num_rows++;
  }

  free(line);
  fclose(fp);
}

int main(int argc, char *argv[]) {
  enableRawMode();
  getWindowSize();

  Buffer buf = {NULL, 0};
  if (argc >= 2)
    openFile(&buf, argv[1]);

  
  while (1) {
    char c;
    refreshScreen(&buf);
    read(STDIN_FILENO, &c, 1);

    switch(c) {
      case 'h': {
        if (cx > 0) cx--;
        break;
      }
      case 'j': {
        if (cy + 1 < window_size.ws_row) cy++;
        break;
      }
      case 'k': {
        if (cy > 0) cy--;
        break;
      }
      case 'l': {
        if (cx + 1 < window_size.ws_col) cx++;
        break;
      }
      case 'q': {
        return 0;
      }
    }
  }
}
