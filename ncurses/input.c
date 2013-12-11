#include <ncurses.h>			/* ncurses.h includes stdio.h */  
#include <string.h> 
 
int main()
{
 char mesg[]="Enter a string: ";		/* message to be appeared on the screen */
 char str[80];
 int row,col,currow;				/* to store the number of rows and *
					 * the number of colums of the screen */
 initscr();				/* start the curses mode */
 getmaxyx(stdscr,row,col);		/* get the number of rows and columns */

 for(currow = 0;;currow++) {
     mvprintw(LINES - 2, 0,"%s", mesg);
     getstr(str);

     mvprintw(currow, 0, "You Entered: %s", str);


     move(LINES - 2, 0);
     clrtoeol();
 }

 getch();
 endwin();

 return 0;
}
