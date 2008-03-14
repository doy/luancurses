require "curses"
require "signal"

local function cleanup(sig)
    curses.clear()
    curses.endwin()
    if sig then
        signal.signal(sig, "default")
        signal.raise(sig)
    end
end
curses.initscr()
signal.signal("INT", cleanup)
signal.signal("TERM", cleanup)
curses.start_color()
curses.setup_term{nl = false, cbreak = true, echo = false, keypad = true}
local colors = {"black", "green", "red", "cyan", "white", "magenta", "blue", "yellow"}
for _, color in ipairs(colors) do
    curses.init_pair(color, color)
end

local x, y = 0, 0
local maxy, maxx = curses.getmaxyx()
while true do
    local c = curses.getch()
    if c == "left" and x > 0 then
        x = x - 1
    elseif c == "right" and x < maxx - 1 then
        x = x + 1
    elseif c == "up" and y > 0 then
        y = y - 1
    elseif c == "down" and y < maxy - 1 then
        y = y + 1
    elseif #c == 1 then
        curses.addch(c, {color = colors[math.random(#colors)]})
    end
    curses.move(y, x)
end
