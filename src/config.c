#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "editor.h"
#include "input.h"
#include "prompt.h"
#include "terminal.h"
#include "utils.h"

EditorConCmdArgs args;

static void cvarSyntaxCallback(void);
static void cvarMouseCallback(void);

CONVAR(tabsize, "Tab size.", "4", cvarSyntaxCallback);
CONVAR(autoindent, "Enable auto indent.", "0", NULL);
CONVAR(bracket, "Use auto bracket completion.", "0", NULL);
CONVAR(mouse, "Enable mouse mode.", "1", cvarMouseCallback);

static void reloadSyntax(void) {
  for (int i = 0; i < editor.file_count; i++) {
    for (int j = 0; j < editor.files[i].num_rows; j++) {
      editorUpdateRow(&editor.files[i].row[j]);
    }
  }
}

static void cvarSyntaxCallback(void) { reloadSyntax(); }

static void cvarMouseCallback(void) {
  bool mode = CONVAR_GETINT(mouse);
  if (editor.mouse_mode != mode) {
    if (mode) {
      enableMouse();
    } else {
      disableMouse();
    }
  }
}

const ColorElement color_element_map[EDITOR_COLOR_COUNT] = {
    {"bg", &editor.color_cfg.bg},

    {"top.fg", &editor.color_cfg.top_status[0]},
    {"top.bg", &editor.color_cfg.top_status[1]},
    {"top.tabs.fg", &editor.color_cfg.top_status[2]},
    {"top.tabs.bg", &editor.color_cfg.top_status[3]},
    {"top.select.fg", &editor.color_cfg.top_status[4]},
    {"top.select.bg", &editor.color_cfg.top_status[5]},

    {"prompt.fg", &editor.color_cfg.prompt[0]},
    {"prompt.bg", &editor.color_cfg.prompt[1]},

    // TODO: Customizable status bar
    {"status.fg", &editor.color_cfg.status[0]},
    {"status.bg", &editor.color_cfg.status[1]},
    {"status.lang.fg", &editor.color_cfg.status[2]},
    {"status.lang.bg", &editor.color_cfg.status[3]},
    {"status.pos.fg", &editor.color_cfg.status[4]},
    {"status.pos.bg", &editor.color_cfg.status[5]},

    {"lineno.fg", &editor.color_cfg.line_number[0]},
    {"lineno.bg", &editor.color_cfg.line_number[1]},

    {"cursorline", &editor.color_cfg.cursor_line},

    {"hl.normal", &editor.color_cfg.highlightFg[HL_NORMAL]},
    {"hl.comment", &editor.color_cfg.highlightFg[HL_COMMENT]},
    {"hl.keyword1", &editor.color_cfg.highlightFg[HL_KEYWORD1]},
    {"hl.keyword2", &editor.color_cfg.highlightFg[HL_KEYWORD2]},
    {"hl.keyword3", &editor.color_cfg.highlightFg[HL_KEYWORD3]},
    {"hl.string", &editor.color_cfg.highlightFg[HL_STRING]},
    {"hl.number", &editor.color_cfg.highlightFg[HL_NUMBER]},
    {"hl.space", &editor.color_cfg.highlightFg[HL_SPACE]},
    {"hl.match", &editor.color_cfg.highlightBg[HL_BG_MATCH]},
    {"hl.select", &editor.color_cfg.highlightBg[HL_BG_SELECT]},
    {"hl.trailing", &editor.color_cfg.highlightBg[HL_BG_TRAILING]},
};

CON_COMMAND(color, "Change the color of an element.") {
  if (args.argc != 2 && args.argc != 3) {
    editorMsg("Usage: color <element> [color]");
    return;
  }

  Color* target = NULL;
  int element_num = sizeof(color_element_map) / sizeof(ColorElement);
  for (int i = 0; i < element_num; i++) {
    if (strcmp(color_element_map[i].label, args.argv[1]) == 0) {
      target = color_element_map[i].color;
      break;
    }
  }
  if (!target) {
    editorMsg("Unknown element \"%s\".", args.argv[1]);
    return;
  }

  if (args.argc == 2) {
    char buf[8];
    colorToStr(*target, buf);
    editorMsg("%s = %s", args.argv[1], buf);
  } else if (args.argc == 3) {
    *target = strToColor(args.argv[2]);
  }
}

CON_COMMAND(exec, "Execute a config file.") {
  if (args.argc != 2) {
    editorMsg("Usage: exec <file>");
    return;
  }

  char filename[EDITOR_PATH_MAX] = {0};
  snprintf(filename, sizeof(filename), "%s", args.argv[1]);
  addDefaultExtension(filename, ".nino", sizeof(filename));

  if (!editorLoadConfig(filename)) {
    // Try config directory
    char config_path[EDITOR_PATH_MAX];
    int len =
        snprintf(config_path, sizeof(config_path),
                 PATH_CAT("%s", CONF_DIR, "%s"), getenv(ENV_HOME), filename);

    if (len < 0 || !editorLoadConfig(config_path)) {
      editorMsg("exec: Failed to exec \"%s\"", args.argv[1]);
      return;
    }
  }
}

CON_COMMAND(newline, "Set the EOL sequence (LF/CRLF).") {
  if (args.argc == 1) {
    editorMsg("%s", (current_file->newline == NL_UNIX) ? "LF" : "CRLF");
    return;
  }

  int nl;

  if (strCaseCmp(args.argv[1], "lf") == 0) {
    nl = NL_UNIX;
  } else if (strCaseCmp(args.argv[1], "crlf") == 0) {
    nl = NL_DOS;
  } else {
    editorMsg("Usage: newline <LF/CRLF>");
    return;
  }

  if (current_file->newline == nl) {
    return;
  }

  EditorAction* action = calloc_s(1, sizeof(EditorAction));
  action->type = ACTION_ATTRI;
  action->attri.new_newline = nl;
  action->attri.old_newline = current_file->newline;

  current_file->newline = nl;

  editorAppendAction(action);
}

CON_COMMAND(echo, "Echo text to console.") {
  if (args.argc < 2) return;

  int total_len = 0;
  char buf[COMMAND_MAX_LENGTH];
  memset(buf, 0, sizeof(buf));

  for (int i = 1; i < args.argc; i++) {
    int arg_len = strlen(args.argv[i]);
    if (total_len + arg_len + 1 <= COMMAND_MAX_LENGTH) {
      if (i > 1) {
        strcat(buf, " ");
        total_len++;
      }
      strcat(buf, args.argv[i]);
      total_len += arg_len;
    } else {
      break;
    }
  }
  editorMsg("%s", buf);
}

CON_COMMAND(clear, "Clear all console output.") {
  UNUSED(args.argc);
  editorMsgClear();
}

static void showCmdHelp(const EditorConCmd* cmd) {
  if (cmd->has_callback) {
    editorMsg("\"%s\"", cmd->name);
  } else {
    if (strcmp(cmd->cvar.default_string, cmd->cvar.string_val) == 0) {
      editorMsg("\"%s\" = \"%s\"", cmd->name, cmd->cvar.string_val);
    } else {
      editorMsg("\"%s\" = \"%s\" (def. \"%s\" )", cmd->name,
                cmd->cvar.string_val, cmd->cvar.default_string);
    }
  }
  editorMsg(" - %s", cmd->help_string);
}

CON_COMMAND(help, "Find help about a convar/concommand.") {
  if (args.argc != 2) {
    editorMsg("Usage: help <command>");
    return;
  }
  EditorConCmd* cmd = editorFindCmd(args.argv[1]);
  if (!cmd) {
    editorMsg("help: No cvar or command named \"%s\".", args.argv[1]);
    return;
  }
  showCmdHelp(cmd);
}

CON_COMMAND(
    find,
    "Find concommands with the specified string in their name/help text.") {
  if (args.argc != 2) {
    editorMsg("Usage: find <string>");
    return;
  }

  const char* s = args.argv[1];

  EditorConCmd* curr = editor.cvars;
  while (curr) {
    if (strCaseStr(curr->name, s) || strCaseStr(curr->help_string, s)) {
      showCmdHelp(curr);
    }
    curr = curr->next;
  }
}

#ifdef _DEBUG

CON_COMMAND(crash, "Cause the editor to crash. (Debug!!)") {
  int crash_type = 0;
  if (args.argc > 1) {
    crash_type = strToInt(args.argv[1]);
  }

  switch (crash_type) {
    case 0:
      // SIGSEGV
      *(volatile char*)0 = 0;
      break;
    case 1:
      // SIGABRT
      abort();
    default:
      editorMsg("Unknown crash type.");
  }
}

#endif

const EditorColorScheme color_default = {
    .bg = {30, 30, 30},
    .top_status =
        {
            {229, 229, 229},
            {37, 37, 37},
            {150, 150, 150},
            {45, 45, 45},
            {229, 229, 229},
            {87, 80, 104},
        },
    .prompt =
        {
            {229, 229, 229},
            {60, 60, 60},
        },
    .status =
        {
            {225, 219, 239},
            {87, 80, 104},
            {225, 219, 239},
            {169, 107, 33},
            {225, 219, 239},
            {217, 138, 43},
        },
    .line_number =
        {
            {127, 127, 127},
            {30, 30, 30},
        },
    .cursor_line = {40, 40, 40},
    .highlightFg =
        {
            {229, 229, 229},
            {106, 153, 85},
            {197, 134, 192},
            {86, 156, 214},
            {78, 201, 176},
            {206, 145, 120},
            {181, 206, 168},
            {63, 63, 63},
        },
    .highlightBg =
        {
            {0, 0, 0},
            {89, 46, 20},
            {38, 79, 120},
            {255, 100, 100},
        },
};

static void parseLine(const char* cmd);

static void cvarCmdCallback(EditorConCmd* cmd) {
  if (args.argc < 2) {
    showCmdHelp(cmd);
    return;
  }
  editorSetConVar(&cmd->cvar, args.argv[1]);
}

static void executeCommand() {
  if (args.argc < 1) return;

  if (args.argc > COMMAND_MAX_ARGC) {
    editorMsg("Command overflows the argument buffer. Clamped!");
    args.argc = COMMAND_MAX_ARGC;
  }

  EditorConCmd* cmd = editorFindCmd(args.argv[0]);
  if (!cmd) {
    editorMsg("Unknown command \"%s\".", args.argv[0]);
    return;
  }

  if (cmd->has_callback) {
    cmd->callback();
  } else {
    cvarCmdCallback(cmd);
  }
}

static void resetArgs() {
  for (int i = 0; i < args.argc; i++) {
    free(args.argv[i]);
  }
  args.argc = 0;
}

static void parseLine(const char* cmd) {
  // Command line parsing
  resetArgs();
  while (*cmd != '\0' && *cmd != '#') {
    switch (*cmd) {
      case '\t':
      case ' ':
        cmd++;
        break;

      case ';':
        executeCommand();
        resetArgs();
        cmd++;
        break;

      default: {
        bool in_quote = (*cmd == '"');
        if (in_quote) {
          cmd++;
        }

        char* buf = calloc_s(sizeof(char), COMMAND_MAX_LENGTH);

        for (int i = 0;
             *cmd != '\0' &&
             (in_quote ? (*cmd != '"')
                       : (*cmd != '#' && *cmd != ';' && *cmd != ' '));
             i++) {
          buf[i] = *cmd;
          cmd++;
        }

        if (in_quote) {
          cmd++;
        }

        if (args.argc < COMMAND_MAX_ARGC) {
          args.argv[args.argc] = buf;
        } else {
          free(buf);
        }

        // Let argc go pass COMMAND_MAX_ARGC.
        // We will detect it in executeCommand.
        args.argc++;
      }
    }
  }

  executeCommand();
}

bool editorLoadConfig(const char* path) {
  FILE* fp = fopen(path, "r");
  if (!fp) return false;

  char buf[COMMAND_MAX_LENGTH] = {0};
  while (fgets(buf, sizeof(buf), fp)) {
    buf[strcspn(buf, "\r\n")] = '\0';
    parseLine(buf);
  }
  fclose(fp);
  return true;
}

void editorInitConfig(void) {
  // Init commands
  INIT_CONVAR(tabsize);
  INIT_CONVAR(autoindent);
  INIT_CONVAR(bracket);
  INIT_CONVAR(mouse);

  INIT_CONCOMMAND(color);
  INIT_CONCOMMAND(newline);

  INIT_CONCOMMAND(exec);
  INIT_CONCOMMAND(echo);
  INIT_CONCOMMAND(clear);
  INIT_CONCOMMAND(help);
  INIT_CONCOMMAND(find);

#ifdef _DEBUG
  INIT_CONCOMMAND(crash);
#endif

  // Load defualt config
  char path[EDITOR_PATH_MAX] = {0};
  const char* home_dir = getenv(ENV_HOME);
  snprintf(path, sizeof(path), PATH_CAT("%s", CONF_DIR, "ninorc"), home_dir);
  if (!editorLoadConfig(path)) {
    snprintf(path, sizeof(path), PATH_CAT("%s", ".ninorc"), home_dir);
    editorLoadConfig(path);
  }
}

void editorFreeConfig(void) { resetArgs(); }

void editorOpenConfigPrompt(void) {
  char* query = editorPrompt("Prompt: %s", CONFIG_MODE, NULL);
  if (query == NULL) return;

  editorMsgClear();
  parseLine(query);
  free(query);
}

void editorSetConVar(EditorConVar* thisptr, const char* string_val) {
  strncpy(thisptr->string_val, string_val, COMMAND_MAX_LENGTH - 1);
  thisptr->string_val[COMMAND_MAX_LENGTH - 1] = '\0';
  thisptr->int_val = strToInt(string_val);

  if (thisptr->callback) {
    thisptr->callback();
  }
}

static inline void registerConCmd(EditorConCmd* thisptr) {
  thisptr->next = editor.cvars;
  editor.cvars = thisptr;
}

void editorInitConCmd(EditorConCmd* thisptr) {
  thisptr->next = NULL;
  thisptr->has_callback = true;
  registerConCmd(thisptr);
}

void editorInitConVar(EditorConCmd* thisptr) {
  thisptr->next = NULL;
  thisptr->has_callback = false;
  if (!thisptr->cvar.default_string) thisptr->cvar.default_string = "";
  editorSetConVar(&thisptr->cvar, thisptr->cvar.default_string);
  registerConCmd(thisptr);
}

EditorConCmd* editorFindCmd(const char* name) {
  EditorConCmd* result = NULL;
  EditorConCmd* curr = editor.cvars;
  while (curr) {
    if (strCaseCmp(name, curr->name) == 0) {
      result = curr;
      break;
    }
    curr = curr->next;
  }
  return result;
}
