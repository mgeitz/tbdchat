/* 
//   Program:             TBD Chat Client
//   File Name:           visual.c
//   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
*/
#include "chat_client.h"

extern WINDOW *mainWin, *inputWin, *chatWin, *chatWinBox, *inputWinBox, *infoLine, *infoLineBottom;


/* Initialize some things */
void initializeCurses() {
   // Initialize curses
   if ((mainWin = initscr()) == NULL) { exit(1); }
   // Start colors
   start_color();
   use_default_colors();
   // Do not echo key presses
   noecho();
   // Read input one char at a time
   cbreak();
   // Capture special keys
   keypad(mainWin, TRUE);

   // Initialize color types
   init_pair(1, -1, -1); // Default                             
   init_pair(2, COLOR_CYAN, -1);
   init_pair(3, COLOR_YELLOW, -1);
   init_pair(4, COLOR_RED, -1);
   init_pair(5, COLOR_BLUE, -1);
   init_pair(6, COLOR_MAGENTA, -1);
   init_pair(7, COLOR_GREEN, -1);
   init_pair(8, COLOR_WHITE, COLOR_RED);
   init_pair(9, COLOR_WHITE, COLOR_BLUE);
   init_pair(10, COLOR_WHITE, COLOR_GREEN);
   init_pair(11, COLOR_BLACK, COLOR_MAGENTA);
}


/* Print message */
void wprintFormat(WINDOW *win, time_t ts, char *from, char *buf, int from_color) {

   // Print formatted time
   wprintFormatTime(win, ts);

   // Print from and buffer
   wattron(win, COLOR_PAIR(1));
   wprintw(win, "[");
   wattroff(win, COLOR_PAIR(1));
   wattron(win, COLOR_PAIR(from_color));
   wprintw(win, "%s", from);
   wattroff(win, COLOR_PAIR(from_color));
   wattron(win, COLOR_PAIR(1));
   wprintw(win, "] %s\n", buf);
   wattroff(win, COLOR_PAIR(1));
}


/* Print formatted error */
void wprintFormatError(WINDOW *win, time_t ts, char *buf) {
   // Print formatted time
   wprintFormatTime(win, ts);

   wattron(win, A_BOLD);
   wprintw(chatWin, " ");
   waddch(chatWin, ACS_HLINE);
   waddch(chatWin, ACS_HLINE);
   waddch(chatWin, ACS_HLINE);
   wprintw(chatWin, " ");
   wattron(win, COLOR_PAIR(8));
   wprintw(chatWin, "Error");
   wattroff(win, COLOR_PAIR(8));
   wattroff(win, A_BOLD);
   wattron(win, COLOR_PAIR(1));
   wprintw(chatWin, " %s\n", buf);
   wattroff(win, COLOR_PAIR(1));
}


/* Print formatted and colored timestamp */
void wprintFormatTime(WINDOW *win, time_t ts) {
   struct tm *timestamp;
   timestamp = localtime(&ts);

   wattron(win, COLOR_PAIR(1));
   wprintw(win, "%02d", timestamp->tm_hour);
   wattroff(win, COLOR_PAIR(1));
   wattron(win, COLOR_PAIR(3));
   wprintw(win, ":");
   wattroff(win, COLOR_PAIR(3));
   wattron(win, COLOR_PAIR(1));
   wprintw(win, "%02d", timestamp->tm_min);
   wattroff(win, COLOR_PAIR(1));
   wattron(win, COLOR_PAIR(3));
   wprintw(win, ":");
   wattroff(win, COLOR_PAIR(3));
   wattron(win, COLOR_PAIR(1));
   wprintw(win, "%02d", timestamp->tm_sec);
   wattroff(win, COLOR_PAIR(1));

   wattron(win, COLOR_PAIR(7));
   wprintw(win, " ");
   waddch(win, ACS_VLINE);
   wprintw(win, " ");
   wattroff(win, COLOR_PAIR(7));
}


/* Print seperator bar with title 2/3rd the length of the window passed in */
void wprintSeperatorTitle(WINDOW *win, char *title, int color, int title_color) {
   int height, width, i;
   getmaxyx(win, height, width);

   // Print formatted time
   wprintFormatTime(win, time(NULL));
   wattron(win, COLOR_PAIR(color));
   for (i = 0; i < ((width / 3) - ((strlen(title) + 2) / 2)); i++) {
      waddch(win, ACS_HLINE);
   }
   waddch(win, ACS_RTEE);
   wattroff(win, COLOR_PAIR(color));
   wattron(win, COLOR_PAIR(title_color));
   wprintw(win, " %s ", title);
   wattroff(win, COLOR_PAIR(title_color));
   wattron(win, COLOR_PAIR(color));
   waddch(win, ACS_LTEE);
   for (i = 0; i < ((width / 3) - ((strlen(title) + 2) / 2)); i++) {
      waddch(win, ACS_HLINE);
   }
   wprintw(win, "\n");
   wattroff(win, COLOR_PAIR(color));
}


/* Print seperator bard 2/3 length of window */
void wprintSeperator(WINDOW *win, int color) {
   int height, width, i;
   getmaxyx(win, height, width);

   wprintFormatTime(win, time(NULL));
   wattron(win, COLOR_PAIR(color));
   for (i = 0; i < (2 * (width / 3) + 2); i++) {
      waddch(win, ACS_HLINE);
   }
   wprintw(win, "\n");
   wattroff(win, COLOR_PAIR(color));
}


/* Print message on startup */
void asciiSplash() {
   int icon = 2;
   int word = 1;
   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "         __\n");
   wattroff(chatWin, A_BOLD);
   wattroff(chatWin, COLOR_PAIR(icon));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "        /_/\\       ");
   wattroff(chatWin, A_BOLD);
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(word));
   wprintw(chatWin, " _____ ____  ____     ____ _           _   \n");
   wattroff(chatWin, COLOR_PAIR(word));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "       / /\\ \\      ");
   wattroff(chatWin, A_BOLD);
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(word));
   wprintw(chatWin, "|_   _| __ )|  _ \\   / ___| |__   __ _| |_ \n");
   wattroff(chatWin, COLOR_PAIR(word));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "      / / /\\ \\      ");
   wattroff(chatWin, A_BOLD);
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(word));
   wprintw(chatWin, " | | |  _ \\| | | | | |   | '_ \\ / _` | __|\n");
   wattroff(chatWin, COLOR_PAIR(word));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "     / / /\\ \\ \\     ");
   wattroff(chatWin, A_BOLD);
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(word));
   wprintw(chatWin, " | | | |_) | |_| | | |___| | | | (_| | |_ \n");
   wattroff(chatWin, COLOR_PAIR(word));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "    / /_/__\\ \\ \\    ");
   wattroff(chatWin, A_BOLD);
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(word));
   wprintw(chatWin, " |_| |____/|____/   \\____|_| |_|\\__,_|\\__|\n");
   wattroff(chatWin, COLOR_PAIR(word));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "   /_/______\\_\\/\\\n");
   wattroff(chatWin, A_BOLD);
   wattroff(chatWin, COLOR_PAIR(icon));

   wprintFormatTime(chatWin, time(NULL));
   wattron(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, A_BOLD);
   wprintw(chatWin, "   \\_\\_________\\/\t");
   wattroff(chatWin, A_BOLD);
   wattroff(chatWin, COLOR_PAIR(icon));
   wattron(chatWin, COLOR_PAIR(1));
   wprintw(chatWin, "version ");
   wattroff(chatWin, COLOR_PAIR(1));
   wattron(chatWin, COLOR_PAIR(3));
   wprintw(chatWin, "%s\n", VERSION);
   wattroff(chatWin, COLOR_PAIR(3));

   wprintFormatTime(chatWin, time(NULL));
   wprintw(chatWin, "\n");

   wrefresh(chatWin);
}


/*
void drawChatWinBox(WINDOW *chatWinBox, int LINES, int COLS) {
   chatWinBox = subwin(mainWin, (LINES * 0.8), COLS, 0, 0);
   box(chatWinBox, 0, 0);
   mvwaddstr(chatWinBox, 0, (COLS * 0.5) - 6, "| TBDChat |" );
   wrefresh(chatWinBox);
}


void drawChatWin(WINDOW *chatWin, int LINES, int COLS) {
   chatWin = subwin(chatWinBox, (LINES * 0.8 - 2), COLS - 2, 1, 1);
}


void drawInfoLine(WINDOW *infoLine, int LINES, int COLS) {
   infoLine = subwin(mainWin, 1, COLS, (LINES * 0.8), 0);
}


void drawInputWinBox(WINDOW *inputWinBox, int LINES, int COLS) {
   inputWinBox = subwin(mainWin, (LINES * 0.2) - 1, COLS, (LINES * 0.8) + 1, 0);
   box(inputWinBox, 0, 0);
}


void drawInputWin(WINDOW *inputWin, int LINES, int COLS) {
   inputWin = subwin(inputWinBox, (LINES * 0.2) - 3, COLS - 2, (LINES * 0.8) + 2, 1);
}

*/
