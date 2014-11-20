/* 
//   Program:             TBD Chat Client
//   File Name:           visual.c
//   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
*/
#include "visual.h"

/* Initialize some things */
void setup_screens() {
   initscr();
   cbreak();
   keypad(stdscr, TRUE);
}

/* Primary output window */
WINDOW *create_text_window() {
   WINDOW *tmp;

   int end_y = LINES * 0.8;

   tmp = newwin(end_y, COLS, 0, 0);
   box(tmp, 0, 0);
   wrefresh(tmp);

   return tmp;
}

/* Primary input window */
WINDOW *create_input_window() {
   WINDOW *tmp;

   int start_y = LINES * 0.8;
   int height = LINES * 0.2;

   tmp = newwin(height, COLS, start_y, 0);
   box(tmp, 0, 0);
   wrefresh(tmp);

   return tmp;
}
