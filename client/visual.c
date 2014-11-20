#include "visual.h"

void setup_screens() {
   initscr();
   cbreak();
   keypad(stdscr, TRUE);
}

WINDOW *create_text_window() {
   WINDOW *tmp;

   int end_y = LINES * 0.8;

   tmp = newwin(end_y, COLS, 0, 0);
   box(tmp, 0, 0);
   wrefresh(tmp);

   return tmp;
}

WINDOW *create_input_window() {
   WINDOW *tmp;

   int start_y = LINES * 0.8;
   int height = LINES * 0.2;

   tmp = newwin(height, COLS, start_y, 0);
   box(tmp, 0, 0);
   wrefresh(tmp);

   return tmp;
}
