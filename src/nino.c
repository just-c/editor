#include <string.h>

#include "editor.h"
#include "file_io.h"
#include "input.h"
#include "os.h"
#include "output.h"
#include "prompt.h"
#include "row.h"

int main(int argc, char* argv[]) {
  editorInit();
  EditorFile file;
  editorInitFile(&file);

  Args cmd_args = argsGet(argc, argv);

  if (cmd_args.count > 1) {
    for (int i = 1; i < cmd_args.count; i++) {
      if (editor.file_count >= EDITOR_FILE_MAX_SLOT) {
        editorMsg("Already opened too many files!");
        break;
      }
      editorInitFile(&file);
      if (editorOpen(&file, cmd_args.args[i])) {
        editorAddFile(&file);
      }
    }
  }

  argsFree(cmd_args);

  if (editor.file_count == 0) {
    editorAddFile(&file);
    editorInsertRow(gCurFile, 0, "", 0);
  }

  editor.loading = false;

  while (editor.file_count) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  editorFree();
  return 0;
}
