#include <stdio.h>
#include <time.h>
#include <process.h>
#include "defs.h"

void sess_rec(int rec_mode)
{
	FILE *out;
	time_t t;

	time(&t);
	system("copy session.txt session.bak > garbage.txt");

	out=fopen("session.txt","at");
	switch(rec_mode)
	{
		case SYSON:
			fprintf(out,"--------------------------------\n");
			fprintf(out,"System  ON @ %s", ctime(&t));
		break;
		case SYSOFF:
			fprintf(out,"System OFF @ %s", ctime(&t));
			fprintf(out,"--------------------------------\n");
		break;
		case STARSON:
			fprintf(out,"Stars   ON @ %s", ctime(&t));
		break;
		case STARSOFF:
			fprintf(out,"Stars  OFF @ %s", ctime(&t));
		break;
	}
	fclose(out);
}

