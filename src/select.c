#include "select.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "editor.h"
#include "row.h"
#include "utils.h"

void getSelectStartEnd(EditorSelectRange* range) {
  if (current_file->cursor.select_y > current_file->cursor.y) {
    range->start_x = current_file->cursor.x;
    range->start_y = current_file->cursor.y;
    range->end_x = current_file->cursor.select_x;
    range->end_y = current_file->cursor.select_y;
  } else if (current_file->cursor.select_y < current_file->cursor.y) {
    range->start_x = current_file->cursor.select_x;
    range->start_y = current_file->cursor.select_y;
    range->end_x = current_file->cursor.x;
    range->end_y = current_file->cursor.y;
  } else {
    // same row
    range->start_y = range->end_y = current_file->cursor.y;
    range->start_x = current_file->cursor.select_x > current_file->cursor.x
                         ? current_file->cursor.x
                         : current_file->cursor.select_x;
    range->end_x = current_file->cursor.select_x > current_file->cursor.x
                       ? current_file->cursor.select_x
                       : current_file->cursor.x;
  }
}

bool isPosSelected(int row, int col, EditorSelectRange range) {
  if (range.start_y < row && row < range.end_y) return true;

  if (range.start_y == row && range.end_y == row)
    return range.start_x <= col && col < range.end_x;

  if (range.start_y == row) return range.start_x <= col;

  if (range.end_y == row) return col < range.end_x;

  return false;
}

void editorDeleteText(EditorSelectRange range) {
  if (range.start_x == range.end_x && range.start_y == range.end_y) return;

  current_file->cursor.x = range.end_x;
  current_file->cursor.y = range.end_y;

  if (range.end_y - range.start_y > 1) {
    for (int i = range.start_y + 1; i < range.end_y; i++) {
      editorFreeRow(&current_file->row[i]);
    }
    int removed_rows = range.end_y - range.start_y - 1;
    memmove(&current_file->row[range.start_y + 1],
            &current_file->row[range.end_y],
            sizeof(EditorRow) * (current_file->num_rows - range.end_y));

    current_file->num_rows -= removed_rows;
    current_file->cursor.y -= removed_rows;

    current_file->lineno_width = getDigit(current_file->num_rows) + 2;
  }
  while (current_file->cursor.y != range.start_y ||
         current_file->cursor.x != range.start_x) {
    editorDelChar();
  }
}

void editorCopyText(EditorClipboard* clipboard, EditorSelectRange range) {
  if (range.start_x == range.end_x && range.start_y == range.end_y) {
    clipboard->size = 0;
    clipboard->data = NULL;
    return;
  }

  clipboard->size = range.end_y - range.start_y + 1;
  clipboard->data = malloc_s(sizeof(char*) * clipboard->size);
  // Only one line
  if (range.start_y == range.end_y) {
    clipboard->data[0] = malloc_s(range.end_x - range.start_x + 1);
    memcpy(clipboard->data[0],
           &current_file->row[range.start_y].data[range.start_x],
           range.end_x - range.start_x);
    clipboard->data[0][range.end_x - range.start_x] = '\0';
    return;
  }

  // First line
  size_t size = current_file->row[range.start_y].size - range.start_x;
  clipboard->data[0] = malloc_s(size + 1);
  memcpy(clipboard->data[0],
         &current_file->row[range.start_y].data[range.start_x], size);
  clipboard->data[0][size] = '\0';

  // Middle
  for (int i = range.start_y + 1; i < range.end_y; i++) {
    size = current_file->row[i].size;
    clipboard->data[i - range.start_y] = malloc_s(size + 1);
    memcpy(clipboard->data[i - range.start_y], current_file->row[i].data, size);
    clipboard->data[i - range.start_y][size] = '\0';
  }
  // Last line
  size = range.end_x;
  clipboard->data[range.end_y - range.start_y] = malloc_s(size + 1);
  memcpy(clipboard->data[range.end_y - range.start_y],
         current_file->row[range.end_y].data, size);
  clipboard->data[range.end_y - range.start_y][size] = '\0';
}

void editorPasteText(const EditorClipboard* clipboard, int x, int y) {
  if (!clipboard->size) return;

  current_file->cursor.x = x;
  current_file->cursor.y = y;

  if (clipboard->size == 1) {
    EditorRow* row = &current_file->row[y];
    char* paste = clipboard->data[0];
    size_t paste_len = strlen(paste);

    row->data = realloc_s(row->data, row->size + paste_len + 1);
    memmove(&row->data[x + paste_len], &row->data[x], row->size - x);
    memcpy(&row->data[x], paste, paste_len);
    row->size += paste_len;
    row->data[row->size] = '\0';
    editorUpdateRow(current_file, row);
    current_file->cursor.x += paste_len;
  } else {
    // First line
    int auto_indent = CONVAR_GETINT(autoindent);
    CONVAR_GETINT(autoindent) = 0;
    editorInsertNewline();
    CONVAR_GETINT(autoindent) = auto_indent;
    editorRowAppendString(current_file, &current_file->row[y],
                          clipboard->data[0], strlen(clipboard->data[0]));
    // Middle
    for (size_t i = 1; i < clipboard->size - 1; i++) {
      editorInsertRow(current_file, y + i, clipboard->data[i],
                      strlen(clipboard->data[i]));
    }
    // Last line
    EditorRow* row = &current_file->row[y + clipboard->size - 1];
    char* paste = clipboard->data[clipboard->size - 1];
    size_t paste_len = strlen(paste);

    row->data = realloc_s(row->data, row->size + paste_len + 1);
    memmove(&row->data[paste_len], row->data, row->size);
    memcpy(row->data, paste, paste_len);
    row->size += paste_len;
    row->data[row->size] = '\0';
    editorUpdateRow(current_file, row);

    current_file->cursor.y = y + clipboard->size - 1;
    current_file->cursor.x = paste_len;
  }
  current_file->sx = editorRowCxToRx(&current_file->row[current_file->cursor.y],
                                     current_file->cursor.x);
}

void editorFreeClipboardContent(EditorClipboard* clipboard) {
  if (!clipboard || !clipboard->size) return;
  for (size_t i = 0; i < clipboard->size; i++) {
    free(clipboard->data[i]);
  }
  clipboard->size = 0;
  free(clipboard->data);
}

void editorCopyToSysClipboard(EditorClipboard* clipboard) {
  if (!CONVAR_GETINT(osc52_copy)) return;

  if (!clipboard || !clipboard->size) return;

  abuf ab = ABUF_INIT;
  for (size_t i = 0; i < clipboard->size; i++) {
    if (i != 0) abufAppendN(&ab, "\n", 1);
    abufAppend(&ab, clipboard->data[i]);
  }

  int b64_len = base64EncodeLen(ab.len);
  char* b64_buf = malloc_s(b64_len * sizeof(char));

  b64_len = base64Encode(ab.buf, ab.len, b64_buf);

  bool tmux = (getenv("TMUX") != NULL);
  if (tmux) {
    fprintf(stdout, "\x1bPtmux;\x1b");
  }

  fprintf(stdout, "\x1b]52;c;%.*s\x07", b64_len, b64_buf);

  if (tmux) {
    fprintf(stdout, "\x1b\\");
  }

  fflush(stdout);

  free(b64_buf);
  abufFree(&ab);
}
