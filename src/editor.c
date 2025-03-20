#include "editor.h"

#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "highlight.h"
#include "output.h"
#include "prompt.h"
#include "terminal.h"
#include "utils.h"

Editor editor;
EditorFile* current_file;

void editorInit(void) {
  memset(&editor, 0, sizeof(Editor));
  editor.loading = true;
  editor.state = EDIT_MODE;
  editor.color_cfg = color_default;

  editor.con_front = -1;

  editorInitTerminal();
  editorInitConfig();
  editorInitHLDB();

  // Draw loading
  memset(&editor.files[0], 0, sizeof(EditorFile));
  current_file = &editor.files[0];
  editorRefreshScreen();
}

void editorFree(void) {
  for (int i = 0; i < editor.file_count; i++) {
    editorFreeFile(&editor.files[i]);
  }
  editorFreeClipboardContent(&editor.clipboard);
  editorFreeHLDB();
  editorFreeConfig();
}

void editorInitFile(EditorFile* file) {
  memset(file, 0, sizeof(EditorFile));
  file->newline = NL_DEFAULT;
}

void editorFreeFile(EditorFile* file) {
  for (int i = 0; i < file->num_rows; i++) {
    editorFreeRow(&file->row[i]);
  }
  editorFreeActionList(file->action_head);
  free(file->row);
  free(file->filename);
}

int editorAddFile(EditorFile* file) {
  if (editor.file_count >= EDITOR_FILE_MAX_SLOT) {
    editorMsg("Already opened too many files!");
    return -1;
  }

  EditorFile* current = &editor.files[editor.file_count];

  *current = *file;
  current->action_head = calloc_s(1, sizeof(EditorActionList));
  current->action_current = current->action_head;

  editor.file_count++;
  return editor.file_count - 1;
}

void editorRemoveFile(int index) {
  if (index < 0 || index > editor.file_count) return;

  EditorFile* file = &editor.files[index];
  editorFreeFile(file);
  if (file == &editor.files[editor.file_count]) {
    // file is at the end
    editor.file_count--;
    return;
  }
  memmove(file, &editor.files[index + 1],
          sizeof(EditorFile) * (editor.file_count - index));
  editor.file_count--;
}

void editorChangeToFile(int index) {
  if (index < 0 || index >= editor.file_count) return;
  editor.file_index = index;
  current_file = &editor.files[index];

  if (editor.tab_offset > index ||
      editor.tab_offset + editor.tab_displayed <= index) {
    editor.tab_offset = index;
  }
}
