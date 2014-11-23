/* 
//   Program:             TBD Chat Client
//   File Name:           visual.c
//   Authors:             Matthew Owens, Michael Geitz, Shayne Wierbowski
*/
//#include <stdlib.h>
#include "visual.h"

//extern WINDOW *mainWin, *inputWin, *chatWin, *chatWinBox, *inputWinBox, *infoLine;

/* Initialize some things */
// I think initscr needs to be called for each window separately
/*void initializeCurses() {
   int LINES, COLS;
   // Initialize curses
   if ((mainWin = initscr()) == NULL) { exit(1); }

   getmaxyx(mainWin, LINES, COLS);

   // Do not echo key presses
   noecho();
   // Read input one char at a time
   cbreak();
   // Capture special keys
   keypad(mainWin, TRUE);

   drawChatWinBox(chatWinBox, LINES, COLS);
   drawChatWin(chatWin, LINES, COLS);
   scrollok(chatWin, TRUE);
   drawInfoLine(infoLine, LINES, COLS);
   drawInputWinBox(inputWinBox, LINES, COLS);
   drawInputWin(inputWin, LINES, COLS);

}


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
