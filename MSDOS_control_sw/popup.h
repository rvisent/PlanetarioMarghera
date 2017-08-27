#define NWMAX 10
#define ONE_LINE 1
#define TWO_LINE 2
#define NO_BORDER 0

#include <conio.h>
#include <alloc.h>

#ifdef POPUP
	char *bufferw[NWMAX][3];
	int coordw[NWMAX][4],n_window;
#else
	extern char *bufferw[NWMAX][3];
	extern int coordw[NWMAX][4],n_window;
#endif

next_popup(int left,int top,int right,int bottom,int foreground,
	int background,int border,int bordertype);
void init_popup(void);
int prev_popup(void);
