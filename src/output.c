#include "output.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "defines.h"
#include "editor.h"
#include "highlight.h"
#include "select.h"
#include "status.h"

void editorScroll() {
    E.rx = 0;
    if (E.cy < E.num_rows) {
        E.rx = editorRowCxToRx(&(E.row[E.cy]), E.cx);
    }

    if (E.cy < E.row_offset) {
        E.row_offset = E.cy;
    }
    if (E.cy >= E.row_offset + E.rows) {
        E.row_offset = E.cy - E.rows + 1;
    }
    if (E.rx < E.col_offset) {
        E.col_offset = E.rx;
    }
    if (E.rx >= E.col_offset + E.cols) {
        E.col_offset = E.rx - E.cols + 1;
    }
}

void editorDrawRows(abuf* ab) {
    editorSelectText();
    for (int i = 0; i < E.rows; i++) {
        int current_row = i + E.row_offset;
        if (current_row < E.num_rows) {
            char line_number[16];
            if (current_row == E.cy) {
                abufAppend(ab, "\x1b[30;100m");
            } else {
                abufAppend(ab, "\x1b[90m");
            }
            snprintf(line_number, sizeof(line_number), "%*d ",
                     E.num_rows_digits, current_row + 1);
            abufAppend(ab, line_number);
            abufAppend(ab, ANSI_CLEAR);
            int len = E.row[current_row].rsize - E.col_offset;
            if (len < 0)
                len = 0;
            if (len > E.cols)
                len = E.cols;
            char* c = &(E.row[current_row].render[E.col_offset]);
            unsigned char* hl = &(E.row[current_row].hl[E.col_offset]);
            unsigned char* selected =
                &(E.row[current_row].selected[E.col_offset]);

            unsigned char current_color = HL_NORMAL;
            int has_bg = 0;
            char buf[32];
            colorToANSI(E.cfg->highlight_color[current_color], buf, 0);
            abufAppend(ab, buf);
            for (int j = 0; j < len; j++) {
                if (iscntrl(c[j])) {
                    char sym = (c[j] <= 26) ? '@' + c[j] : '?';
                    abufAppend(ab, ANSI_INVERT);
                    abufAppendN(ab, &sym, 1);
                    abufAppend(ab, ANSI_CLEAR);
                    if (current_color >= 0) {
                        colorToANSI(E.cfg->highlight_color[current_color], buf,
                                    0);
                        abufAppend(ab, buf);
                    }
                } else {
                    unsigned char color = hl[j];
                    if (E.is_selected && selected[j]) {
                        if (!has_bg) {
                            has_bg = 1;
                            colorToANSI(E.cfg->highlight_color[HL_SELECT], buf,
                                        1);
                            abufAppend(ab, buf);
                        }
                    } else if (has_bg && color != HL_MATCH) {
                        abufAppend(ab, ANSI_DEFAULT_BG);
                    }
                    if (color != current_color) {
                        current_color = color;
                        abufAppend(ab, ANSI_DEFAULT_FG);
                        if (color == HL_MATCH) {
                            has_bg = 1;
                            colorToANSI(E.cfg->highlight_color[HL_NORMAL], buf,
                                        0);
                            colorToANSI(E.cfg->highlight_color[color], buf, 1);
                        } else {
                            colorToANSI(E.cfg->highlight_color[color], buf, 0);
                        }
                        abufAppend(ab, buf);
                    }
                    abufAppendN(ab, &c[j], 1);
                }
            }
            abufAppend(ab, ANSI_CLEAR);
        }
        abufAppend(ab, "\x1b[K");
        abufAppend(ab, "\r\n");
    }
}

int editorRefreshScreen() {
    abuf ab = ABUF_INIT;

    abufAppend(&ab, "\x1b[?25l");
    abufAppend(&ab, "\x1b[H");

    editorDrawTopStatusBar(&ab);
    editorDrawRows(&ab);
    editorDrawStatusMsgBar(&ab);
    editorDrawStatusBar(&ab);

    char buf[32];
    if (E.state == EDIT_MODE) {
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.row_offset) + 2,
                 (E.rx - E.col_offset) + 1 + E.num_rows_digits + 1);
    } else {
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.rows + 2, E.px + 1);
    }

    abufAppend(&ab, buf);

    abufAppend(&ab, "\x1b[?25h");

    int success = write(STDOUT_FILENO, ab.buf, ab.len) == ab.len;
    abufFree(&ab);

    return success;
}
