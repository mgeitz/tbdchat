/* 
//   Program:             TBD Chat Client
//   File Name:           visual.c
//   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
*/
#include "visual.h"

/* Initialize some things */
// I think initscr needs to be called for each window separately
void setup_screens() {
   //initscr();
   // Do not echo key presses
   noecho();
   // Read input one char at a time
   cbreak();
   // Capture special keys
   keypad(stdscr, TRUE);
}


/* Primary output window */
//WINDOW *create_text_window() {
//   WINDOW *tmp;

 //  int end_y = LINES * 0.8;

//   tmp = newwin(end_y, COLS, 0, 0);
//   box(tmp, 0, 0);
//   wrefresh(tmp);

//   return tmp;
//}
/*
void drawTextScreen(Window *win) {
   int nh, nw;

   int end_y = LINES * 0.8;

   win = newwin(end_y, COLS, 0, 0);
   box(tmp, 0, 0);

}
*/
/* Primary input window */
//WINDOW *create_input_window() {
//   WINDOW *tmp;

//   int start_y = LINES * 0.8;
//   int height = LINES * 0.2;

//   tmp = newwin(height, COLS, start_y, 0);
//   box(tmp, 0, 0);
//   wrefresh(tmp);

//   return tmp;
//}
