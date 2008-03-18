#include "strings.h"
#include <curses.h>
#include <string.h>

#define lengthof(x) (sizeof(x) / sizeof((x)[0]))

typedef struct _trans {
    const char* str;
    int tag;
} trans;

static trans colors[] = {
    {"black",   COLOR_BLACK},
    {"red",     COLOR_RED},
    {"green",   COLOR_GREEN},
    {"yellow",  COLOR_YELLOW},
    {"blue",    COLOR_BLUE},
    {"magenta", COLOR_MAGENTA},
    {"cyan",    COLOR_CYAN},
    {"white",   COLOR_WHITE},
};

static trans modes[] = {
    {"standout",   A_STANDOUT},
    {"underline",  A_UNDERLINE},
    {"reverse",    A_REVERSE},
    {"blink",      A_BLINK},
    {"dim",        A_DIM},
    {"bold",       A_BOLD},
    {"protect",    A_PROTECT},
    {"invis",      A_INVIS},
    {"altcharset", A_ALTCHARSET},
    {"chartext",   A_CHARTEXT},
};

static trans keys[] = {
    {"left",      KEY_LEFT},
    {"right",     KEY_RIGHT},
    {"up",        KEY_UP},
    {"down",      KEY_DOWN},
    {"home",      KEY_HOME},
    {"end",       KEY_END},
    {"backspace", KEY_BACKSPACE},
    {"enter",     KEY_ENTER},
    {"page down", KEY_NPAGE},
    {"page up",   KEY_PPAGE},
    {"break",     KEY_BREAK},
    {"delete",    KEY_DC},
    {"insert",    KEY_IC},
};

/* XXX: the ACS_ defines are actually just indexes into another internal array,
 * which means that we can't use them as initializers. think of a better way
 * to do this. */
/*
static trans chars[] = {
    {"block",    ACS_BLOCK},
    {"board",    ACS_BOARD},
    {"btee",     ACS_BTEE},
    {"bullet",   ACS_BULLET},
    {"ckboard",  ACS_CKBOARD},
    {"darrow",   ACS_DARROW},
    {"degree",   ACS_DEGREE},
    {"diamond",  ACS_DIAMOND},
    {"gequal",   ACS_GEQUAL},
    {"hline",    ACS_HLINE},
    {"lantern",  ACS_LANTERN},
    {"larrow",   ACS_LARROW},
    {"lequal",   ACS_LEQUAL},
    {"llcorner", ACS_LLCORNER},
    {"lrcorner", ACS_LRCORNER},
    {"ltee",     ACS_LTEE},
    {"nequal",   ACS_NEQUAL},
    {"pi",       ACS_PI},
    {"plminus",  ACS_PLMINUS},
    {"plus",     ACS_PLUS},
    {"rarrow",   ACS_RARROW},
    {"rtee",     ACS_RTEE},
    {"s1",       ACS_S1},
    {"s3",       ACS_S3},
    {"s7",       ACS_S7},
    {"s9",       ACS_S9},
    {"sterling", ACS_STERLING},
    {"ttee",     ACS_TTEE},
    {"uarrow",   ACS_UARROW},
    {"ulcorner", ACS_ULCORNER},
    {"urcorner", ACS_URCORNER},
    {"vline",    ACS_VLINE},
};
*/

static const char* fn_keys[] = {
    "F0",  "F1",  "F2",  "F3",  "F4",  "F5",  "F6",  "F7",
    "F8",  "F9",  "F10", "F11", "F12", "F13", "F14", "F15",
    "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23",
    "F24", "F25", "F26", "F27", "F28", "F29", "F30", "F31",
    "F32", "F33", "F34", "F35", "F36", "F37", "F38", "F39",
    "F40", "F41", "F42", "F43", "F44", "F45", "F46", "F47",
    "F48", "F49", "F50", "F51", "F52", "F53", "F54", "F55",
    "F56", "F57", "F58", "F59", "F60", "F61", "F62", "F63",
};

static int str2enum(const trans table[], int len, const char* str)
{
    int i;

    for (i = 0; i < len; ++i) {
        if (!strcmp(str, table[i].str)) {
            return table[i].tag;
        }
    }

    return -1;
}

static const char* enum2str(const trans* table, int len, int tag)
{
    int i;

    for (i = 0; i < len; ++i) {
        if (tag == table[i].tag) {
            return table[i].str;
        }
    }

    return NULL;
}

static void each_item(const trans* table, int len, table_cb cb, void* data)
{
    int i;

    for (i = 0; i < len; ++i) {
        cb(table[i].str, table[i].tag, data);
    }
}

int get_color_enum(const char* str)
{
    return str2enum(colors, lengthof(colors), str);
}

int get_mode_enum(const char* str)
{
    return str2enum(modes, lengthof(modes), str);
}

int get_key_enum(const char* str)
{
    int ret;

    ret = str2enum(keys, lengthof(keys), str);

    if (ret == -1) {
        int fkey;
        if (sscanf(str, "F%d", &fkey) == 1) {
            return KEY_F(fkey);
        }
    }

    return ret == -1 ? (int)str[0] : ret;
}

int get_char_enum(const char* str)
{
    /*
    int ret;

    ret = str2enum(chars, lengthof(chars), str);

    return ret == -1 ? (int)str[0] : ret;
    */
    return (int)str[0];
}

const char* get_color_str(int tag)
{
    return enum2str(colors, lengthof(colors), tag);
}

const char* get_mode_str(int tag)
{
    return enum2str(modes, lengthof(modes), tag);
}

const char* get_key_str(int tag)
{
    if (tag >= KEY_F(0) && tag <= KEY_F(63)) {
        return fn_keys[tag - KEY_F0];
    }

    return enum2str(keys, lengthof(keys), tag);
}

const char* get_char_str(int tag)
{
    /*
    return enum2str(chars, lengthof(chars), tag);
    */
    return NULL;
}

void each_color(table_cb cb, void* data)
{
    each_item(colors, lengthof(colors), cb, data);
}

void each_mode(table_cb cb, void* data)
{
    each_item(modes, lengthof(modes), cb, data);
}

void each_key(table_cb cb, void* data)
{
    each_item(keys, lengthof(keys), cb, data);
}

void each_char(table_cb cb, void* data)
{
    /*
    each_item(chars, lengthof(chars), cb, data);
    */
}
