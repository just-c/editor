#include "file_io.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "input.h"
#include "output.h"
#include "prompt.h"
#include "row.h"

static int isFileOpened(FileInfo info) {
  for (int i = 0; i < editor.file_count; i++) {
    if (areFilesEqual(editor.files[i].file_info, info)) {
      return i;
    }
  }
  return -1;
}

static char* editroRowsToString(EditorFile* file, size_t* len) {
  size_t total_len = 0;
  int nl_len = (file->newline == NL_UNIX) ? 1 : 2;
  for (int i = 0; i < file->num_rows; i++) {
    total_len += file->row[i].size + nl_len;
  }

  // last line no newline
  *len = (total_len > 0) ? total_len - nl_len : 0;

  char* buf = malloc_s(total_len);
  char* p = buf;
  for (int i = 0; i < file->num_rows; i++) {
    memcpy(p, file->row[i].data, file->row[i].size);
    p += file->row[i].size;
    if (i != file->num_rows - 1) {
      if (file->newline == NL_DOS) {
        *p = '\r';
        p++;
      }
      *p = '\n';
      p++;
    }
  }

  return buf;
}

bool editorOpen(EditorFile* file, const char* path) {
  editorInitFile(file);

  FileType type = getFileType(path);
  switch (type) {
    case FT_REG: {
      FileInfo file_info = getFileInfo(path);
      if (file_info.error) {
        editorMsg("Can't load \"%s\"! Failed to get file info.", path);
        return false;
      }
      file->file_info = file_info;
      int open_index = isFileOpened(file_info);

      if (open_index != -1) {
        editorChangeToFile(open_index);
        return false;
      }
    } break;

    case FT_DIR:
      changeDir(path);
      return false;

    case FT_DEV:
      editorMsg("Can't load \"%s\"! It's a device file.", path);
      return false;

    default:
      break;
  }

  FILE* fp = openFile(path, "rb");
  if (!fp) {
    if (errno != ENOENT) {
      editorMsg("Can't load \"%s\"! %s", path, strerror(errno));
      return false;
    }

    // file doesn't exist
    char parent_dir[EDITOR_PATH_MAX];
    snprintf(parent_dir, sizeof(parent_dir), "%s", path);
    getDirName(parent_dir);
    if (access(parent_dir, 0) != 0) {  // F_OK
      editorMsg("Can't create \"%s\"! %s", path, strerror(errno));
      return false;
    }
    if (access(parent_dir, 2) != 0) {  // W_OK
      editorMsg("Can't write to \"%s\"! %s", path, strerror(errno));
      return false;
    }
  }

  const char* full_path = getFullPath(path);
  size_t path_len = strlen(full_path) + 1;
  free(file->filename);
  file->filename = malloc_s(path_len);
  memcpy(file->filename, full_path, path_len);

  file->dirty = 0;

  if (!fp) {
    editorInsertRow(file, file->cursor.y, "", 0);
    return true;
  }

  bool has_end_nl = true;
  bool has_cr = false;
  size_t at = 0;
  size_t cap = 16;

  char* line = NULL;
  size_t n = 0;
  int64_t len;

  file->row = malloc_s(sizeof(EditorRow) * cap);

  while ((len = getLine(&line, &n, fp)) != -1) {
    has_end_nl = false;
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
      if (line[len - 1] == '\r') has_cr = true;
      has_end_nl = true;
      len--;
    }
    // editorInsertRow but faster
    if (at >= cap) {
      cap *= 2;
      file->row = realloc_s(file->row, sizeof(EditorRow) * cap);
    }
    file->row[at].size = len;
    file->row[at].data = line;

    editorUpdateRow(&file->row[at]);

    line = NULL;
    n = 0;
    at++;
  }
  file->row = realloc_s(file->row, sizeof(EditorRow) * at);
  file->num_rows = at;
  file->lineno_width = getDigit(file->num_rows) + 2;

  if (has_end_nl) {
    editorInsertRow(file, file->num_rows, "", 0);
  }

  if (file->num_rows < 2) {
    file->newline = NL_DEFAULT;
  } else if (has_cr) {
    file->newline = NL_DOS;
  } else if (file->num_rows) {
    file->newline = NL_UNIX;
  }

  free(line);
  fclose(fp);

  return true;
}

void editorSave(EditorFile* file, int save_as) {
  if (!file->filename || save_as) {
    char* path = editorPrompt("Save as: %s", SAVE_AS_MODE, NULL);
    if (!path) {
      editorMsg("Save aborted.");
      return;
    }

    // Check path is valid
    FILE* fp = openFile(path, "wb");
    if (!fp) {
      editorMsg("Can't save \"%s\"! %s", path, strerror(errno));
      return;
    }
    fclose(fp);

    const char* full_path = getFullPath(path);
    size_t path_len = strlen(full_path) + 1;
    free(file->filename);
    file->filename = malloc_s(path_len);
    memcpy(file->filename, full_path, path_len);
  }

  size_t len;
  char* buf = editroRowsToString(file, &len);

  FILE* fp = openFile(file->filename, "wb");
  if (fp) {
    if (fwrite(buf, sizeof(char), len, fp) == len) {
      fclose(fp);
      free(buf);
      file->dirty = 0;
      editorMsg("%d bytes written to disk.", len);
      return;
    }
    fclose(fp);
  }
  free(buf);
  editorMsg("Can't save \"%s\"! %s", file->filename, strerror(errno));
}

void editorOpenFilePrompt(void) {
  if (editor.file_count >= EDITOR_FILE_MAX_SLOT) {
    editorMsg("Reached max file slots! Cannot open more files.",
              strerror(errno));
    return;
  }

  char* path = editorPrompt("Open: %s", OPEN_FILE_MODE, NULL);
  if (!path) return;

  EditorFile file;
  if (editorOpen(&file, path)) {
    int index = editorAddFile(&file);
    editor.state = EDIT_MODE;
    // hack: refresh screen to update editor.tab_displayed
    editorRefreshScreen();
    editorChangeToFile(index);
  }

  free(path);
}
