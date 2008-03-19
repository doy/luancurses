#include "strings.h"
#include <curses.h>
#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

#define REG_TABLE "luancurses"

#define NO_ARG_FUNCTION(name) \
static int l_##name(lua_State* L) \
{ \
    lua_pushboolean(L, name() == OK); \
    return 1; \
}

typedef struct _pos {
    int x;
    int y;
} pos;

static int ncolors = 0, ncolor_pairs = 0, default_color_available = 0;

static int get_color_pair(lua_State* L, const char* str)
{
    int ret = -1;

    lua_getfield(L, LUA_REGISTRYINDEX, REG_TABLE);
    lua_getfield(L, -1, "color_pairs");
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_isstring(L, -2)) {
            const char* key;

            key = lua_tostring(L, -2);
            if (!strcmp(key, str)) {
                ret = lua_tointeger(L, -1);
                lua_pop(L, 2);
                break;
            }
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 2);

    return ret;
}

static void init_color_pairs(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, REG_TABLE);
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "default");
    lua_setfield(L, -2, "color_pairs");
    lua_pop(L, 1);
}

static void register_color(const char* color_str, int color_tag, void* data)
{
    lua_pushinteger((lua_State*)data, color_tag);
    lua_setfield((lua_State*)data, -2, color_str);
    ncolors++;
}

static void register_default_color(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, REG_TABLE);
    lua_getfield(L, -1, "colors");
    lua_pushinteger(L, -1);
    lua_setfield(L, -2, "default");
    lua_pop(L, 2);
    default_color_available = 1;
}

static void init_colors(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, REG_TABLE);
    lua_newtable(L);
    each_color(register_color, L);
    lua_setfield(L, -2, "colors");
    lua_pop(L, 1);
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
    val = get_color_pair(L, str);
    if (val == -1) {
        return luaL_error(L, "Unknown color pair \"%s\"", str);
    }

    return COLOR_PAIR(val);
}

static int get_char_attr(lua_State* L, int stack_pos)
{
    int mode = A_NORMAL;

    lua_pushnil(L);
    while (lua_next(L, stack_pos) != 0) {
        if (lua_isstring(L, -2)) {
            const char* str;

            str = lua_tostring(L, -2);
            if (!strcmp(str, "color")) {
                mode |= get_char_color(L, stack_pos);
            }
            else {
                int cur_mode;

                cur_mode = get_mode_enum(str);
                if (cur_mode == -1) {
                    return luaL_error(L, "Unknown attribute \"%s\"", str);
                }

                lua_toboolean(L, -1) ? (mode |= cur_mode) : (mode &= ~cur_mode);
            }
        }
        lua_pop(L, 1);
    }

    return mode;
}

NO_ARG_FUNCTION(initscr)
NO_ARG_FUNCTION(endwin)
NO_ARG_FUNCTION(erase)
NO_ARG_FUNCTION(clear)
NO_ARG_FUNCTION(clrtobot)
NO_ARG_FUNCTION(clrtoeol)
NO_ARG_FUNCTION(deleteln)
NO_ARG_FUNCTION(insertln)
NO_ARG_FUNCTION(refresh)
NO_ARG_FUNCTION(beep)
NO_ARG_FUNCTION(flash)

static int l_isendwin(lua_State* L)
{
    lua_pushboolean(L, isendwin());
    return 1;
}

static int l_start_color(lua_State* L)
{
    if (has_colors()) {
        init_color_pairs(L);
        init_colors(L);
        lua_pushboolean(L, start_color() == OK);
    }
    else {
        lua_pushboolean(L, FALSE);
    }

    return 1;
}

static int l_use_default_colors(lua_State* L)
{
    register_default_color(L);
    lua_pushboolean(L, use_default_colors() == OK);
    return 1;
}

static int l_assume_default_colors(lua_State* L)
{
    const char* fg;
    const char* bg;
    int nfg, nbg;

    register_default_color(L);

    fg = luaL_optlstring(L, 1, "default", NULL);
    bg = luaL_optlstring(L, 2, "default", NULL);

    lua_getfield(L, LUA_REGISTRYINDEX, REG_TABLE);
    lua_getfield(L, -1, "colors");
    lua_getfield(L, -1, fg);
    nfg = luaL_checkint(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, bg);
    nbg = luaL_checkint(L, -1);
    lua_pop(L, 3);

    lua_pushboolean(L, assume_default_colors(nfg, nbg) == OK);
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
            if (!strcmp(str, "cbreak")) {
                ret += ((lua_toboolean(L, -1) ? cbreak() : nocbreak()) == OK);
            }
            else if (!strcmp(str, "echo")) {
                ret += ((lua_toboolean(L, -1) ? echo() : noecho()) == OK);
            }
            else if (!strcmp(str, "halfdelay")) {
                ret += (halfdelay(lua_tointeger(L, -1)) == OK);
            }
            else if (!strcmp(str, "intrflush")) {
                ret += (intrflush(stdscr, lua_toboolean(L, -1)) == OK);
            }
            else if (!strcmp(str, "keypad")) {
                ret += (keypad(stdscr, lua_toboolean(L, -1)) == OK);
            }
            else if (!strcmp(str, "meta")) {
                ret += (meta(stdscr, lua_toboolean(L, -1)) == OK);
            }
            else if (!strcmp(str, "nodelay")) {
                ret += (nodelay(stdscr, lua_toboolean(L, -1)) == OK);
            }
            else if (!strcmp(str, "raw")) {
                ret += ((lua_toboolean(L, -1) ? raw() : noraw()) == OK);
            }
            else if (!strcmp(str, "qiflush")) {
                lua_toboolean(L, -1) ? qiflush() : noqiflush();
                ret++;
            }
            else if (!strcmp(str, "timeout")) {
                if (lua_isnil(L, -1)) {
                    ret += (notimeout(stdscr, TRUE) == OK);
                }
                else {
                    timeout(lua_tointeger(L, -1));
                    ret++;
                }
            }
            else if (!strcmp(str, "typeahead")) {
                ret += ((lua_toboolean(L, -1) ? typeahead(0) :
                                                typeahead(-1)) == OK);
            }
            else if (!strcmp(str, "clear")) {
                ret += (clearok(stdscr, lua_toboolean(L, -1)) == OK);
            }
            else if (!strcmp(str, "idl")) {
                ret += (idlok(stdscr, lua_toboolean(L, -1)) == OK);
            }
            else if (!strcmp(str, "idc")) {
                idcok(stdscr, lua_toboolean(L, -1));
                ret++;
            }
            else if (!strcmp(str, "immed")) {
                immedok(stdscr, lua_toboolean(L, -1));
                ret++;
            }
            else if (!strcmp(str, "leave")) {
                ret += (leaveok(stdscr, lua_toboolean(L, -1)) == OK);
            }
            else if (!strcmp(str, "scroll")) {
                ret += (scrollok(stdscr, lua_toboolean(L, -1)) == OK);
            }
            else if (!strcmp(str, "nl")) {
                ret += ((lua_toboolean(L, -1) ? nl() : nonl()) == OK);
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
    const char* name;
    int color_val;
    short r, g, b;

    if (can_change_color() == FALSE) {
        lua_pushboolean(L, FALSE);
        return 1;
    }

    name = luaL_checklstring(L, 1, NULL);

    lua_getfield(L, LUA_REGISTRYINDEX, REG_TABLE);
    lua_getfield(L, -1, "colors");
    lua_getfield(L, -1, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushinteger(L, ++ncolors);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, name);
    }
    color_val = lua_tointeger(L, -1);
    lua_pop(L, 2);

    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "r");
        lua_getfield(L, 2, "g");
        lua_getfield(L, 2, "b");
        r = luaL_checkint(L, -3) * 1000 / 256;
        g = luaL_checkint(L, -2) * 1000 / 256;
        b = luaL_checkint(L, -1) * 1000 / 256;
        lua_pop(L, 3);
    }
    else {
        r = luaL_checkint(L, 2) * 1000 / 256;
        g = luaL_checkint(L, 3) * 1000 / 256;
        b = luaL_checkint(L, 4) * 1000 / 256;
    }

    lua_pushboolean(L, init_color(color_val, r, g, b) == OK);

    return 1;
}

static int l_init_pair(lua_State* L)
{
    const char *name, *fg, *bg;
    int name_val, fg_val, bg_val;

    /* check the arguments, and get them */
    name = luaL_checklstring(L, 1, NULL);
    if (default_color_available) {
        fg =   luaL_optlstring(L, 2, "default", NULL);
        bg =   luaL_optlstring(L, 3, "default", NULL);
    }
    else {
        fg =   luaL_optlstring(L, 2, "white", NULL);
        bg =   luaL_optlstring(L, 3, "black", NULL);
    }

    lua_getfield(L, LUA_REGISTRYINDEX, REG_TABLE);

    /* figure out which pair value to use */
    lua_getfield(L, -1, "color_pairs");
    lua_getfield(L, -1, name);
    if (lua_isnil(L, -1)) {
        /* if it was nil, we want to set a new value in the color_pairs table,
         * and we want to leave that C color_pair value on top of the stack
         * for consistency */
        lua_pop(L, 1);
        lua_pushinteger(L, ++ncolor_pairs);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, name);
    }
    name_val = lua_tointeger(L, -1);
    lua_pop(L, 2);

    /* figure out which foreground value to use */
    lua_getfield(L, -1, "colors");
    lua_getfield(L, -1, fg);
    if (lua_isnil(L, -1)) {
        return luaL_error(L, "init_pair: Trying to use a non-existant foreground color: \"%s\"", fg);
    }
    fg_val = lua_tointeger(L, -1);
    lua_pop(L, 1);

    /* and background value */
    lua_getfield(L, -1, bg);
    if (lua_isnil(L, -1)) {
        return luaL_error(L, "init_pair: Trying to use a non-existant background color: \"%s\"", bg);
    }
    bg_val = lua_tointeger(L, -1);
    lua_pop(L, 3);

    if (name_val != 0) {
        lua_pushboolean(L, (init_pair(name_val, fg_val, bg_val) == OK));
    }
    else {
        lua_pushboolean(L, (assume_default_colors(fg_val, bg_val) == OK));
    }
    return 1;
}

static int l_getch(lua_State* L)
{
    int c;
    pos p;
    const char* key_name;

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

    key_name = get_key_str(c);

    if (key_name == NULL) {
        char s;

        s = c;
        lua_pushlstring(L, &s, 1);
    }
    else {
        lua_pushstring(L, key_name);
    }

    return 1;
}

static int l_ungetch(lua_State* L)
{
    const char* ch_str;
    int ch;

    ch_str = luaL_checklstring(L, 1, NULL);
    ch = get_key_enum(ch_str);

    lua_pushboolean(L, ungetch(ch) == OK);
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

        y = luaL_checkint(L, 1);
        x = luaL_checkint(L, 2);

        lua_pushboolean(L, (move(y, x) == OK));
    }

    return 1;
}

static int l_addch(lua_State* L)
{
    int is_mv;
    pos p;
    chtype ch;

    is_mv = get_pos(L, &p);
    ch = get_char_enum(luaL_checklstring(L, 1, NULL));
    if (lua_istable(L, 2)) {
        ch |= get_char_attr(L, 2);
    }

    if (is_mv) {
        lua_pushboolean(L, mvaddch(p.y, p.x, ch) == OK);
    }
    else {
        lua_pushboolean(L, addch(ch) == OK);
    }

    return 1;
}

static int l_echochar(lua_State* L)
{
    int is_mv;
    pos p;
    chtype ch;

    is_mv = get_pos(L, &p);
    ch = get_char_enum(luaL_checklstring(L, 1, NULL));
    if (lua_istable(L, 2)) {
        ch |= get_char_attr(L, 2);
    }

    if (is_mv) {
        int ret;

        ret = move(p.y, p.x);
        if (ret == OK) {
            ret = echochar(ch);
        }

        lua_pushboolean(L, ret == OK);
    }
    else {
        lua_pushboolean(L, echochar(ch) == OK);
    }

    return 1;
}

static int l_addstr(lua_State* L)
{
    int is_mv, set_attrs = 0;
    pos p;
    const char* str;
    attr_t old_mode = 0;
    short old_color = 0;

    is_mv = get_pos(L, &p);
    str = luaL_checklstring(L, 1, NULL);
    if (lua_istable(L, 2)) {
        int new_mode, new_color;

        set_attrs = 1;
        attr_get(&old_mode, &old_color, NULL);
        new_mode = get_char_attr(L, 2);
        new_color = PAIR_NUMBER(new_mode);
        new_mode &= A_ATTRIBUTES & ~A_COLOR;
        attr_set(new_mode, new_color, NULL);
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

static int l_delch(lua_State* L)
{
    pos p;

    if (get_pos(L, &p)) {
        lua_pushboolean(L, mvdelch(p.y, p.x) == OK);
    }
    else {
        lua_pushboolean(L, delch() == OK);
    }

    return 1;
}

static int l_insch(lua_State* L)
{
    int is_mv;
    pos p;
    chtype ch;

    is_mv = get_pos(L, &p);
    ch = get_char_enum(luaL_checklstring(L, 1, NULL));
    if (lua_istable(L, 2)) {
        ch |= get_char_attr(L, 2);
    }

    if (is_mv) {
        lua_pushboolean(L, mvinsch(p.y, p.x, ch) == OK);
    }
    else {
        lua_pushboolean(L, insch(ch) == OK);
    }

    return 1;
}

static int l_insstr(lua_State* L)
{
    int is_mv, set_attrs = 0;
    pos p;
    const char* str;
    attr_t old_mode = 0;
    short old_color = 0;

    is_mv = get_pos(L, &p);
    str = luaL_checklstring(L, 1, NULL);
    if (lua_istable(L, 2)) {
        int new_mode, new_color;

        set_attrs = 1;
        attr_get(&old_mode, &old_color, NULL);
        new_mode = get_char_attr(L, 2);
        new_color = PAIR_NUMBER(new_mode);
        new_mode &= A_ATTRIBUTES & ~A_COLOR;
        attr_set(new_mode, new_color, NULL);
    }

    if (is_mv) {
        lua_pushboolean(L, mvinsstr(p.y, p.x, str) == OK);
    }
    else {
        lua_pushboolean(L, insstr(str) == OK);
    }

    if (set_attrs) {
        attr_set(old_mode, old_color, NULL);
    }

    return 1;
}

static int l_insdelln(lua_State* L)
{
    int n;

    n = luaL_checkint(L, 1);

    lua_pushboolean(L, (insdelln(n) == OK));
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
    { "endwin", l_endwin },
    { "isendwin", l_isendwin },
    { "start_color", l_start_color },
    { "use_default_colors", l_use_default_colors },
    { "assume_default_colors", l_assume_default_colors },
    { "setup_term", l_setup_term },
    { "init_color", l_init_color },
    { "init_pair", l_init_pair },
    { "getch", l_getch },
    { "ungetch", l_ungetch },
    { "move", l_move },
    { "addch", l_addch },
    { "echochar", l_echochar },
    { "addstr", l_addstr },
    { "erase", l_erase },
    { "clear", l_clear },
    { "clrtobot", l_clrtobot },
    { "clrtoeol", l_clrtoeol },
    { "delch", l_delch },
    { "deleteln", l_deleteln },
    { "insch", l_insch },
    { "insstr", l_insstr },
    { "insdelln", l_insdelln },
    { "insertln", l_insertln },
    { "refresh", l_refresh },
    { "getmaxyx", l_getmaxyx },
    { "getyx", l_getyx },
    { "colors", l_colors },
    { "color_pairs", l_color_pairs },
    { "beep", l_beep },
    { "flash", l_flash },
    { NULL, NULL },
};

extern int luaopen_curses(lua_State* L)
{
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, REG_TABLE);

    luaL_register(L, "curses", reg);
    lua_getglobal(L, "curses");
    lua_pushstring(L, "LuaNcurses 0.02");
    lua_setfield(L, -2, "_VERSION");

    return 1;
}
