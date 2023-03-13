#include "utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "editor.h"
#include "terminal.h"
#include "unicode.h"

void *malloc_s(size_t size) {
    void *ptr = malloc(size);
    if (!ptr && size != 0)
        PANIC("malloc");
    return ptr;
}

void *calloc_s(size_t n, size_t size) {
    void *ptr = calloc(n, size);
    if (!ptr && size != 0)
        PANIC("calloc");
    return ptr;
}

void *realloc_s(void *ptr, size_t size) {
    ptr = realloc(ptr, size);
    if (!ptr && size != 0)
        PANIC("realloc");
    return ptr;
}

void abufAppend(abuf *ab, const char *s) { abufAppendN(ab, s, strlen(s)); }

void abufAppendN(abuf *ab, const char *s, size_t n) {
    if (n == 0)
        return;

    if (ab->len + n > ab->capacity) {
        ab->capacity += n;
        ab->capacity *= ABUF_GROWTH_RATE;
        char *new = realloc_s(ab->buf, ab->capacity);
        ab->buf = new;
    }

    memcpy(&ab->buf[ab->len], s, n);
    ab->len += n;
}

void abufFree(abuf *ab) { free(ab->buf); }

int editorRowNextUTF8(EditorRow *row, int cx) {
    if (cx < 0)
        return 0;

    if (cx >= row->size)
        return row->size;

    const char *s = &row->data[cx];
    size_t byte_size;
    decodeUTF8(s, row->size - cx, &byte_size);
    return cx + byte_size;
}

int editorRowPreviousUTF8(EditorRow *row, int cx) {
    if (cx <= 0)
        return 0;

    if (cx > row->size)
        return row->size;

    int i = 0;
    size_t byte_size;
    while (i < cx) {
        decodeUTF8(&row->data[i], row->size - i, &byte_size);
        i += byte_size;
    }
    return i - byte_size;
}

int editorRowCxToRx(EditorRow *row, int cx) {
    int rx = 0;
    int i = 0;
    while (i < cx) {
        size_t byte_size;
        uint32_t unicode = decodeUTF8(&row->data[i], row->size - i, &byte_size);
        if (unicode == '\t') {
            rx += (CONVAR_GETINT(tabsize) - 1) - (rx % CONVAR_GETINT(tabsize)) +
                  1;
        } else {
            int width = unicodeWidth(unicode);
            if (width < 0)
                width = 1;
            rx += width;
        }
        i += byte_size;
    }
    return rx;
}

int editorRowRxToCx(EditorRow *row, int rx) {
    int cur_rx = 0;
    int cx = 0;
    while (cx < row->size) {
        size_t byte_size;
        uint32_t unicode =
            decodeUTF8(&row->data[cx], row->size - cx, &byte_size);
        if (unicode == '\t') {
            cur_rx += (CONVAR_GETINT(tabsize) - 1) -
                      (cur_rx % CONVAR_GETINT(tabsize)) + 1;
        } else {
            int width = unicodeWidth(unicode);
            if (width < 0)
                width = 1;
            cur_rx += width;
        }
        if (cur_rx > rx)
            return cx;
        cx += byte_size;
    }
    return cx;
}

static bool isValidColor(const char *color) {
    if (strlen(color) != 6)
        return false;
    for (int i = 0; i < 6; i++) {
        if (!(('0' <= color[i]) || (color[i] <= '9') || ('A' <= color[i]) ||
              (color[i] <= 'F') || ('a' <= color[i]) || (color[i] <= 'f')))
            return false;
    }
    return true;
}

Color strToColor(const char *color) {
    Color result = {0, 0, 0};
    if (!isValidColor(color))
        return result;

    int shift = 16;
    unsigned int hex = strtoul(color, NULL, 16);
    result.r = (hex >> shift) & 0xFF;
    shift -= 8;
    result.g = (hex >> shift) & 0xFF;
    shift -= 8;
    result.b = (hex >> shift) & 0xFF;
    return result;
}

int colorToANSI(Color color, char ansi[32], int is_bg) {
    if (color.r == 0 && color.g == 0 && color.b == 0 && is_bg) {
        return snprintf(ansi, 32, "%s", ANSI_DEFAULT_BG);
    }
    return snprintf(ansi, 32, "\x1b[%d;2;%d;%d;%dm", is_bg ? 48 : 38, color.r,
                    color.g, color.b);
}

int colorToStr(Color color, char buf[8]) {
    return snprintf(buf, 8, "%02x%02x%02x", color.r, color.g, color.b);
}

int isSeparator(char c) {
    return isspace(c) || c == '\0' ||
           strchr("`~!@#$%^&*()-=+[{]}\\|;:'\",.<>/?", c) != NULL;
}

int getDigit(int n) {
    if (n < 10)
        return 1;
    if (n < 100)
        return 2;
    if (n < 1000)
        return 3;
    if (n < 10000000) {
        if (n < 1000000) {
            if (n < 10000)
                return 4;
            return 5 + (n >= 100000);
        }
        return 7;
    }
    if (n < 1000000000)
        return 8 + (n >= 100000000);
    return 10;
}
