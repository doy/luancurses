local error    = error
local insert   = table.insert
local ipairs   = ipairs
local setmetatable = setmetatable
local type     = type
local unpack   = unpack

require 'curses'
local addstr   = curses.addstr
local addch    = curses.addch
local clrtoeol = curses.clrtoeol
local move     = curses.move

module 'tictactoe_board'

-- constants
local board_background = {
    "   |   |   ",
    "   |   |   ",
    "   |   |   ",
    "---+---+---",
    "   |   |   ",
    "   |   |   ",
    "   |   |   ",
    "---+---+---",
    "   |   |   ",
    "   |   |   ",
    "   |   |   ",
}

-- constructor arguments are the coordinates on the screen where the board
-- should be drawn
function new(y, x)
    -- the board uses empty tables to represent 'empty tile', since the
    -- table constructor is guaranteed to produce unique tables, which will
    -- compare not equal with each other
    local board = {
        { {}, {}, {} },
        { {}, {}, {} },
        { {}, {}, {} },
    }
    return setmetatable({board = board, yorig = y, xorig = x}, {__index = _M})
end

function clone(self)
    local copy = new(self.yorig, self.xorig)
    for row in ipairs(self.board) do
        for col in ipairs(self.board[row]) do
            if self:at(row, col) ~= nil then
                copy.board[row][col] = self.board[row][col]
            end
        end
    end
    return copy
end

function size()
    return #board_background, board_background[1]:len()
end

-- translate board positions to positions on the screen
-- return a table with named members, to pass directly to the curses functions
function position(self, row, col)
    return {y = self.yorig + (row - 1) * 4 + 1,
            x = self.xorig + (col - 1) * 4 + 1}
end

function draw(self)
    -- draw the board
    for i, line in ipairs(board_background) do
        addstr({y = self.yorig + i - 1, x = self.xorig}, line)
    end

    -- draw the x's and o's
    for row in ipairs(self.board) do
        for col, tile in ipairs(self.board[row]) do
            if tile == "x" then
                addch(self:position(row, col), "X", {color = "red"})
            elseif tile == "o" then
                addch(self:position(row, col), "O", {color = "blue"})
            end
        end
    end
end

function at(self, y, x)
    if type(self.board[y][x]) == "table" then
        return nil
    else
        return self.board[y][x]
    end
end

-- return a list of empty tiles on the board, where tiles are 2 element lists
-- of {y, x}
function empty_tiles(self)
    local ret = {}
    for row in ipairs(self.board) do
        for col in ipairs(self.board[row]) do
            if self:at(row, col) == nil then
                insert(ret, {row, col})
            end
        end
    end
    return ret
end

function mark(self, turn, row, col)
    if turn == "x" or turn == "o" then
        self.board[row][col] = turn
    else
        error("mark called with \'" .. turn .. "\'")
    end
end

-- check whether there is a winner on the board
-- returns the symbol for the winner (or nil if there is no winner), as well as
-- a list of the tiles that make up the win, so that we can mark them later
function winner(self)
    -- check rows and columns
    for i = 1, 3 do
        if self.board[i][1] == self.board[i][2] and
           self.board[i][2] == self.board[i][3] then
            return self.board[i][i], {{i, 1}, {i, 2}, {i, 3}}
        elseif self.board[1][i] == self.board[2][i] and
               self.board[2][i] == self.board[3][i] then
            return self.board[i][i], {{1, i}, {2, i}, {3, i}}
        end
    end

    -- check diagonals
    if self.board[1][1] == self.board[2][2] and
       self.board[2][2] == self.board[3][3] then
        return self.board[2][2], {{1, 1}, {2, 2}, {3, 3}}
    elseif self.board[3][1] == self.board[2][2] and
           self.board[2][2] == self.board[1][3] then
        return self.board[2][2], {{3, 1}, {2, 2}, {1, 3}}
    end
end

function mark_winner(self, winner, winner_tiles)
    for _, loc in ipairs(winner_tiles) do
        addch(self:position(unpack(loc)), winner:upper(), {color = "green"})
    end
end

-- draw a string centered under the board
function caption(self, str)
    move(self.yorig + #board_background + 1, 0)
    clrtoeol()
    addstr({y = self.yorig + #board_background + 1,
            x = self.xorig + (board_background[1]:len() - 1) / 2 -
                str:len() / 2},
           str)
end
