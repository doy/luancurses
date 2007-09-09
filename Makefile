src/curses.so : src/curses.c
	gcc -O -shared -fpic -Wall -pedantic -Werror src/curses.c -o src/curses.so -lcurses -llua
