#define _GNU_SOURCE  // SIGWINCH

#include "terminal.h"

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "defines.h"
#include "editor.h"
#include "os.h"
#include "output.h"

static struct termios orig_termios;

static void disableRawMode(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    PANIC("tcsetattr");
}

static void enableRawMode(void) {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) PANIC("tcgetattr");

  struct termios raw = orig_termios;

  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) PANIC("tcsetattr");
}

// Reads a character from the console.
static bool readConsole(uint32_t* unicode) {
  // Decode UTF-8

  int bytes;
  uint8_t first_byte;

  if (read(STDIN_FILENO, &first_byte, 1) != 1) {
    return false;
  }

  if ((first_byte & 0x80) == 0x00) {
    *unicode = (uint32_t)first_byte;
    return true;
  }

  if ((first_byte & 0xE0) == 0xC0) {
    *unicode = (first_byte & 0x1F) << 6;
    bytes = 1;
  } else if ((first_byte & 0xF0) == 0xE0) {
    *unicode = (first_byte & 0x0F) << 12;
    bytes = 2;
  } else if ((first_byte & 0xF8) == 0xF0) {
    *unicode = (first_byte & 0x07) << 18;
    bytes = 3;
  } else {
    return false;
  }

  uint8_t buf[3];
  if (read(STDIN_FILENO, buf, bytes) != bytes) {
    return false;
  }

  int shift = (bytes - 1) * 6;
  for (int i = 0; i < bytes; i++) {
    if ((buf[i] & 0xC0) != 0x80) {
      return false;
    }
    *unicode |= (buf[i] & 0x3F) << shift;
    shift -= 6;
  }

  return true;
}

typedef struct {
  const char* str;
  int value;
} StrIntPair;

static const StrIntPair sequence_lookup[] = {
    {"[1~", HOME_KEY},
    // {"[2~", INSERT_KEY},
    {"[3~", DEL_KEY},
    {"[4~", END_KEY},
    {"[5~", PAGE_UP},
    {"[6~", PAGE_DOWN},
    {"[7~", HOME_KEY},
    {"[8~", END_KEY},

    {"[A", ARROW_UP},
    {"[B", ARROW_DOWN},
    {"[C", ARROW_RIGHT},
    {"[D", ARROW_LEFT},
    {"[F", END_KEY},
    {"[H", HOME_KEY},

    /*
      Code     Modifiers
    ---------+---------------------------
       2     | Shift
       3     | Alt
       4     | Shift + Alt
       5     | Control
       6     | Shift + Control
       7     | Alt + Control
       8     | Shift + Alt + Control
       9     | Meta
       10    | Meta + Shift
       11    | Meta + Alt
       12    | Meta + Alt + Shift
       13    | Meta + Ctrl
       14    | Meta + Ctrl + Shift
       15    | Meta + Ctrl + Alt
       16    | Meta + Ctrl + Alt + Shift
    ---------+---------------------------
    */

    // Shift
    {"[1;2A", SHIFT_UP},
    {"[1;2B", SHIFT_DOWN},
    {"[1;2C", SHIFT_RIGHT},
    {"[1;2D", SHIFT_LEFT},
    {"[1;2F", SHIFT_END},
    {"[1;2H", SHIFT_HOME},

    // Alt
    {"[1;3A", ALT_UP},
    {"[1;3B", ALT_DOWN},

    // Shift+Alt
    {"[1;4A", SHIFT_ALT_UP},
    {"[1;4B", SHIFT_ALT_DOWN},

    // Ctrl
    {"[1;5A", CTRL_UP},
    {"[1;5B", CTRL_DOWN},
    {"[1;5C", CTRL_RIGHT},
    {"[1;5D", CTRL_LEFT},
    {"[1;5F", CTRL_END},
    {"[1;5H", CTRL_HOME},

    // Shift+Ctrl
    {"[1;6A", SHIFT_CTRL_UP},
    {"[1;6B", SHIFT_CTRL_DOWN},
    {"[1;6C", SHIFT_CTRL_RIGHT},
    {"[1;6D", SHIFT_CTRL_LEFT},

    // Page UP / Page Down
    {"[5;2~", SHIFT_PAGE_UP},
    {"[6;2~", SHIFT_PAGE_DOWN},
    {"[5;5~", CTRL_PAGE_UP},
    {"[6;5~", CTRL_PAGE_DOWN},
    {"[5;6~", SHIFT_CTRL_PAGE_UP},
    {"[6;6~", SHIFT_CTRL_PAGE_DOWN},
};

EditorInput editorReadKey(void) {
  uint32_t c;
  EditorInput result = {.type = UNKNOWN};

  while (!readConsole(&c)) {
  }

  if (c == ESC) {
    char seq[16] = {0};
    bool success = false;
    if (!readConsole(&c)) {
      result.type = ESC;
      return result;
    }
    seq[0] = (char)c;

    if (seq[0] != '[') {
      result.type = ALT_KEY(seq[0]);
      return result;
    }

    for (size_t i = 1; i < sizeof(seq) - 1; i++) {
      if (!readConsole(&c)) {
        return result;
      }
      seq[i] = (char)c;
      if (isupper(seq[i]) || seq[i] == 'm' || seq[i] == '~') {
        success = true;
        break;
      }
    }

    if (!success) {
      return result;
    }

    // Mouse input
    if (seq[1] == '<' && editor.mouse_mode) {
      int type;
      char m;
      sscanf(&seq[2], "%d;%d;%d%c", &type, &result.data.cursor.x,
             &result.data.cursor.y, &m);
      (*&result.data.cursor.x)--;
      (*&result.data.cursor.y)--;

      switch (type) {
        case 0:
          if (m == 'M') {
            result.type = MOUSE_PRESSED;
          } else if (m == 'm') {
            result.type = MOUSE_RELEASED;
          }
          break;
        case 1:
          if (m == 'M') {
            result.type = SCROLL_PRESSED;
          } else if (m == 'm') {
            result.type = SCROLL_RELEASED;
          }
          break;
        case 32:
          result.type = MOUSE_MOVE;
          break;
        case 64:
          result.type = WHEEL_UP;
          break;
        case 65:
          result.type = WHEEL_DOWN;
          break;
        default:
          break;
      }
      return result;
    }

    for (size_t i = 0; i < sizeof(sequence_lookup) / sizeof(sequence_lookup[0]);
         i++) {
      if (strcmp(sequence_lookup[i].str, seq) == 0) {
        result.type = sequence_lookup[i].value;
        return result;
      }
    }
    return result;
  }

  if ((c <= 31 || c == BACKSPACE) && c != '\t') {
    result.type = c;
    return result;
  }

  result.type = CHAR_INPUT;
  result.data.unicode = c;

  return result;
}

static int getCursorPos(int* rows, int* cols) {
  char buf[32];
  size_t i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
  return 0;
}

static int getWindowSize(int* rows, int* cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return getCursorPos(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

static void SIGSEGV_handler(int sig) {
  if (sig != SIGSEGV) return;
  terminalExit();
  UNUSED(write(STDOUT_FILENO, "Exit from SIGSEGV_handler\r\n", 27));
  _exit(EXIT_FAILURE);
}

static void SIGABRT_handler(int sig) {
  if (sig != SIGABRT) return;
  terminalExit();
  UNUSED(write(STDOUT_FILENO, "Exit from SIGABRT_handler\r\n", 27));
  _exit(EXIT_FAILURE);
}

static void enableSwap(void) {
  UNUSED(write(STDOUT_FILENO, "\x1b[?1049h\x1b[H", 11));
}

static void disableSwap(void) {
  UNUSED(write(STDOUT_FILENO, "\x1b[?1049l", 8));
}

void enableMouse(void) {
  if (!editor.mouse_mode &&
      write(STDOUT_FILENO, "\x1b[?1002h\x1b[?1015h\x1b[?1006h", 24) == 24)
    editor.mouse_mode = true;
}

void disableMouse(void) {
  if (editor.mouse_mode &&
      write(STDOUT_FILENO, "\x1b[?1002l\x1b[?1015l\x1b[?1006l", 24) == 24)
    editor.mouse_mode = false;
}

void resizeWindow(void) {
  int rows = 0;
  int cols = 0;

  if (getWindowSize(&rows, &cols) == -1) PANIC("getWindowSize");

  if (editor.screen_rows != rows || editor.screen_cols != cols) {
    editor.screen_rows = rows;
    editor.screen_cols = cols;
    // TODO: Don't hard coding rows
    editor.display_rows = rows - 2;

    if (!editor.loading) editorRefreshScreen();
  }
}

void editorInitTerminal(void) {
  enableRawMode();
  enableSwap();
  enableMouse();
  atexit(terminalExit);
  resizeWindow();

  if (signal(SIGSEGV, SIGSEGV_handler) == SIG_ERR) {
    PANIC("SIGSEGV_handler");
  }

  if (signal(SIGABRT, SIGABRT_handler) == SIG_ERR) {
    PANIC("SIGABRT_handler");
  }
}

void terminalExit(void) {
  disableMouse();
  disableSwap();
  // Show cursor
  UNUSED(write(STDOUT_FILENO, "\x1b[?25h", 6));
  disableRawMode();
}
