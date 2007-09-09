require 'curses'

-- globals {{{
local map, rows, cols
local colors = {
    "black", "green",
    "red",   "cyan",
    "white", "magenta",
    "blue",  "yellow"
}
-- }}}

-- helpful functions {{{
local function pline(str)
    curses.addstr({y = 0, x = 0}, str)
    curses.clrtoeol()
end

local function botl(str)
    curses.addstr({y = rows - 1, x = 0}, str)
    curses.clrtoeol()
end

local function move_char(char, direction)
    if direction.x == 0 and direction.y == 0 then return 1 end
    local x, y = char.x, char.y
    char.x = char.x + direction.x
    char.y = char.y + direction.y
    if x ~= char.x or y ~= char.y then return 1
    else return 0
    end
end
-- }}}

-- curses initialization {{{
curses.initscr();
curses.start_color();
curses.setup_term{nl = false, cbreak = true, echo = false, keypad = true}
for _, color in ipairs(colors) do
    curses.init_pair(color, color)
end
-- }}}

-- get the term size and the size of the map we want to draw {{{
rows, cols = curses.getmaxyx()
map = {ul = {x = 0, y = 1}, lr = {x = cols - 1, y = rows - 3}}
map.w = map.lr.x - map.ul.x + 1
map.h = map.lr.y - map.ul.y + 1
-- }}}

-- draw the screen {{{
curses.clear()
for i = map.ul.y, map.lr.y do
    curses.addstr({y = i, x = map.ul.x}, ("."):rep(map.w))
end
-- }}}

-- initialize the character {{{
local char = {
    x = math.random(map.ul.x, map.lr.x),
    y = math.random(map.ul.y, map.lr.y),
    move = function(self, offset)
        if self.x + offset.x < map.ul.x or
           self.x + offset.x > map.lr.x or
           self.y + offset.y < map.ul.y or
           self.y + offset.y > map.lr.y then
            return 0
        else
            self.x = self.x + offset.x
            self.y = self.y + offset.y
            return 1
        end
    end,
    draw = function(self)
        curses.addch(self, "@", {bold = true})
        curses.move(self)
    end,
    erase = function(self)
        curses.addch(self, ".")
        curses.move(self)
    end
}
-- }}}

-- movement commands {{{
local directions = {
    y =     {x = -1, y = -1},
    h =     {x = -1, y = 0},  left =  {x = -1, y = 0},
    b =     {x = -1, y = 1},          
    k =     {x = 0,  y = -1}, up =    {x = 0, y = -1},
    ["."] = {x = 0,  y = 0},  s =     {x = 0, y = 0},
    j =     {x = 0,  y = 1},  down =  {x = 0, y = 1},
    u =     {x = 1,  y = -1},
    l =     {x = 1,  y = 0},  right = {x = 1, y = 0},
    n =     {x = 1,  y = 1},
}
-- }}}

-- main loop {{{
local turns = 0
while true do
    botl("T:" .. turns)
    char:draw()
    local c = curses.getch()
    char:erase()
    pline("")
    if directions[c] then
        turns = turns + char:move(directions[c])
    elseif (c == "Q") then
        break
    else
        if c == "\n" then c = "^J" end
        if c == "\r" then c = "^M" end
        pline("Unknown command '" .. c .. "'")
    end
end
-- }}}

-- cleanup {{{
curses.clear()
curses.endwin()
-- }}}
