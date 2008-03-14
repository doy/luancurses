local insert = table.insert
local ipairs = ipairs
local remove = table.remove
local unpack = unpack

require 'curses'
local addch  = curses.addch
local getch  = curses.getch

module 'tictactoe_player'

local function get_move_human(self, board)
    -- first find the first empty tile on the board, so that we start the
    -- cursor off in an empty tile
    local y, x
    for row in ipairs(board.board) do
        for col in ipairs(board.board[row]) do
            if board:at(row, col) == nil then
                y, x = row, col
                break
            end
        end
        if y or x then break end
    end

    while true do
        local c
        -- draw the character we are about to put down under the cursor only if
        -- the tile under the cursor is empty
        if board:at(y, x) == nil then
            addch(board:position(y, x), self.symbol:upper())
            c = getch(board:position(y, x))
            addch(board:position(y, x), " ")
        else
            c = getch(board:position(y, x))
        end

            if c == "left"  or c == "h" then
            x = (x - 2) % 3 + 1
        elseif c == "right" or c == "l" then
            x = x % 3 + 1
        elseif c == "up"    or c == "k" then
            y = (y - 2) % 3 + 1
        elseif c == "down"  or c == "j" then
            y = y % 3 + 1
        elseif c == "enter" or c == " " then
            -- don't return illegal moves
            if board:at(y, x) == nil then
                return y, x
            end
        end
    end
end

-- recursive helper function for the computer's move calculation
-- it returns true if the player can certainly win from the given board
-- position and the given turn
-- it returns false if the player will lose against a perfect opponent
-- it returns nil if the player can force a draw, but cannot win
-- the second return value is a list of positions that correspond to the move
-- predictions for the rest of the game
local function find_move(self, board, turn)
    local tiles = board:empty_tiles()
    -- hardcode starting move, since there's only one best move anyway, and
    -- this speeds things up quite a bit
    if #tiles == 9 then
        return nil, {{3, 3}}
    end

    local winner = board:winner()
    if winner then
        if winner == self.symbol then return true, {}
        else return false
        end
    elseif #tiles == 0 then
        return nil, {}
    else
        local next_turn
        if turn == "x" then next_turn = "o" else next_turn = "x" end

        local won, path, tie_path
        for _, tile in ipairs(tiles) do
            -- create a copy of the board with the next position to try
            -- filled in, and see if there is a winning position
            local copy = board:clone()
            copy:mark(turn, unpack(tile))
            won, path = find_move(self, copy, next_turn)

            if self.symbol == turn then
                -- if it's our turn and the move we just tried will certainly 
                -- win, then return that move
                if won == true then
                    insert(path, tile)
                    return true, path
                end
            else
                -- if it's the opponent's turn and the opponent's move will
                -- make us lose, then drop the rest of this branch
                if won == false then return false end
            end

            -- keep track of if we found a path that will result in a tie,
            -- since that is the second best result
            if won == nil then
                insert(path, tile)
                tie_path = path
            end
        end

        -- if we haven't yet returned with a winning path, then return with
        -- a tie if possible
        if tie_path then return nil, tie_path end

        -- if we get here and it's our turn, that means that all possibilities
        -- for our move will make us lose, so drop this branch
        -- if we get here and it's the opponent's turn, that means that
        -- all possibilities for the opponent's move will make us win, so just
        -- return any path
        if self.symbol == turn then return false
        else return true, path
        end
    end
end

local function get_move_computer(self, board)
    local win, path = find_move(self, board, self.symbol)
    -- the next move is the end of the path that find_move returned
    return unpack(remove(path))
end

function new(type, symbol)
    local get_move
    if type == "human" then
        get_move = get_move_human
    else
        get_move = get_move_computer
    end

    -- we don't need an actual object, a table will do fine
    return {get_move = get_move, symbol = symbol}
end
