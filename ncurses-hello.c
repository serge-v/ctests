#include <ncurses.h>

static char *lines[] = {
	"line1",
	"line2",
	"line3"
};

static int ncount = 3;
static int sel = 0;

static void
draw_list(WINDOW *win)
{
	for (int i = 0; i < ncount; i++) {
		if (i == sel)
			wattron(win, COLOR_PAIR(1));
		mvwaddstr(win, i+2, 1, lines[i]);
		if (i == sel)
			wattroff(win, COLOR_PAIR(1));
	}
}

int main()
{
	int quit = 0;
	int row, col;
	WINDOW *win;

	initscr();
	start_color();
	getmaxyx(stdscr, row, col);

	init_pair(1, COLOR_RED, COLOR_BLACK);
	printw("rows: %d, cols %d", row, col);

	refresh();
	noecho();
	cbreak();

	win = newwin(row-6, col-2, 2, 1);
	box(win, 0 , 0);
	draw_list(win);
	wrefresh(win);
	keypad(win, TRUE);
	curs_set(0);

	while (!quit) {
		int ch = getch();
		mvwaddstr(win, 20, 1, "key:");
		attron(A_BOLD);
		wprintw(win, "%d %c", ch, ch);
		attroff(A_BOLD);
		switch (ch) {
			case 'q': case 'Q':
				quit = 1;
				break;
			case 'A':
				if (--sel < 0)
					sel = ncount-1;
				break;
			case 'B':
				if (++sel >= ncount)
				sel = 0;
				break;
		}
		draw_list(win);
		wrefresh(win);
	}

	endwin();
	return 0;
}
