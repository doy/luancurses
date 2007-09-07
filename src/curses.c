#include <curses.h>
#include <lua.h>

/* necessary because atexit() expects a function that returns void, and gcc
 * whines otherwise */
static void _endwin(void)
{
    endwin();
}

static int l_initialize(lua_State* L)
{
    int ret;

    /* XXX: do we want to do this? how important is cleaning up? */
    signal(SIGTERM, exit);
    atexit(_endwin);
    ret = initscr();
    if (has_colors()) {
        start_color();
    }

    lua_pushboolean(L, ret == OK);
    return 1;
}

static int l_initscr(lua_State* L)
{
    lua_pushboolean(L, initscr() == OK);
    return 1;
}

static int l_setup_term(lua_State* L)
{
    int ret = 0;

    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushnil(L);
    while (lua_next(L, t) != 0) {
        if (lua_isstring(L, -2)) {
            char* str;

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
                ret += ((keypad(stdscr, lua_toboolean(L, -1))) == OK);
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

static int l_init_pair(lua_State* L)
{
}

static int l_getch(lua_State* L)
{
}

static int l_move(lua_State* L)
{
}

static int l_addch(lua_State* L)
{
}

static int l_refresh(lua_State* L)
{
}

int luaopen_ncurses(lua_State* L)
{
}
