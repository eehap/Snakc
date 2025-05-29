#include <curses.h>
#include <signal.h>
#include <stdlib.h>

static void finish(int sig);
void init_game();

int main(int argc, char **argv) {
    signal(SIGINT, finish);

    initscr();
    cbreak();
    nodelay(stdscr, TRUE);
    curs_set(0);
    noecho();

    init_game();

    finish(0);
}

static void finish(int sig) {
    endwin();
    exit(0);
}
