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
