#ifndef FILE_IO_H
#define FILE_IO_H

#include <stdbool.h>
#include <stdlib.h>

#include "utils.h"

typedef struct EditorFile EditorFile;

bool editorOpen(EditorFile* file, const char* filename);
void editorSave(EditorFile* file, int save_as);
void editorOpenFilePrompt(void);

#endif
