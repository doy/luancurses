local Board = require "tictactoe_board"
local Player = require "tictactoe_player"
require "curses"
require "signal"

-- deinitialize curses on exit, or when a signal is received, so we don't leave
-- the terminal in a messed up state
local function cleanup(sig)
    curses.endwin()
    if sig then
        signal.signal(sig, "default")
        signal.raise(sig)
    end
end

local function get_player_types()
    local player1, player2
    io.write("Is player 1 a human? ");
    local response = io.read()
    if response:sub(1, 1):lower() == "y" then
        player1 = "human"
    else
        player1 = "computer"
    end

    io.write("Is player 2 a human? ");
    local response = io.read()
    if response:sub(1, 1):lower() == "y" then
        player2 = "human"
    else
        player2 = "computer"
    end

    return player1, player2
end

local function init_curses()
    curses.initscr()
    signal.signal("INT", cleanup)
    signal.signal("TERM", cleanup)
    curses.start_color()
    curses.use_default_colors()
    curses.setup_term{nl = false, cbreak = true, echo = false, keypad = true}
    for _, color in ipairs({"red", "blue", "green"}) do
        curses.init_pair(color, color)
    end
end

local function init_board(ymax, xmax)
    local ymax, xmax = curses.getmaxyx()
    local board_y, board_x = Board.size()
    -- center the board horizontally, and place the board a little above center
    -- vertically, so that the caption isn't too low
    return Board.new(math.floor(ymax - board_y) / 2 - 1,
                     math.floor(xmax - board_x) / 2)
end

local function main()
    -- initialize the game
    local player1, player2 = get_player_types()
    init_curses()
    board = init_board()
    players = { x = Player.new(player1, "x"), o = Player.new(player2, "o") }

    -- start the main loop
    local turn = "x"
    board:draw()
    curses.refresh()
    while true do
        if #board:empty_tiles() == 0 then
            board:caption("Tie!")
            break
        end

        board:mark(turn, players[turn]:get_move(board))
        board:draw()

        local winner, winner_tiles = board:winner()
        if winner then
            board:mark_winner(winner, winner_tiles)
            board:caption(winner:upper() .. " wins!")
            break
        end
        if turn == "x" then turn = "o" else turn = "x" end
    end
end

-- use pcall to catch lua errors, which we can't catch with our signal handlers
-- (so that we can clean up curses if necessary)
local success, err_msg = pcall(main)

if success then
    curses.getch()
    cleanup()
else
    cleanup()
    print(err_msg)
end
