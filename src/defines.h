#ifndef DEFINES_H
#define DEFINES_H

#define CTRL_KEY(k) ((k)&0x1F)
#define ALT_KEY(k) ((k) | 0x1B00)

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

enum EditorKey {
    UNKNOWN = -1,
    ESC = 27,
    BACKSPACE = 127,
    CHAR_INPUT = 1000,
    ARROW_UP,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN,
    SHIFT_UP,
    SHIFT_DOWN,
    SHIFT_LEFT,
    SHIFT_RIGHT,
    SHIFT_HOME,
    SHIFT_END,
    SHIFT_PAGE_UP,
    SHIFT_PAGE_DOWN,
    ALT_UP,
    ALT_DOWN,
    SHIFT_ALT_UP,
    SHIFT_ALT_DOWN,
    CTRL_UP,
    CTRL_DOWN,
    CTRL_LEFT,
    CTRL_RIGHT,
    CTRL_HOME,
    CTRL_END,
    CTRL_PAGE_UP,
    CTRL_PAGE_DOWN,
    SHIFT_CTRL_UP,
    SHIFT_CTRL_DOWN,
    SHIFT_CTRL_LEFT,
    SHIFT_CTRL_RIGHT,
    SHIFT_CTRL_PAGE_UP,
    SHIFT_CTRL_PAGE_DOWN,
    MOUSE_PRESSED,
    MOUSE_RELEASED,
    SCROLL_PRESSED,
    SCROLL_RELEASED,
    MOUSE_MOVE,
    WHEEL_UP,
    WHEEL_DOWN,
};

enum EditorState {
    EDIT_MODE = 0,
    FIND_MODE,
    GOTO_LINE_MODE,
    OPEN_FILE_MODE,
    CONFIG_MODE,
    SAVE_AS_MODE,
};

#define HL_FG_MASK 0x0F
#define HL_BG_MASK 0xF0
#define HL_FG_BITS 4

enum EditorHighlightFg {
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2,
    HL_KEYWORD3,
    HL_STRING,
    HL_NUMBER,
    HL_SPACE,

    HL_FG_COUNT,
};

enum EditorHighlightBg {
    HL_BG_NORMAL = 0,
    HL_BG_MATCH,
    HL_BG_SELECT,
    HL_BG_TRAILING,

    HL_BG_COUNT,
};

enum EditorField {
    FIELD_EMPTY,
    FIELD_TOP_STATUS,
    FIELD_TEXT,
    FIELD_LINENO,
    FIELD_PROMPT,
    FIELD_STATUS,
    FIELD_ERROR,
};

#endif
