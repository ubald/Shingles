#pragma once

#include <ostream>

/**
 * http://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
 * http://pastebin.com/zWC3t9hC
 */
namespace Color {
    enum Code {
        FG_DEFAULT = 39,
        FG_BLACK = 30,
        FG_RED = 31,
        FG_GREEN = 32,
        FG_YELLOW = 33,
        FG_BLUE = 34,
        FG_MAGENTA = 35,
        FG_CYAN = 36,
        FG_LIGHT_GRAY = 37,
        FG_DARK_GRAY = 90,
        FG_LIGHT_RED = 91,
        FG_LIGHT_GREEN = 92,
        FG_LIGHT_YELLOW = 93,
        FG_LIGHT_BLUE = 94,
        FG_LIGHT_MAGENTA = 95,
        FG_LIGHT_CYAN = 96,
        FG_WHITE = 97,
        BG_RED = 41,
        BG_GREEN = 42,
        BG_YELLOW = 43,
        BG_BLUE = 44,
        BG_MAGENTA = 45,
        BG_CYAN = 46,
        BG_WHITE = 47,
        BG_DEFAULT = 49
    };
    std::ostream& operator<<(std::ostream& os, Code code);
}