#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>

#include "action.h"
#include "config.h"
#include "file_io.h"
#include "os.h"
#include "row.h"
#include "select.h"

#define EDITOR_FILE_MAX_SLOT 32

#define EDITOR_CON_COUNT 16
#define EDITOR_CON_LENGTH 255

#define EDITOR_PROMPT_LENGTH 255
#define EDITOR_RIGHT_PROMPT_LENGTH 32

typedef struct EditorSyntax EditorSyntax;

typedef struct EditorFile {
  // Cursor position
  EditorCursor cursor;

  // Hidden cursor x position
  int sx;

  // bracket complete level
  int bracket_autocomplete;

  // Editor offsets
  int row_offset;
  int col_offset;

  // Total line number
  int num_rows;
  int lineno_width;

  // File info
  int dirty;
  uint8_t newline;
  char* filename;
  FileInfo file_info;

  // Text buffers
  EditorRow* row;

  // Syntax highlight information
  EditorSyntax* syntax;

  // Undo redo
  EditorActionList* action_head;
  EditorActionList* action_current;
} EditorFile;

typedef struct Editor {
  // Raw screen size
  int screen_rows;
  int screen_cols;

  // Text field size
  int display_rows;

  // Editor mode
  bool loading;
  int state;
  bool mouse_mode;

  // Cursor position for prompt
  int px;

  // Copy paste
  EditorClipboard clipboard;

  // Color settings
  EditorColorScheme color_cfg;

  // ConCmd linked list
  EditorConCmd* cvars;

  // Files
  EditorFile files[EDITOR_FILE_MAX_SLOT];
  int file_count;
  int file_index;
  int tab_offset;
  int tab_displayed;

  // Syntax highlight
  EditorSyntax* HLDB;

  // Console
  int con_front;
  int con_rear;
  int con_size;
  char con_msg[EDITOR_CON_COUNT][EDITOR_CON_LENGTH];

  // Prompt
  char prompt[EDITOR_PROMPT_LENGTH];
  char prompt_right[EDITOR_RIGHT_PROMPT_LENGTH];
} Editor;

// Text editor
extern Editor editor;

// Current file
extern EditorFile* gCurFile;

void editorInit(void);
void editorFree(void);
void editorInitFile(EditorFile* file);
void editorFreeFile(EditorFile* file);

// Multiple files control
int editorAddFile(EditorFile* file);
void editorRemoveFile(int index);
void editorChangeToFile(int index);

#endif
