#include "output.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "defines.h"
#include "editor.h"
#include "os.h"
#include "prompt.h"
#include "select.h"
#include "unicode.h"

static void editorDrawTopStatusBar(abuf* ab) {
  const char* right_buf = "  nino  ";
  bool has_more_files = false;
  int rlen = strlen(right_buf);
  int len = 0;

  gotoXY(ab, 1, 1);

  setColor(ab, editor.color_cfg.top_status[0], 0);
  setColor(ab, editor.color_cfg.top_status[1], 1);

  if (editor.tab_offset != 0) {
    abufAppendN(ab, "<", 1);
    len++;
  }

  editor.tab_displayed = 0;
  if (editor.loading) {
    const char* loading_text = "Loading...";
    int loading_text_len = strlen(loading_text);
    abufAppendN(ab, loading_text, loading_text_len);
    len = loading_text_len;
  } else {
    for (int i = 0; i < editor.file_count; i++) {
      if (i < editor.tab_offset) continue;

      const EditorFile* file = &editor.files[i];

      bool is_current = (file == current_file);
      if (is_current) {
        setColor(ab, editor.color_cfg.top_status[4], 0);
        setColor(ab, editor.color_cfg.top_status[5], 1);
      } else {
        setColor(ab, editor.color_cfg.top_status[2], 0);
        setColor(ab, editor.color_cfg.top_status[3], 1);
      }

      char buf[EDITOR_PATH_MAX] = {0};
      const char* filename =
          file->filename ? getBaseName(file->filename) : "Untitled";
      int buf_len = snprintf(buf, sizeof(buf), " %s%s ", file->dirty ? "*" : "",
                             filename);
      int tab_width = strUTF8Width(buf);

      if (editor.screen_cols - len < tab_width ||
          (i != editor.file_count - 1 &&
           editor.screen_cols - len == tab_width)) {
        has_more_files = true;
        if (editor.tab_displayed == 0) {
          // Display at least one tab
          // TODO: This is wrong
          tab_width = editor.screen_cols - len - 1;
          buf_len = editor.screen_cols - len - 1;
        } else {
          break;
        }
      }

      // Not enough space to even show one tab
      if (tab_width < 0) break;

      abufAppendN(ab, buf, buf_len);
      len += tab_width;
      editor.tab_displayed++;
    }
  }

  setColor(ab, editor.color_cfg.top_status[0], 0);
  setColor(ab, editor.color_cfg.top_status[1], 1);

  if (has_more_files) {
    abufAppendN(ab, ">", 1);
    len++;
  }

  while (len < editor.screen_cols) {
    if (editor.screen_cols - len == rlen) {
      abufAppendN(ab, right_buf, rlen);
      break;
    } else {
      abufAppend(ab, " ");
      len++;
    }
  }
}

static void editorDrawConMsg(abuf* ab) {
  if (editor.con_size == 0) {
    return;
  }

  setColor(ab, editor.color_cfg.prompt[0], 0);
  setColor(ab, editor.color_cfg.prompt[1], 1);

  bool should_draw_prompt = (editor.state != EDIT_MODE);
  int draw_x = editor.screen_rows - editor.con_size;
  if (should_draw_prompt) {
    draw_x--;
  }

  int index = editor.con_front;
  for (int i = 0; i < editor.con_size; i++) {
    gotoXY(ab, draw_x, 0);
    draw_x++;

    const char* buf = editor.con_msg[index];
    index = (index + 1) % EDITOR_CON_COUNT;

    int len = strlen(buf);
    if (len > editor.screen_cols) {
      len = editor.screen_cols;
    }

    abufAppendN(ab, buf, len);

    while (len < editor.screen_cols) {
      abufAppend(ab, " ");
      len++;
    }
  }
}

static void editorDrawPrompt(abuf* ab) {
  bool should_draw_prompt = (editor.state != EDIT_MODE);
  if (!should_draw_prompt) {
    return;
  }

  setColor(ab, editor.color_cfg.prompt[0], 0);
  setColor(ab, editor.color_cfg.prompt[1], 1);

  gotoXY(ab, editor.screen_rows - 1, 0);

  const char* left = editor.prompt;
  int len = strlen(left);

  const char* right = editor.prompt_right;
  int rlen = strlen(right);

  if (rlen > editor.screen_cols) {
    rlen = 0;
  }

  if (len + rlen > editor.screen_cols) {
    len = editor.screen_cols - rlen;
  }

  abufAppendN(ab, left, len);

  while (len < editor.screen_cols) {
    if (editor.screen_cols - len == rlen) {
      abufAppendN(ab, right, rlen);
      break;
    } else {
      abufAppend(ab, " ");
      len++;
    }
  }
}

static void editorDrawStatusBar(abuf* ab) {
  gotoXY(ab, editor.screen_rows, 0);

  setColor(ab, editor.color_cfg.status[0], 0);
  setColor(ab, editor.color_cfg.status[1], 1);

  const char* help_str = "";
  const char* help_info[] = {
      " ^Q: Quit  ^O: Open  ^P: Prompt  ^S: Save  ^F: Find  ^G: Goto",
      " ^Q: Quit  ^O: Open  ^P: Prompt",
      " ^Q: Cancel  Up: back  Down: Next",
      " ^Q: Cancel",
      " ^Q: Cancel",
      " ^Q: Cancel",
      " ^Q: Cancel",
  };
  if (CONVAR_GETINT(helpinfo)) help_str = help_info[editor.state];

  char lang[16];
  char pos[64];
  int len = strlen(help_str);
  int lang_len, pos_len;
  int rlen;
  if (editor.file_count == 0) {
    lang_len = 0;
    pos_len = 0;
  } else {
    const char* file_type = "Plain Text";
    int row = current_file->cursor.y + 1;
    int col = editorRowCxToRx(&current_file->row[current_file->cursor.y],
                              current_file->cursor.x) +
              1;
    float line_percent = 0.0f;
    const char* nl_type = (current_file->newline == NL_UNIX) ? "LF" : "CRLF";
    if (current_file->num_rows - 1 > 0) {
      line_percent = (float)current_file->row_offset /
                     (current_file->num_rows - 1) * 100.0f;
    }

    lang_len = snprintf(lang, sizeof(lang), "  %s  ", file_type);
    pos_len = snprintf(pos, sizeof(pos), " %d:%d [%.f%%] <%s> ", row, col,
                       line_percent, nl_type);
  }

  rlen = lang_len + pos_len;

  if (rlen > editor.screen_cols) rlen = 0;
  if (len + rlen > editor.screen_cols) len = editor.screen_cols - rlen;

  abufAppendN(ab, help_str, len);

  while (len < editor.screen_cols) {
    if (editor.screen_cols - len == rlen) {
      setColor(ab, editor.color_cfg.status[2], 0);
      setColor(ab, editor.color_cfg.status[3], 1);
      abufAppendN(ab, lang, lang_len);
      setColor(ab, editor.color_cfg.status[4], 0);
      setColor(ab, editor.color_cfg.status[5], 1);
      abufAppendN(ab, pos, pos_len);
      break;
    } else {
      abufAppend(ab, " ");
      len++;
    }
  }
}

static void editorDrawRows(abuf* ab) {
  setColor(ab, editor.color_cfg.bg, 1);

  EditorSelectRange range = {0};
  if (current_file->cursor.is_selected) getSelectStartEnd(&range);

  for (int i = current_file->row_offset, s_row = 2;
       i < current_file->row_offset + editor.display_rows; i++, s_row++) {
    bool is_row_full = false;
    // Move cursor to the beginning of a row
    gotoXY(ab, s_row, 1);

    editor.color_cfg.highlightBg[HL_BG_NORMAL] = editor.color_cfg.bg;
    if (i < current_file->num_rows) {
      char line_number[16];
      if (i == current_file->cursor.y) {
        if (!current_file->cursor.is_selected) {
          editor.color_cfg.highlightBg[HL_BG_NORMAL] =
              editor.color_cfg.cursor_line;
        }
        setColor(ab, editor.color_cfg.line_number[1], 0);
        setColor(ab, editor.color_cfg.line_number[0], 1);
      } else {
        setColor(ab, editor.color_cfg.line_number[0], 0);
        setColor(ab, editor.color_cfg.line_number[1], 1);
      }

      snprintf(line_number, sizeof(line_number), " %*d ",
               current_file->lineno_width - 2, i + 1);
      abufAppend(ab, line_number);

      abufAppend(ab, ANSI_CLEAR);
      setColor(ab, editor.color_cfg.bg, 1);

      int cols = editor.screen_cols - current_file->lineno_width;
      int col_offset =
          editorRowRxToCx(&current_file->row[i], current_file->col_offset);
      int len = current_file->row[i].size - col_offset;
      len = (len < 0) ? 0 : len;

      int rlen = current_file->row[i].rsize - current_file->col_offset;
      is_row_full = (rlen > cols);
      rlen = is_row_full ? cols : rlen;
      rlen += current_file->col_offset;

      char* c = &current_file->row[i].data[col_offset];

      uint8_t curr_fg = HL_BG_NORMAL;
      uint8_t curr_bg = HL_NORMAL;

      setColor(ab, editor.color_cfg.highlightFg[curr_fg], 0);
      setColor(ab, editor.color_cfg.highlightBg[curr_bg], 1);

      int j = 0;
      int rx = current_file->col_offset;
      while (rx < rlen) {
        if (iscntrl(c[j]) && c[j] != '\t') {
          char sym = (c[j] <= 26) ? '@' + c[j] : '?';
          abufAppend(ab, ANSI_INVERT);
          abufAppendN(ab, &sym, 1);
          abufAppend(ab, ANSI_CLEAR);
          setColor(ab, editor.color_cfg.highlightFg[curr_fg], 0);
          setColor(ab, editor.color_cfg.highlightBg[curr_bg], 1);

          rx++;
          j++;
        } else {
          uint8_t fg = HL_NORMAL;
          uint8_t bg = HL_BG_NORMAL;

          if (current_file->cursor.is_selected &&
              isPosSelected(i, j + col_offset, range)) {
            bg = HL_BG_SELECT;
          }
          if (CONVAR_GETINT(drawspace) && (c[j] == ' ' || c[j] == '\t')) {
            fg = HL_SPACE;
          }

          // Update color
          if (fg != curr_fg) {
            curr_fg = fg;
            setColor(ab, editor.color_cfg.highlightFg[fg], 0);
          }
          if (bg != curr_bg) {
            curr_bg = bg;
            setColor(ab, editor.color_cfg.highlightBg[bg], 1);
          }

          if (c[j] == '\t') {
            if (CONVAR_GETINT(drawspace)) {
              abufAppend(ab, "|");
            } else {
              abufAppend(ab, " ");
            }

            rx++;
            while (rx % CONVAR_GETINT(tabsize) != 0 && rx < rlen) {
              abufAppend(ab, " ");
              rx++;
            }
            j++;
          } else if (c[j] == ' ') {
            if (CONVAR_GETINT(drawspace)) {
              abufAppend(ab, ".");
            } else {
              abufAppend(ab, " ");
            }
            rx++;
            j++;
          } else {
            size_t byte_size;
            uint32_t unicode = decodeUTF8(&c[j], len - j, &byte_size);
            int width = unicodeWidth(unicode);
            if (width >= 0) {
              rx += width;
              // Make sure double won't exceed the screen
              if (rx <= rlen) abufAppendN(ab, &c[j], byte_size);
            }
            j += byte_size;
          }
        }
      }

      // Add newline character when selected
      if (current_file->cursor.is_selected && range.end_y > i &&
          i >= range.start_y &&
          current_file->row[i].rsize - current_file->col_offset < cols) {
        setColor(ab, editor.color_cfg.highlightBg[HL_BG_SELECT], 1);
        abufAppend(ab, " ");
      }
      setColor(ab, editor.color_cfg.highlightBg[HL_BG_NORMAL], 1);
    }
    if (!is_row_full) abufAppend(ab, "\x1b[K");
    setColor(ab, editor.color_cfg.bg, 1);
  }
}

void editorRefreshScreen(void) {
  abuf ab = ABUF_INIT;

  abufAppend(&ab, "\x1b[?25l");
  abufAppend(&ab, "\x1b[H");

  editorDrawTopStatusBar(&ab);
  editorDrawRows(&ab);

  editorDrawConMsg(&ab);
  editorDrawPrompt(&ab);

  editorDrawStatusBar(&ab);

  bool should_show_cursor = true;
  if (editor.state == EDIT_MODE) {
    int row = (current_file->cursor.y - current_file->row_offset) + 2;
    int col = (editorRowCxToRx(&current_file->row[current_file->cursor.y],
                               current_file->cursor.x) -
               current_file->col_offset) +
              1 + current_file->lineno_width;
    if (row <= 1 || row > editor.screen_rows - 1 || col <= 1 ||
        col > editor.screen_cols ||
        row >= editor.screen_rows - editor.con_size) {
      should_show_cursor = false;
    } else {
      gotoXY(&ab, row, col);
    }
  } else {
    // prompt
    gotoXY(&ab, editor.screen_rows - 1, editor.px + 1);
  }

  if (should_show_cursor) {
    abufAppend(&ab, "\x1b[?25h");
  } else {
    abufAppend(&ab, "\x1b[?25l");
  }

  UNUSED(write(STDOUT_FILENO, ab.buf, ab.len));
  abufFree(&ab);
}
