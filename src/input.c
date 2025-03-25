#include "input.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "editor.h"
#include "file_io.h"
#include "output.h"
#include "prompt.h"
#include "select.h"
#include "terminal.h"
#include "unicode.h"
#include "utils.h"

void editorScrollToCursor(void) {
  int cols = editor.screen_cols - current_file->lineno_width;
  int rx = 0;
  if (current_file->cursor.y < current_file->num_rows) {
    rx = editorRowCxToRx(&current_file->row[current_file->cursor.y],
                         current_file->cursor.x);
  }

  if (current_file->cursor.y < current_file->row_offset) {
    current_file->row_offset = current_file->cursor.y;
  }
  if (current_file->cursor.y >=
      current_file->row_offset + editor.display_rows) {
    current_file->row_offset = current_file->cursor.y - editor.display_rows + 1;
  }
  if (rx < current_file->col_offset) {
    current_file->col_offset = rx;
  }
  if (rx >= current_file->col_offset + cols) {
    current_file->col_offset = rx - cols + 1;
  }
}

void editorScrollToCursorCenter(void) {
  current_file->row_offset = current_file->cursor.y - editor.display_rows / 2;
  if (current_file->row_offset < 0) {
    current_file->row_offset = 0;
  }
}

int getMousePosField(int x, int y) {
  if (y < 0 || y >= editor.screen_rows) return FIELD_ERROR;
  if (y == 0) return FIELD_TOP_STATUS;
  if (y == editor.screen_rows - 2 && editor.state != EDIT_MODE)
    return FIELD_PROMPT;
  if (y == editor.screen_rows - 1) return FIELD_STATUS;
  if (editor.file_count == 0) return FIELD_EMPTY;
  if (x < current_file->lineno_width) return FIELD_LINENO;
  return FIELD_TEXT;
}

void mousePosToEditorPos(int* x, int* y) {
  int row = current_file->row_offset + *y - 1;
  if (row < 0) {
    *x = 0;
    *y = 0;
    return;
  }
  if (row >= current_file->num_rows) {
    *y = current_file->num_rows - 1;
    *x = current_file->row[*y].rsize;
    return;
  }

  int col = *x - current_file->lineno_width + current_file->col_offset;
  if (col < 0) {
    col = 0;
  } else if (col > current_file->row[row].rsize) {
    col = current_file->row[row].rsize;
  }

  *x = col;
  *y = row;
}

void editorScroll(int dist) {
  int line = current_file->row_offset + dist;
  if (line < 0) {
    line = 0;
  } else if (line >= current_file->num_rows) {
    line = current_file->num_rows - 1;
  }
  current_file->row_offset = line;
}

void editorMoveCursor(int key) {
  const EditorRow* row = &current_file->row[current_file->cursor.y];
  switch (key) {
    case ARROW_LEFT:
      if (current_file->cursor.x != 0) {
        current_file->cursor.x = editorRowPreviousUTF8(
            &current_file->row[current_file->cursor.y], current_file->cursor.x);
        current_file->sx = editorRowCxToRx(
            &current_file->row[current_file->cursor.y], current_file->cursor.x);
      } else if (current_file->cursor.y > 0) {
        current_file->cursor.y--;
        current_file->cursor.x = current_file->row[current_file->cursor.y].size;
        current_file->sx = editorRowCxToRx(
            &current_file->row[current_file->cursor.y], current_file->cursor.x);
      }
      break;

    case ARROW_RIGHT:
      if (row && current_file->cursor.x < row->size) {
        current_file->cursor.x = editorRowNextUTF8(
            &current_file->row[current_file->cursor.y], current_file->cursor.x);
        current_file->sx = editorRowCxToRx(
            &current_file->row[current_file->cursor.y], current_file->cursor.x);
      } else if (row && (current_file->cursor.y + 1 < current_file->num_rows) &&
                 current_file->cursor.x == row->size) {
        current_file->cursor.y++;
        current_file->cursor.x = 0;
        current_file->sx = 0;
      }
      break;

    case ARROW_UP:
      if (current_file->cursor.y != 0) {
        current_file->cursor.y--;
        current_file->cursor.x = editorRowRxToCx(
            &current_file->row[current_file->cursor.y], current_file->sx);
      }
      break;

    case ARROW_DOWN:
      if (current_file->cursor.y + 1 < current_file->num_rows) {
        current_file->cursor.y++;
        current_file->cursor.x = editorRowRxToCx(
            &current_file->row[current_file->cursor.y], current_file->sx);
      }
      break;
  }
  row = (current_file->cursor.y >= current_file->num_rows)
            ? NULL
            : &current_file->row[current_file->cursor.y];
  int row_len = row ? row->size : 0;
  if (current_file->cursor.x > row_len) {
    current_file->cursor.x = row_len;
  }
}

static int findNextCharIndex(const EditorRow* row, int index,
                             IsCharFunc is_char) {
  while (index < row->size && !is_char(row->data[index])) {
    index++;
  }
  return index;
}

static int findPrevCharIndex(const EditorRow* row, int index,
                             IsCharFunc is_char) {
  while (index > 0 && !is_char(row->data[index - 1])) {
    index--;
  }
  return index;
}

static void editorMoveCursorWordLeft(void) {
  if (current_file->cursor.x == 0) {
    if (current_file->cursor.y == 0) return;
    editorMoveCursor(ARROW_LEFT);
  }

  const EditorRow* row = &current_file->row[current_file->cursor.y];
  current_file->cursor.x =
      findPrevCharIndex(row, current_file->cursor.x, isIdentifierChar);
  current_file->cursor.x =
      findPrevCharIndex(row, current_file->cursor.x, isNonIdentifierChar);
  current_file->sx = editorRowCxToRx(&current_file->row[current_file->cursor.y],
                                     current_file->cursor.x);
}

static void editorMoveCursorWordRight(void) {
  if (current_file->cursor.x ==
      current_file->row[current_file->cursor.y].size) {
    if (current_file->cursor.y == current_file->num_rows - 1) return;
    current_file->cursor.x = 0;
    current_file->cursor.y++;
  }

  const EditorRow* row = &current_file->row[current_file->cursor.y];
  current_file->cursor.x =
      findNextCharIndex(row, current_file->cursor.x, isIdentifierChar);
  current_file->cursor.x =
      findNextCharIndex(row, current_file->cursor.x, isNonIdentifierChar);
  current_file->sx = editorRowCxToRx(&current_file->row[current_file->cursor.y],
                                     current_file->cursor.x);
}

static void editorSelectWord(const EditorRow* row, int cx, IsCharFunc is_char) {
  current_file->cursor.select_x = findPrevCharIndex(row, cx, is_char);
  current_file->cursor.x = findNextCharIndex(row, cx, is_char);
  current_file->sx = editorRowCxToRx(row, current_file->cursor.x);
  current_file->cursor.is_selected = true;
}

static int handleTabClick(int x) {
  if (editor.loading) return -1;

  bool has_more_files = false;
  int tab_displayed = 0;
  int len = 0;
  if (editor.tab_offset != 0) {
    len++;
  }

  for (int i = 0; i < editor.file_count; i++) {
    if (i < editor.tab_offset) continue;

    const EditorFile* file = &editor.files[i];
    const char* filename =
        file->filename ? getBaseName(file->filename) : "Untitled";
    int tab_width = strUTF8Width(filename) + 2;

    if (file->dirty) tab_width++;

    if (editor.screen_cols - len < tab_width ||
        (i != editor.file_count - 1 && editor.screen_cols - len == tab_width)) {
      has_more_files = true;
      if (tab_displayed == 0) {
        // Display at least one tab
        tab_width = editor.screen_cols - len - 1;
      } else {
        break;
      }
    }

    len += tab_width;
    if (len > x) return i;

    tab_displayed++;
  }
  if (has_more_files) editor.tab_offset++;
  return -1;
}

static bool moveMouse(int x, int y) {
  if (getMousePosField(x, y) != FIELD_TEXT) return false;
  mousePosToEditorPos(&x, &y);
  current_file->cursor.is_selected = true;
  current_file->cursor.x = editorRowRxToCx(&current_file->row[y], x);
  current_file->cursor.y = y;
  current_file->sx = x;
  return true;
}

// Protect closing file with unsaved changes
static int close_protect = -1;
static void editorCloseFile(int index) {
  if (index < 0 || index > editor.file_count) {
    close_protect = -1;
    return;
  }

  if (editor.files[index].dirty && close_protect != index) {
    editorMsg(
        "File has unsaved changes. Press again to close file "
        "anyway.");
    close_protect = index;
    return;
  }

  editorRemoveFile(index);

  if (index < editor.file_index ||
      (editor.file_index == index && index == editor.file_count)) {
    editorChangeToFile(editor.file_index - 1);
  }
}

void editorProcessKeypress(void) {
  // Protect quiting program with unsaved files
  static bool quit_protect = true;

  static bool pressed = false;
  static int64_t prev_click_time = 0;
  static int mouse_click = 0;
  static int curr_x = 0;
  static int curr_y = 0;

  EditorInput input = editorReadKey();

  // Global keybinds
  switch (input.type) {
    // Prompt
    case CTRL_KEY('p'):
      editorOpenConfigPrompt();
      return;

    // Open file
    case CTRL_KEY('o'):
      editorOpenFilePrompt();
      return;
  }

  editorMsgClear();

  bool should_scroll = true;

  bool should_record_action = false;

  EditorAction* action = calloc_s(1, sizeof(EditorAction));
  action->type = ACTION_EDIT;
  EditAction* edit = &action->edit;

  edit->old_cursor = current_file->cursor;

  int c = input.type;
  int x = input.data.cursor.x;
  int y = input.data.cursor.y;
  switch (c) {
    // Action: Newline
    case '\r': {
      should_record_action = true;

      getSelectStartEnd(&edit->deleted_range);

      if (current_file->cursor.is_selected) {
        editorCopyText(&edit->deleted_text, edit->deleted_range);
        editorDeleteText(edit->deleted_range);
        current_file->cursor.is_selected = false;
      }

      edit->added_range.start_x = current_file->cursor.x;
      edit->added_range.start_y = current_file->cursor.y;
      editorInsertNewline();
      edit->added_range.end_x = current_file->cursor.x;
      edit->added_range.end_y = current_file->cursor.y;
      editorCopyText(&edit->added_text, edit->added_range);
    } break;

    // Quit editor
    case CTRL_KEY('q'): {
      close_protect = -1;
      editorFreeAction(action);
      bool dirty = false;
      for (int i = 0; i < editor.file_count; i++) {
        if (editor.files[i].dirty) {
          dirty = true;
          break;
        }
      }
      if (dirty && quit_protect) {
        editorMsg(
            "Files have unsaved changes. Press ^Q again to quit "
            "anyway.");
        quit_protect = false;
        return;
      }
#ifdef _DEBUG
      editorFree();
#endif
      exit(EXIT_SUCCESS);
    }

    // Close current file
    case CTRL_KEY('w'):
      quit_protect = true;
      editorFreeAction(action);
      editorCloseFile(editor.file_index);
      return;

    // Save
    case CTRL_KEY('s'):
      should_scroll = false;
      if (current_file->dirty) editorSave(current_file, 0);
      break;

    // Save all
    case ALT_KEY(CTRL_KEY('s')):
      // Alt+Ctrl+S
      should_scroll = false;
      for (int i = 0; i < editor.file_count; i++) {
        if (editor.files[i].dirty) {
          editorSave(&editor.files[i], 0);
        }
      }
      break;

    // Save as
    case CTRL_KEY('n'):
      should_scroll = false;
      editorSave(current_file, 1);
      break;

    case HOME_KEY:
    case SHIFT_HOME: {
      int start_x = findNextCharIndex(
          &current_file->row[current_file->cursor.y], 0, isNonSpace);
      if (start_x == current_file->cursor.x) start_x = 0;
      current_file->cursor.x = start_x;
      current_file->sx =
          editorRowCxToRx(&current_file->row[current_file->cursor.y], start_x);
      current_file->cursor.is_selected = (c == (SHIFT_HOME));
    } break;

    case END_KEY:
    case SHIFT_END:
      if (current_file->cursor.y < current_file->num_rows &&
          current_file->cursor.x !=
              current_file->row[current_file->cursor.y].size) {
        current_file->cursor.x = current_file->row[current_file->cursor.y].size;
        current_file->sx = editorRowCxToRx(
            &current_file->row[current_file->cursor.y], current_file->cursor.x);
        current_file->cursor.is_selected = (c == SHIFT_END);
      }
      break;

    case CTRL_LEFT:
    case SHIFT_CTRL_LEFT:
      editorMoveCursorWordLeft();
      current_file->cursor.is_selected = (c == SHIFT_CTRL_LEFT);
      break;

    case CTRL_RIGHT:
    case SHIFT_CTRL_RIGHT:
      editorMoveCursorWordRight();
      current_file->cursor.is_selected = (c == SHIFT_CTRL_RIGHT);
      break;

    // Find
    case CTRL_KEY('f'):
      should_scroll = false;
      current_file->cursor.is_selected = false;
      editorFind();
      break;

    // Goto line
    case CTRL_KEY('g'):
      should_scroll = false;
      current_file->cursor.is_selected = false;
      editorGotoLine();
      break;

    case CTRL_KEY('a'):
    SELECT_ALL:
      if (current_file->num_rows == 1 && current_file->row[0].size == 0) break;
      current_file->cursor.is_selected = true;
      current_file->cursor.y = current_file->num_rows - 1;
      current_file->cursor.x =
          current_file->row[current_file->num_rows - 1].size;
      current_file->sx = editorRowCxToRx(
          &current_file->row[current_file->cursor.y], current_file->cursor.x);
      current_file->cursor.select_y = 0;
      current_file->cursor.select_x = 0;

      should_scroll = false;
      break;

    // Action: Delete
    case DEL_KEY:
    case CTRL_KEY('h'):
    case BACKSPACE: {
      if (!current_file->cursor.is_selected) {
        if (c == DEL_KEY) {
          if (current_file->cursor.y == current_file->num_rows - 1 &&
              current_file->cursor.x ==
                  current_file->row[current_file->num_rows - 1].size)
            break;
        } else if (current_file->cursor.x == 0 && current_file->cursor.y == 0) {
          break;
        }
      }

      should_record_action = true;

      if (current_file->cursor.is_selected) {
        getSelectStartEnd(&edit->deleted_range);
        editorCopyText(&edit->deleted_text, edit->deleted_range);
        editorDeleteText(edit->deleted_range);
        current_file->cursor.is_selected = false;
        break;
      }

      if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);

      edit->deleted_range.end_x = current_file->cursor.x;
      edit->deleted_range.end_y = current_file->cursor.y;

      editorMoveCursor(ARROW_LEFT);

      edit->deleted_range.start_x = current_file->cursor.x;
      edit->deleted_range.start_y = current_file->cursor.y;
      editorCopyText(&edit->deleted_text, edit->deleted_range);
      editorDeleteText(edit->deleted_range);
    } break;

    // Action: Cut
    case CTRL_KEY('x'): {
      if (current_file->num_rows == 1 && current_file->row[0].size == 0) break;

      should_record_action = true;
      editorFreeClipboardContent(&editor.clipboard);

      if (!current_file->cursor.is_selected) {
        // Copy line
        EditorSelectRange range = {
            findNextCharIndex(&current_file->row[current_file->cursor.y], 0,
                              isNonSpace),
            current_file->cursor.y,
            current_file->row[current_file->cursor.y].size,
            current_file->cursor.y};
        editorCopyText(&editor.clipboard, range);

        // Delete line
        range.start_x = 0;
        if (current_file->num_rows != 1) {
          if (current_file->cursor.y == current_file->num_rows - 1) {
            range.start_y--;
            range.start_x = current_file->row[range.start_y].size;
          } else {
            range.end_y++;
            range.end_x = 0;
          }
        }

        edit->deleted_range = range;
        editorCopyText(&edit->deleted_text, edit->deleted_range);
        editorDeleteText(edit->deleted_range);
      } else {
        getSelectStartEnd(&edit->deleted_range);
        editorCopyText(&edit->deleted_text, edit->deleted_range);
        editorCopyText(&editor.clipboard, edit->deleted_range);
        editorDeleteText(edit->deleted_range);
        current_file->cursor.is_selected = false;
      }
      editorCopyToSysClipboard(&editor.clipboard);
    } break;

    // Copy
    case CTRL_KEY('c'): {
      editorFreeClipboardContent(&editor.clipboard);
      should_scroll = false;
      if (current_file->cursor.is_selected) {
        EditorSelectRange range;
        getSelectStartEnd(&range);
        editorCopyText(&editor.clipboard, range);
      } else {
        // Copy line
        EditorSelectRange range = {
            findNextCharIndex(&current_file->row[current_file->cursor.y], 0,
                              isNonSpace),
            current_file->cursor.y,
            current_file->row[current_file->cursor.y].size,
            current_file->cursor.y};
        editorCopyText(&editor.clipboard, range);
      }
      editorCopyToSysClipboard(&editor.clipboard);
    } break;

    // Action: Paste
    case CTRL_KEY('v'): {
      if (!editor.clipboard.size) break;

      should_record_action = true;

      getSelectStartEnd(&edit->deleted_range);

      if (current_file->cursor.is_selected) {
        editorCopyText(&edit->deleted_text, edit->deleted_range);
        editorDeleteText(edit->deleted_range);
        current_file->cursor.is_selected = false;
      }

      edit->added_range.start_x = current_file->cursor.x;
      edit->added_range.start_y = current_file->cursor.y;
      editorPasteText(&editor.clipboard, current_file->cursor.x,
                      current_file->cursor.y);

      edit->added_range.end_x = current_file->cursor.x;
      edit->added_range.end_y = current_file->cursor.y;
      editorCopyText(&edit->added_text, edit->added_range);
    } break;

    // Undo
    case CTRL_KEY('z'):
      current_file->cursor.is_selected = false;
      should_scroll = editorUndo();
      break;

    // Redo
    case CTRL_KEY('y'):
      current_file->cursor.is_selected = false;
      should_scroll = editorRedo();
      break;

    // Select word
    case CTRL_KEY('d'): {
      const EditorRow* row = &current_file->row[current_file->cursor.y];
      if (!isIdentifierChar(row->data[current_file->cursor.x])) {
        should_scroll = false;
        break;
      }
      editorSelectWord(row, current_file->cursor.x, isNonIdentifierChar);
    } break;

    // Previous file
    case CTRL_KEY('['):
      should_scroll = false;
      if (editor.file_count < 2) break;

      if (editor.file_index == 0)
        editorChangeToFile(editor.file_count - 1);
      else
        editorChangeToFile(editor.file_index - 1);
      break;

    // Next file
    case CTRL_KEY(']'):
      should_scroll = false;
      if (editor.file_count < 2) break;

      if (editor.file_index == editor.file_count - 1)
        editorChangeToFile(0);
      else
        editorChangeToFile(editor.file_index + 1);
      break;

    case SHIFT_PAGE_UP:
    case SHIFT_PAGE_DOWN:
    case PAGE_UP:
    case PAGE_DOWN:
      current_file->cursor.is_selected =
          (c == SHIFT_PAGE_UP || c == SHIFT_PAGE_DOWN);
      {
        if (c == PAGE_UP || c == SHIFT_PAGE_UP) {
          current_file->cursor.y = current_file->row_offset;
        } else if (c == PAGE_DOWN || c == SHIFT_PAGE_DOWN) {
          current_file->cursor.y =
              current_file->row_offset + editor.display_rows - 1;
          if (current_file->cursor.y >= current_file->num_rows)
            current_file->cursor.y = current_file->num_rows - 1;
        }
        int times = editor.display_rows;
        while (times--) {
          if (c == PAGE_UP || c == SHIFT_PAGE_UP) {
            if (current_file->cursor.y == 0) {
              current_file->cursor.x = 0;
              current_file->sx = 0;
              break;
            }
            editorMoveCursor(ARROW_UP);
          } else {
            if (current_file->cursor.y == current_file->num_rows - 1) {
              current_file->cursor.x =
                  current_file->row[current_file->cursor.y].size;
              break;
            }
            editorMoveCursor(ARROW_DOWN);
          }
        }
      }
      break;

    case SHIFT_CTRL_PAGE_UP:
    case CTRL_PAGE_UP:
      current_file->cursor.is_selected = (c == SHIFT_CTRL_PAGE_UP);
      while (current_file->cursor.y > 0) {
        editorMoveCursor(ARROW_UP);
        if (current_file->row[current_file->cursor.y].size == 0) {
          break;
        }
      }
      break;

    case SHIFT_CTRL_PAGE_DOWN:
    case CTRL_PAGE_DOWN:
      current_file->cursor.is_selected = (c == SHIFT_CTRL_PAGE_DOWN);
      while (current_file->cursor.y < current_file->num_rows - 1) {
        editorMoveCursor(ARROW_DOWN);
        if (current_file->row[current_file->cursor.y].size == 0) {
          break;
        }
      }
      break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      if (current_file->cursor.is_selected) {
        EditorSelectRange range;
        getSelectStartEnd(&range);

        if (c == ARROW_UP || c == ARROW_LEFT) {
          current_file->cursor.x = range.start_x;
          current_file->cursor.y = range.start_y;
        } else {
          current_file->cursor.x = range.end_x;
          current_file->cursor.y = range.end_y;
        }
        current_file->sx = editorRowCxToRx(
            &current_file->row[current_file->cursor.y], current_file->cursor.x);
        if (c == ARROW_UP || c == ARROW_DOWN) {
          editorMoveCursor(c);
        }
        current_file->cursor.is_selected = false;
      } else {
        editorMoveCursor(c);
      }
      break;

    case SHIFT_UP:
    case SHIFT_DOWN:
    case SHIFT_LEFT:
    case SHIFT_RIGHT:
      current_file->cursor.is_selected = true;
      editorMoveCursor(c - 9);
      break;

    case CTRL_HOME:
      current_file->cursor.is_selected = false;
      current_file->cursor.y = 0;
      current_file->cursor.x = 0;
      current_file->sx = 0;
      break;

    case CTRL_END:
      current_file->cursor.is_selected = false;
      current_file->cursor.y = current_file->num_rows - 1;
      current_file->cursor.x =
          current_file->row[current_file->num_rows - 1].size;
      current_file->sx = editorRowCxToRx(
          &current_file->row[current_file->cursor.y], current_file->cursor.x);
      break;

    // Action: Copy Line Up
    // Action: Copy Line Down
    case SHIFT_ALT_UP:
    case SHIFT_ALT_DOWN:
      should_record_action = true;
      current_file->cursor.is_selected = false;
      edit->old_cursor.is_selected = 0;
      editorInsertRow(current_file, current_file->cursor.y,
                      current_file->row[current_file->cursor.y].data,
                      current_file->row[current_file->cursor.y].size);

      edit->added_range.start_x =
          current_file->row[current_file->cursor.y].size;
      edit->added_range.start_y = current_file->cursor.y;
      edit->added_range.end_x =
          current_file->row[current_file->cursor.y + 1].size;
      edit->added_range.end_y = current_file->cursor.y + 1;
      editorCopyText(&edit->added_text, edit->added_range);

      if (c == SHIFT_ALT_DOWN) current_file->cursor.y++;
      break;

    // Action: Move Line Up
    // Action: Move Line Down
    case ALT_UP:
    case ALT_DOWN: {
      EditorSelectRange range;
      getSelectStartEnd(&range);
      if (c == ALT_UP) {
        if (range.start_y == 0) break;
      } else {
        if (range.end_y == current_file->num_rows - 1) break;
      }

      should_record_action = true;

      int old_cx = current_file->cursor.x;
      int old_cy = current_file->cursor.y;
      int old_select_x = current_file->cursor.select_x;
      int old_select_y = current_file->cursor.select_y;

      range.start_x = 0;
      int paste_x = 0;
      if (c == ALT_UP) {
        range.start_y--;
        range.end_x = current_file->row[range.end_y].size;
        editorCopyText(&edit->added_text, range);
        //  Move empty string at the start to the end
        char* temp = edit->added_text.data[0];
        memmove(&edit->added_text.data[0], &edit->added_text.data[1],
                (edit->added_text.size - 1) * sizeof(char*));
        edit->added_text.data[edit->added_text.size - 1] = temp;
      } else {
        range.end_x = 0;
        range.end_y++;
        editorCopyText(&edit->added_text, range);
        // Move empty string at the end to the start
        char* temp = edit->added_text.data[edit->added_text.size - 1];
        memmove(&edit->added_text.data[1], &edit->added_text.data[0],
                (edit->added_text.size - 1) * sizeof(char*));
        edit->added_text.data[0] = temp;
      }
      edit->deleted_range = range;
      editorCopyText(&edit->deleted_text, range);
      editorDeleteText(range);

      if (c == ALT_UP) {
        old_cy--;
        old_select_y--;
      } else {
        paste_x = current_file->row[current_file->cursor.y].size;
        old_cy++;
        old_select_y++;
      }

      range.start_x = paste_x;
      range.start_y = current_file->cursor.y;
      editorPasteText(&edit->added_text, paste_x, current_file->cursor.y);
      range.end_x = current_file->cursor.x;
      range.end_y = current_file->cursor.y;
      edit->added_range = range;

      current_file->cursor.x = old_cx;
      current_file->cursor.y = old_cy;
      current_file->cursor.select_x = old_select_x;
      current_file->cursor.select_y = old_select_y;
    } break;

    // Mouse input
    case MOUSE_PRESSED: {
      int field = getMousePosField(x, y);
      if (field != FIELD_TEXT) {
        should_scroll = false;
        mouse_click = 0;
        if (field == FIELD_TOP_STATUS) {
          editorChangeToFile(handleTabClick(x));
        }
        break;
      }

      int64_t click_time = getTime();
      int64_t time_diff = click_time - prev_click_time;

      if (x == curr_x && y == curr_y && time_diff / 1000 < 500) {
        mouse_click++;
      } else {
        mouse_click = 1;
      }
      prev_click_time = click_time;

      pressed = true;
      curr_x = x;
      curr_y = y;

      mousePosToEditorPos(&x, &y);
      int cx = editorRowRxToCx(&current_file->row[y], x);

      switch (mouse_click % 4) {
        case 1:
          // Mouse to pos
          current_file->cursor.is_selected = false;
          current_file->cursor.y = y;
          current_file->cursor.x = cx;
          current_file->sx = x;
          break;
        case 2: {
          // Select word
          const EditorRow* row = &current_file->row[y];
          if (row->size == 0) break;
          if (cx == row->size) cx--;

          IsCharFunc is_char;
          if (isspace(row->data[cx])) {
            is_char = isNonSpace;
          } else if (isIdentifierChar(row->data[cx])) {
            is_char = isNonIdentifierChar;
          } else {
            is_char = isNonSeparator;
          }
          editorSelectWord(row, cx, is_char);
        } break;
        case 3:
          // Select line
          if (current_file->cursor.y == current_file->num_rows - 1) {
            current_file->cursor.x =
                current_file->row[current_file->cursor.y].size;
            current_file->cursor.select_x = 0;
            current_file->sx =
                editorRowCxToRx(&current_file->row[y], current_file->cursor.x);
          } else {
            current_file->cursor.x = 0;
            current_file->cursor.y++;
            current_file->cursor.select_x = 0;
            current_file->sx = 0;
          }
          current_file->cursor.is_selected = true;
          break;
        case 0:
          goto SELECT_ALL;
      }
    } break;

    case MOUSE_RELEASED:
      should_scroll = false;
      pressed = false;
      break;

    case MOUSE_MOVE:
      should_scroll = false;
      if (moveMouse(x, y)) {
        curr_x = x;
        curr_y = y;
      }
      break;

    // Scroll up
    case WHEEL_UP: {
      int field = getMousePosField(x, y);
      should_scroll = false;
      if (field != FIELD_TEXT && field != FIELD_LINENO) {
        if (field == FIELD_TOP_STATUS) {
          if (editor.tab_offset > 0) editor.tab_offset--;
        }
        break;
      }
    }
    // fall through
    case CTRL_UP:
      should_scroll = false;
      editorScroll(-(c == WHEEL_UP ? 3 : 1));
      if (pressed) moveMouse(curr_x, curr_y);
      break;

    // Scroll down
    case WHEEL_DOWN: {
      int field = getMousePosField(x, y);
      should_scroll = false;
      if (field != FIELD_TEXT && field != FIELD_LINENO) {
        if (field == FIELD_TOP_STATUS) {
          handleTabClick(editor.screen_cols);
        }
        break;
      }
    }
    // fall through
    case CTRL_DOWN:
      should_scroll = false;
      editorScroll(c == WHEEL_DOWN ? 3 : 1);
      if (pressed) moveMouse(curr_x, curr_y);
      break;

    // Close tab
    case SCROLL_PRESSED:
      // Return to prevent resetting close_protect
      editorFreeAction(action);
      return;

    case SCROLL_RELEASED:
      should_scroll = false;
      if (getMousePosField(x, y) == FIELD_TOP_STATUS) {
        quit_protect = true;
        editorFreeAction(action);
        editorCloseFile(handleTabClick(x));
        return;
      }
      break;

    // Action: Input
    case CHAR_INPUT: {
      c = input.data.unicode;
      should_record_action = true;

      getSelectStartEnd(&edit->deleted_range);

      if (current_file->cursor.is_selected) {
        editorCopyText(&edit->deleted_text, edit->deleted_range);
        editorDeleteText(edit->deleted_range);
        current_file->cursor.is_selected = false;
      }

      int x_offset = 0;
      edit->added_range.start_x = current_file->cursor.x;
      edit->added_range.start_y = current_file->cursor.y;

      editorInsertUnicode(c);

      edit->added_range.end_x = current_file->cursor.x + x_offset;
      edit->added_range.end_y = current_file->cursor.y;
      editorCopyText(&edit->added_text, edit->added_range);

      current_file->sx = editorRowCxToRx(
          &current_file->row[current_file->cursor.y], current_file->cursor.x);
      current_file->cursor.is_selected = false;

      if (x_offset == -1) {
        should_record_action = false;
      }
    } break;

    default:
      should_scroll = false;
      break;
  }

  if (current_file->cursor.x == current_file->cursor.select_x &&
      current_file->cursor.y == current_file->cursor.select_y) {
    current_file->cursor.is_selected = false;
  }

  if (!current_file->cursor.is_selected) {
    current_file->cursor.select_x = current_file->cursor.x;
    current_file->cursor.select_y = current_file->cursor.y;
  }

  if (c != MOUSE_PRESSED && c != MOUSE_RELEASED) mouse_click = 0;

  if (should_record_action) {
    edit->new_cursor = current_file->cursor;
    editorAppendAction(action);
  } else {
    editorFreeAction(action);
  }

  if (should_scroll) editorScrollToCursor();
  close_protect = -1;
  quit_protect = true;
}
