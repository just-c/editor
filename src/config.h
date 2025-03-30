#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#include "defines.h"
#include "utils.h"

typedef struct EditorColorScheme EditorColorScheme;
extern const EditorColorScheme color_default;

#define EDITOR_COLOR_COUNT 34

typedef struct {
  const char* label;
  Color* color;
} ColorElement;

extern const ColorElement color_element_map[EDITOR_COLOR_COUNT];

struct EditorColorScheme {
  Color bg;
  Color top_status[6];
  Color prompt[2];
  Color status[6];
  Color line_number[2];
  Color cursor_line;
  Color highlightFg[HL_FG_COUNT];
  Color highlightBg[HL_BG_COUNT];
};

#endif
