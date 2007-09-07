require "curses"

curses.initscr();
curses.start_color();
curses.setup_term{nl = false, cbreak = true, echo = false, keypad = true}
curses.init_pair("black", "black")
curses.init_pair("green", "green")
curses.init_pair("red", "red")
curses.init_pair("cyan", "cyan")
curses.init_pair("white", "white")
curses.init_pair("magenta", "magenta")
curses.init_pair("blue", "blue")
curses.init_pair("yellow", "yellow")

local x, y = 0, 0
local maxy, maxx = curses.getmaxyx()
while true do
    local c = curses.getch()
    if c == "left" and x > 0 then
        x = x - 1
    elseif c == "right" and x < maxx then
        x = x + 1
    elseif c == "up" and y > 0 then
        y = y - 1
    elseif c == "down" and y < maxy then
        y = y + 1
    elseif #c == 1 then
        curses.addstr(c)
    end
    curses.move(y, x)
end
