#include "popup.h"

int prev_popup(void) {
int i,left,top,right,bottom;

if(n_window) {

	puttext(coordw[n_window][0],coordw[n_window][1],coordw[n_window][2],
		coordw[n_window][3],bufferw[n_window][0]);
	free(bufferw[n_window][0]);

	left=coordw[n_window][0]+1;
	top=coordw[n_window][1]+1;
	right=coordw[n_window][2]-1;
	bottom=coordw[n_window][3]-1;

	puttext(left,bottom+2,right+2,bottom+2,
		bufferw[n_window][1]);
	free(bufferw[n_window][1]);

	puttext(right+2,top,right+2,bottom+1,
		bufferw[n_window][2]);
	free(bufferw[n_window][2]);

	n_window--;
	window(coordw[n_window][0],coordw[n_window][1],coordw[n_window][2],
		coordw[n_window][3]);
	return 0;
	}
	else
	return 1;
}


