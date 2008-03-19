include Make.config
include Make.$(OS)

BIN = src/curses.so
OBJ = src/curses.o src/strings.o
CC = gcc
INCLUDES = -I$(LUA_INCLUDEPATH)
DEFINES =
LIBS = -lcurses -l$(LUA_LIBNAME)
COMMONFLAGS = -Werror -Wall -pedantic -O2 -g -pipe $(OS_FLAGS)
CFLAGS = -c $(INCLUDES) $(DEFINES) $(COMMONFLAGS)
LDFLAGS = $(LIBS) $(COMMONFLAGS) -shared

SRC = src/curses.c src/strings.c src/strings.h
TEST_LUAS = test/rl.lua \
            test/test.lua
TTT_TEST_DIR = tictactoe
TTT_TEST_LUAS = test/tictactoe/tictactoe.lua \
		test/tictactoe/tictactoe_board.lua \
		test/tictactoe/tictactoe_player.lua
OTHER_FILES = Makefile \
	      Make.config \
	      README \
	      LICENSE \
	      TODO
VERSION = "LuaNcurses-0.02"

build : $(BIN)

$(BIN) : $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

%.o : %.c
	$(CC) $(CFLAGS) -o $@ $<

clean :
	rm -f $(OBJ) $(BIN)

dep :
	makedepend $(DEFINES) -Y $(SRC) > /dev/null 2>&1
	rm -f Makefile.bak

install :
	mkdir -p $(LUA_DIR)
	cp $(BIN) $(LUA_DIR)

dist : $(VERSION).tar.gz

$(VERSION).tar.gz : $(SRC) $(TEST_LUAS) $(OTHER_FILES)
	@echo "Creating $(VERSION).tar.gz"
	@mkdir $(VERSION)
	@mkdir $(VERSION)/src
	@cp $(SRC) $(VERSION)/src
	@mkdir $(VERSION)/test
	@cp $(TEST_LUAS) $(VERSION)/test
	@mkdir $(VERSION)/test/$(TTT_TEST_DIR)
	@cp $(TTT_TEST_LUAS) $(VERSION)/test/$(TTT_TEST_DIR)
	@cp $(OTHER_FILES) $(VERSION)
	@tar czf $(VERSION).tar.gz $(VERSION)
	@rm -rf $(VERSION)

# DO NOT DELETE

src/curses.o: src/strings.h
src/strings.o: src/strings.h
