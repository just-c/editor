#include <stdlib.h>

#include "defines.h"
#include "editor.h"
#include "input.h"
#include "output.h"
#include "status.h"

void editorGotoLine() {
    char* query = editorPrompt("Goto line: %s", GOTO_LINE_MODE, NULL);
    if (query == NULL)
        return;
    int line = atoi(query);
    if (line < 0) {
        line = E.num_rows + 1 + line;
    }

    if (line > 0 && line <= E.num_rows) {
        E.cursor.x = 0;
        E.sx = 0;
        E.cursor.y = line - 1;
        editorScroll();
    } else {
        editorSetStatusMsg("Type a line number between 1 to %d.", E.num_rows);
    }

    if (query) {
        free(query);
    }
}
