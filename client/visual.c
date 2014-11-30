/* 
//   Program:             TBD Chat Client
//   File Name:           visual.c
//   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
*/
#include "chat_client.h"

extern WINDOW *mainWin, *inputWin, *chatWin, *chatWinBox, *inputWinBox, *infoLine, *infoLineBottom;


/* Initialize some things */
void initializeCurses() {
   // Initialize the terminal window, read terminfo
   if ((mainWin = initscr()) == NULL) { exit(1); }
   // Do not echo key presses
   noecho();
   // Read input one char at a time
   cbreak();
   // Capture special keys
   keypad(mainWin, TRUE);
   // Start colors
   start_color();
   use_default_colors();

   // Initialize color pairs
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

   drawChatWin();
   drawInputWin();
   drawInfoLines();

   asciiSplash();
}


/* Draw chat box and window */
void drawChatWin() {
   // Create chat box and window
   chatWinBox = subwin(mainWin, (LINES * 0.8), COLS, 0, 0);
   box(chatWinBox, 0, 0);
   mvwaddch(chatWinBox, 0, (COLS * 0.5) - 6, ACS_RTEE);
   wattron(chatWinBox, COLOR_PAIR(3));
   mvwaddstr(chatWinBox, 0, (COLS * 0.5) - 5, " TBDChat " );
   wattroff(chatWinBox, COLOR_PAIR(3));
   mvwaddch(chatWinBox, 0, (COLS * 0.5) + 4, ACS_LTEE );
   wrefresh(chatWinBox);
   chatWin = subwin(chatWinBox, (LINES * 0.8 - 2), COLS - 2, 1, 1);
   scrollok(chatWin, TRUE);
}


/* Draw input box and window */
void drawInputWin() {
   // Create input box and window
   inputWinBox = subwin(mainWin, (LINES * 0.2) - 1, COLS, (LINES * 0.8) + 1, 0);
   box(inputWinBox, 0, 0);
   inputWin = subwin(inputWinBox, (LINES * 0.2) - 3, COLS - 2, (LINES * 0.8) + 2, 1);
}


/* Draw info lines */
void drawInfoLines() {
   // Create info lines
   infoLine = subwin(mainWin, 1, COLS, (LINES * 0.8), 0);
   wbkgd(infoLine, COLOR_PAIR(3));
   wprintw(infoLine, " Type /help to view a list of available commands");
   wrefresh(infoLine);
   wrefresh(infoLineBottom);
   infoLineBottom = subwin(mainWin, 1, COLS, LINES - 1, 0);
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

   // Print error
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


/* Print formatted alert/notification */
void wprintFormatNotice(WINDOW *win, time_t ts, char *buf) {
   // Print formatted time
   wprintFormatTime(win, ts);

   // Print notice
   wattron(win, A_BOLD);
   wprintw(chatWin, " ");
   waddch(chatWin, ACS_HLINE);
   waddch(chatWin, ACS_HLINE);
   waddch(chatWin, ACS_HLINE);
   wprintw(chatWin, " ");
   wattron(win, COLOR_PAIR(2));
   wprintw(chatWin, "Notice");
   wattroff(win, COLOR_PAIR(2));
   wattroff(win, A_BOLD);
   wattron(win, COLOR_PAIR(1));
   wprintw(chatWin, " %s\n", buf);
   wattroff(win, COLOR_PAIR(1));
}


/* Print formatted and colored timestamp */
void wprintFormatTime(WINDOW *win, time_t ts) {
   struct tm *timestamp;

   // Get tm struct from localtime using epoch time
   timestamp = localtime(&ts);

   // Print HH:MM:SS
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

   // Print vertical line
   wattron(win, COLOR_PAIR(7));
   wprintw(win, " ");
   waddch(win, ACS_VLINE);
   wprintw(win, " ");
   wattroff(win, COLOR_PAIR(7));
}


/* Print seperator bar with title 2/3rd the length of the window passed in */
void wprintSeperatorTitle(WINDOW *win, char *title, int color, int title_color) {
   int height, width, i;

   // Get window size
   getmaxyx(win, height, width);

   // Print formatted time
   wprintFormatTime(win, time(NULL));

   // Print first half of seperator
   wattron(win, COLOR_PAIR(color));
   for (i = 0; i < ((width / 3) - ((strlen(title) + 2) / 2)); i++) {
      waddch(win, ACS_HLINE);
   }

   // Print pipes and center title
   waddch(win, ACS_RTEE);
   wattroff(win, COLOR_PAIR(color));
   wattron(win, COLOR_PAIR(title_color));
   //wattron(win, A_BOLD);
   wprintw(win, " %s ", title);
   //wattroff(win, A_BOLD);
   wattroff(win, COLOR_PAIR(title_color));
   wattron(win, COLOR_PAIR(color));
   waddch(win, ACS_LTEE);

   // Print second half of seperator
   for (i = 0; i < ((width / 3) - ((strlen(title) + 2) / 2)); i++) {
      waddch(win, ACS_HLINE);
   }
   wprintw(win, "\n");
   wattroff(win, COLOR_PAIR(color));
}


/* Print seperator bard 2/3 length of window */
void wprintSeperator(WINDOW *win, int color) {
   int height, width, i;

   // Get window size
   getmaxyx(win, height, width);

   // Print formatted time
   wprintFormatTime(win, time(NULL));
   
   // Print seperator
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
}


/* Handle window resizing */
void resizeHandler(int sig) {
   useconds_t sleep_time = 100;

   // End current windows, wait briefly
   endwin();
   refresh();
   clear();
   usleep(sleep_time);

   // Redraw windows
   drawChatWin();
   drawInputWin();
   drawInfoLines();

   // Redraw ascii splash
   asciiSplash();

   // Refresh and move cursor to input window
   wrefresh(chatWin);
   wcursyncup(inputWin);
   wrefresh(inputWin);
}
