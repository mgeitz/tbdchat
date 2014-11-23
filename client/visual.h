#ifndef VISUAL_H
#define VISUAL_H
#include <ncurses.h>

void initializeCurses();
void drawChatWinBox(WINDOW *chatWinBox, int LINES, int COLS);
void drawChatWin(WINDOW *chatWin, int LINES, int COLS);
void drawInfoLine(WINDOW *infoLine, int LINES, int COLS);
void drawInputWinBox(WINDOW *inputWinBox, int LINES, int COLS);
void drawInputWin(WINDOW *inputWin, int LINES, int COLS);

#endif
