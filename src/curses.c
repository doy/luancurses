#include <curses.h>
#include <lua.h>
#include <lauxlib.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define REG_TABLE "luancurses"

typedef struct _pos {
    int x;
    int y;
} pos;

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
};

static int ncolors = 1, ncolor_pairs = 1;

/* necessary because atexit() expects a function that returns void, and gcc
 * whines otherwise */
static void _endwin(void)
{
    endwin();
}

static void init_colors(lua_State* L)
{
    int i;

    for (i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i) {
        lua_pushinteger(L, colors[i].tag);
        lua_setfield(L, -2, colors[i].str);
    }

    ncolors = 8;
}

static int get_pos(lua_State* L, pos* p)
{
    if (!lua_istable(L, 1)) {
        return 0;
    }

    getyx(stdscr, p->y, p->x);

    lua_getfield(L, 1, "x");
    if (lua_isnumber(L, -1)) {
        p->x = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "y");
    if (lua_isnumber(L, -1)) {
        p->y = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);

    lua_remove(L, 1);

    return 1;
}

static int get_mode(lua_State* L, const char* str)
{
    int i;

    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        if (!strcmp(str, modes[i].str)) {
            return modes[i].tag;
        }
    }

    return A_NORMAL;
}

static chtype get_char(const char* str)
{
    /* add the ACS_ defines here */
    return str[0];
}

static int get_char_attr(lua_State* L, int stack_pos)
{
    int mode = A_NORMAL;

    lua_pushnil(L);
    while (lua_next(L, stack_pos) != 0) {
        if (lua_isstring(L, -2)) {
            const char* str;

            str = lua_tostring(L, -2);
            lua_toboolean(L, -1) ?
                (mode |= get_mode(L, str)) : (mode &= ~get_mode(L, str));
        }
        lua_pop(L, 1);
    }

    return mode;
}

static int get_char_color(lua_State* L, int stack_pos)
{
    const char* str;
    int val = -1;

    lua_getfield(L, stack_pos, "color");
    if (!lua_isstring(L, -1)) {
        return 0;
    }
    str = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, REG_TABLE);
    lua_getfield(L, -1, "color_pairs");
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_isstring(L, -2)) {
            const char* key;

            key = lua_tostring(L, -2);
            if (!strcmp(key, str)) {
                val = lua_tointeger(L, -1);
            }
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 2);

    if (val == -1) {
        return luaL_error(L, "Unknown color_pair %s", str);
    }

    return COLOR_PAIR(val);
}

static int l_initscr(lua_State* L)
{
    lua_pushboolean(L, initscr() == OK);
    return 1;
}

static int l_start_color(lua_State* L)
{
    if (has_colors()) {
        lua_getfield(L, LUA_REGISTRYINDEX, REG_TABLE);
        lua_newtable(L);
        lua_setfield(L, -2, "color_pairs");
        lua_newtable(L);
        init_colors(L);
        lua_setfield(L, -2, "colors");
        lua_setfield(L, LUA_REGISTRYINDEX, REG_TABLE);
        lua_pushboolean(L, start_color() == OK);
    }
    else {
        lua_pushboolean(L, FALSE);
    }

    return 1;
}

static int l_setup_term(lua_State* L)
{
    int ret = 0;

    luaL_checktype(L, 1, LUA_TTABLE);

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        if (lua_isstring(L, -2)) {
            const char* str;

            str = lua_tostring(L, -2);
            /* XXX: this certainly needs expansion */
            if (!strcmp(str, "nl")) {
                ret += ((lua_toboolean(L, -1) ? nl() : nonl()) == OK);
            }
            else if (!strcmp(str, "cbreak")) {
                ret += ((lua_toboolean(L, -1) ? cbreak() : nocbreak()) == OK);
            }
            else if (!strcmp(str, "echo")) {
                ret += ((lua_toboolean(L, -1) ? echo() : noecho()) == OK);
            }
            else if (!strcmp(str, "keypad")) {
                ret += (keypad(stdscr, lua_toboolean(L, -1)) == OK);
            }
            else if (!strcmp(str, "scroll")) {
                ret += (scrollok(stdscr, lua_toboolean(L, -1)) == OK);
            }
            else {
                luaL_error(L, "Unknown or unimplemented terminal mode %s", str);
            }
        }
        lua_pop(L, 1);
    }

    lua_pushnumber(L, ret);
    return 1;
}

static int l_init_color(lua_State* L)
{
    /* test can_change_color here */
    return 0;
}

static int l_init_pair(lua_State* L)
{
    const char *name, *fg, *bg;
    int name_val, fg_val, bg_val;

    /* check the arguments, and get them */
    name = luaL_checklstring(L, 1, NULL);
    fg =   luaL_optlstring(L, 2, "white", NULL);
    bg =   luaL_optlstring(L, 3, "black", NULL);

    lua_getfield(L, LUA_REGISTRYINDEX, REG_TABLE);

    /* figure out which pair value to use */
    lua_getfield(L, -1, "color_pairs");
    lua_getfield(L, -1, name);
    if (lua_isnil(L, -1)) {
        /* if it was nil, we want to set a new value in the color_pairs table,
         * and we want to leave that C color_pair value on top of the stack
         * for consistency */
        lua_pop(L, 1);
        lua_pushinteger(L, ncolor_pairs++);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, name);
    }
    name_val = lua_tointeger(L, -1);
    lua_pop(L, 2);

    /* figure out which foreground value to use */
    lua_getfield(L, -1, "colors");
    lua_getfield(L, -1, fg);
    if (lua_isnil(L, -1)) {
        return luaL_error(L, "init_pair: Trying to use a non-existant foreground color");
    }
    fg_val = lua_tointeger(L, -1);
    lua_pop(L, 1);

    /* and background value */
    lua_getfield(L, -1, bg);
    if (lua_isnil(L, -1)) {
        return luaL_error(L, "init_pair: Trying to use a non-existant background color");
    }
    bg_val = lua_tointeger(L, -1);
    lua_pop(L, 3);

    lua_pushboolean(L, (init_pair(name_val, fg_val, bg_val) == OK));
    return 1;
}

static int l_getch(lua_State* L)
{
    int c, i, found = 0;
    pos p;

    if (get_pos(L, &p)) {
        c = mvgetch(p.y, p.x);
    }
    else {
        c = getch();
    }
    if (c == ERR) {
        lua_pushboolean(L, 0);
        return 1;
    }

    for (i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        if (c == keys[i].tag) {
            lua_pushstring(L, keys[i].str);
            found = 1;
            break;
        }
    }

    if (!found) {
        if (c >= KEY_F(1) && c <= KEY_F(64)) {
            lua_pushfstring(L, "F%d", c - KEY_F0);
        }
        else {
            char s[1];

            s[0] = c;
            lua_pushlstring(L, s, 1);
        }
    }

    return 1;
}

static int l_move(lua_State* L)
{
    pos p;

    if (get_pos(L, &p)) {
        lua_pushboolean(L, (move(p.y, p.x) == OK));
    }
    else {
        int x, y;

        y = luaL_checkinteger(L, 1);
        x = luaL_checkinteger(L, 2);

        lua_pushboolean(L, (move(y, x) == OK));
    }

    return 1;
}

static int l_addch(lua_State* L)
{
    int is_mv;
    pos p;
    size_t l;
    attr_t mode = 0;
    short color = 0;
    chtype ch;

    is_mv = get_pos(L, &p);
    ch = get_char(luaL_checklstring(L, 1, &l));
    if (lua_istable(L, 2)) {
        mode = get_char_attr(L, 2);
        color = get_char_color(L, 2);
    }

    if (is_mv) {
        lua_pushboolean(L, mvaddch(p.y, p.x, ch | mode | color) == OK);
    }
    else {
        lua_pushboolean(L, addch(ch | mode | color) == OK);
    }

    return 1;
}

static int l_addstr(lua_State* L)
{
    int is_mv, set_attrs = 0;
    pos p;
    size_t l;
    const char* str;
    attr_t old_mode = 0;
    short old_color = 0;

    is_mv = get_pos(L, &p);
    str = luaL_checklstring(L, 1, &l);
    if (lua_istable(L, 2)) {
        set_attrs = 1;
        attr_get(&old_mode, &old_color, NULL);
        attr_set(get_char_attr(L, 2), get_char_color(L, 2), NULL);
    }

    if (is_mv) {
        lua_pushboolean(L, mvaddstr(p.y, p.x, str) == OK);
    }
    else {
        lua_pushboolean(L, addstr(str) == OK);
    }

    if (set_attrs) {
        attr_set(old_mode, old_color, NULL);
    }

    return 1;
}

static int l_refresh(lua_State* L)
{
    lua_pushboolean(L, (refresh() == OK));
    return 1;
}

static int l_getmaxyx(lua_State* L)
{
    int x, y;

    getmaxyx(stdscr, y, x);

    lua_pushnumber(L, y);
    lua_pushnumber(L, x);
    return 2;
}

static int l_getyx(lua_State* L)
{
    int x, y;

    getyx(stdscr, y, x);

    lua_pushnumber(L, y);
    lua_pushnumber(L, x);
    return 2;
}

static int l_colors(lua_State* L)
{
    lua_pushinteger(L, COLORS);
    return 1;
}

static int l_color_pairs(lua_State* L)
{
    lua_pushinteger(L, COLOR_PAIRS);
    return 1;
}

const luaL_Reg reg[] = {
    { "initscr", l_initscr },
    { "start_color", l_start_color },
    { "setup_term", l_setup_term },
    { "init_color", l_init_color },
    { "init_pair", l_init_pair },
    { "getch", l_getch },
    { "move", l_move },
    { "addch", l_addch },
    { "addstr", l_addstr },
    { "refresh", l_refresh },
    { "getmaxyx", l_getmaxyx },
    { "getyx", l_getyx },
    { "colors", l_colors },
    { "color_pairs", l_color_pairs },
    { NULL, NULL },
};

extern int luaopen_curses(lua_State* L)
{
    /* XXX: do we want to do this? how important is cleaning up? */
    signal(SIGTERM, exit);
    atexit(_endwin);

    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, REG_TABLE);

    luaL_register(L, "curses", reg);

    return 1;
}