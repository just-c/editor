#include "row.h"

#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "editor.h"
#include "unicode.h"
#include "utils.h"

void editorUpdateRow(EditorRow* row) {
  row->rsize = editorRowCxToRx(row, row->size);
}

void editorInsertRow(EditorFile* file, int at, const char* s, size_t len) {
  if (at < 0 || at > file->num_rows) return;

  file->row = realloc_s(file->row, sizeof(EditorRow) * (file->num_rows + 1));
  memmove(&file->row[at + 1], &file->row[at],
          sizeof(EditorRow) * (file->num_rows - at));

  file->row[at].size = len;
  file->row[at].data = malloc_s(len + 1);
  memcpy(file->row[at].data, s, len);
  file->row[at].data[len] = '\0';

  editorUpdateRow(&file->row[at]);

  file->num_rows++;
  file->lineno_width = getDigit(file->num_rows) + 2;
}

void editorFreeRow(EditorRow* row) { free(row->data); }

void editorDelRow(EditorFile* file, int at) {
  if (at < 0 || at >= file->num_rows) return;
  editorFreeRow(&file->row[at]);
  memmove(&file->row[at], &file->row[at + 1],
          sizeof(EditorRow) * (file->num_rows - at - 1));

  file->num_rows--;
  file->lineno_width = getDigit(file->num_rows) + 2;
}

void editorRowInsertChar(EditorRow* row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->data = realloc_s(row->data, row->size + 2);
  memmove(&row->data[at + 1], &row->data[at], row->size - at + 1);
  row->size++;
  row->data[at] = c;
  editorUpdateRow(row);
}

void editorRowDelChar(EditorRow* row, int at) {
  if (at < 0 || at >= row->size) return;
  memmove(&row->data[at], &row->data[at + 1], row->size - at);
  row->size--;
  editorUpdateRow(row);
}

void editorRowAppendString(EditorRow* row, const char* s, size_t len) {
  row->data = realloc_s(row->data, row->size + len + 1);
  memcpy(&row->data[row->size], s, len);
  row->size += len;
  row->data[row->size] = '\0';
  editorUpdateRow(row);
}

void editorInsertChar(int c) {
  if (current_file->cursor.y == current_file->num_rows) {
    editorInsertRow(current_file, current_file->num_rows, "", 0);
  }
  editorRowInsertChar(&current_file->row[current_file->cursor.y],
                      current_file->cursor.x, c);
  current_file->cursor.x++;
}

void editorInsertUnicode(uint32_t unicode) {
  char output[4];
  int len = encodeUTF8(unicode, output);
  if (len == -1) return;

  for (int i = 0; i < len; i++) {
    editorInsertChar(output[i]);
  }
}

void editorInsertNewline(void) {
  int i = 0;

  if (current_file->cursor.x == 0) {
    editorInsertRow(current_file, current_file->cursor.y, "", 0);
  } else {
    editorInsertRow(current_file, current_file->cursor.y + 1, "", 0);
    EditorRow* curr_row = &current_file->row[current_file->cursor.y];
    EditorRow* new_row = &current_file->row[current_file->cursor.y + 1];
    if (CONVAR_GETINT(autoindent)) {
      while (i < current_file->cursor.x &&
             (curr_row->data[i] == ' ' || curr_row->data[i] == '\t'))
        i++;
      if (i != 0) editorRowAppendString(new_row, curr_row->data, i);
      if (curr_row->data[current_file->cursor.x - 1] == ':' ||
          (curr_row->data[current_file->cursor.x - 1] == '{' &&
           curr_row->data[current_file->cursor.x] != '}')) {
        editorRowAppendString(new_row, "\t", 1);
        i++;
      }
    }
    editorRowAppendString(new_row, &curr_row->data[current_file->cursor.x],
                          curr_row->size - current_file->cursor.x);
    curr_row->size = current_file->cursor.x;
    curr_row->data[curr_row->size] = '\0';
    editorUpdateRow(curr_row);
  }
  current_file->cursor.y++;
  current_file->cursor.x = i;
  current_file->sx =
      editorRowCxToRx(&current_file->row[current_file->cursor.y], i);
}

void editorDelChar(void) {
  if (current_file->cursor.y == current_file->num_rows) return;
  if (current_file->cursor.x == 0 && current_file->cursor.y == 0) return;
  EditorRow* row = &current_file->row[current_file->cursor.y];
  if (current_file->cursor.x > 0) {
    editorRowDelChar(row, current_file->cursor.x - 1);
    current_file->cursor.x--;
  } else {
    current_file->cursor.x = current_file->row[current_file->cursor.y - 1].size;
    editorRowAppendString(&current_file->row[current_file->cursor.y - 1],
                          row->data, row->size);
    editorDelRow(current_file, current_file->cursor.y);
    current_file->cursor.y--;
  }
  current_file->sx = editorRowCxToRx(&current_file->row[current_file->cursor.y],
                                     current_file->cursor.x);
}

int editorRowNextUTF8(EditorRow* row, int cx) {
  if (cx < 0) return 0;

  if (cx >= row->size) return row->size;

  const char* s = &row->data[cx];
  size_t byte_size;
  decodeUTF8(s, row->size - cx, &byte_size);
  return cx + byte_size;
}

int editorRowPreviousUTF8(EditorRow* row, int cx) {
  if (cx <= 0) return 0;

  if (cx > row->size) return row->size;

  int i = 0;
  size_t byte_size = 0;
  while (i < cx) {
    decodeUTF8(&row->data[i], row->size - i, &byte_size);
    i += byte_size;
  }
  return i - byte_size;
}

int editorRowCxToRx(const EditorRow* row, int cx) {
  int rx = 0;
  int i = 0;
  while (i < cx) {
    size_t byte_size;
    uint32_t unicode = decodeUTF8(&row->data[i], row->size - i, &byte_size);
    if (unicode == '\t') {
      rx += (CONVAR_GETINT(tabsize) - 1) - (rx % CONVAR_GETINT(tabsize)) + 1;
    } else {
      int width = unicodeWidth(unicode);
      if (width < 0) width = 1;
      rx += width;
    }
    i += byte_size;
  }
  return rx;
}

int editorRowRxToCx(const EditorRow* row, int rx) {
  int cur_rx = 0;
  int cx = 0;
  while (cx < row->size) {
    size_t byte_size;
    uint32_t unicode = decodeUTF8(&row->data[cx], row->size - cx, &byte_size);
    if (unicode == '\t') {
      cur_rx +=
          (CONVAR_GETINT(tabsize) - 1) - (cur_rx % CONVAR_GETINT(tabsize)) + 1;
    } else {
      int width = unicodeWidth(unicode);
      if (width < 0) width = 1;
      cur_rx += width;
    }
    if (cur_rx > rx) return cx;
    cx += byte_size;
  }
  return cx;
}
