#ifndef OS_UNIX_H
#define OS_UNIX_H

#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>

#define ENV_HOME "HOME"
#define CONF_DIR ".config/nino"
#define DIR_SEP "/"

#include <linux/limits.h>
#define EDITOR_PATH_MAX PATH_MAX

struct FileInfo {
  struct stat info;

  bool error;
};

struct DirIter {
  DIR* dp;
  struct dirent* entry;

  bool error;
};

#endif
