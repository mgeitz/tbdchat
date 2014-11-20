#ifndef VISUAL_H
#define VISUAL_H
#include <ncurses.h>

void setup_screens();
WINDOW *create_text_window();
WINDOW *create_input_window();

#endif
