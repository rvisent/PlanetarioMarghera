#include "popup.h"

int next_popup(int left,int top,int right,int bottom,int foreground,
	int background,int border,int bordertype) {

int wsize,shadow;
register int i,j;
char *video_ombra;

if(n_window==NWMAX)
	return 1;

if (bordertype)
	{
	n_window++;
	wsize=(right-left+3)*(bottom-top+3)*2;
	if((bufferw[n_window][0]=(char *)malloc(wsize))==NULL)
		{
		clrscr();
		printf("Error...");
		return 2;
		}
	gettext(left-1,top-1,right+1,bottom+1,bufferw[n_window][0]);
	coordw[n_window][0]=left-1;
	coordw[n_window][1]=top-1;
	coordw[n_window][2]=right+1;
	coordw[n_window][3]=bottom+1;

/******************************************************
					 ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
					 ³                        ³
					 ³ ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿ ³°
					 ³ ³Left    Top    Right³ ³°
					 ³ ³                    ³ ³°
					 ³ ³        "W"         ³ ³°
					 ³ ³                    ³ ³"RSH"
					 ³ ³                    ³ ³°
					 ³ ³       Bottom       ³ ³°
					 ³ ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ³°
					 ³         "W+B"          ³°
					 ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ°
					  °°°°°°°°°"BSH"°°°°°°°°°°°°

					  W=R-L+1;
					W+B=(R+1)-(L-1)+1=r-L+3;
					RSH=B+1-T+1=B-T+2;
					BSH=R+2-L+1=R-L+3;
 ******************************************************/

	shadow=(right-left+3)*2;
	if((bufferw[n_window][1]=(char *)malloc(shadow))==NULL)
		{
		clrscr();
		printf("Error...");
		return 2;
		}
	if((video_ombra=(char *)malloc(shadow))==NULL)
		{
		clrscr();
		printf("Error...");
		return 2;
		}
	gettext(left,bottom+2,right+2,bottom+2,bufferw[n_window][1]);
	gettext(left,bottom+2,right+2,bottom+2,video_ombra);
	for(i=1;i<=((right-left+3)*2-1);i+=2)
		video_ombra[i]=0x01;
	puttext(left,bottom+2,right+2,bottom+2,video_ombra);
	free(video_ombra);

	shadow=(bottom-top+2)*2;
	if((bufferw[n_window][2]=(char *)malloc(shadow))==NULL)
		{
		clrscr();
		printf("Error...");
		return 2;
		}
	if((video_ombra=(char *)malloc(shadow))==NULL)
		{
		clrscr();
		printf("Error...");
		return 2;
		}
	gettext(right+2,top,right+2,bottom+1,bufferw[n_window][2]);
	gettext(right+2,top,right+2,bottom+1,video_ombra);
	for(i=1;i<=((bottom-top+2)*2-1);i+=2)
		video_ombra[i]=0x01;
	puttext(right+2,top,right+2,bottom+1,video_ombra);
	free(video_ombra);
	/******************************************************/

 	window(left-1,top-1,right+1,bottom+1);
	textcolor(border);
	textbackground(background);
	clrscr();
	window(left-1,top-1,right+2,bottom+2);
	if(bordertype==ONE_LINE)
		{
		cprintf("Ú");
		for(i=1;i<=(right-left+1);i++)
			cprintf("Ä");
		cprintf("¿");
		for(j=2;j<=(bottom-top+2);j++)
			{
			gotoxy(1,j);
			cprintf("³");
			gotoxy(right-left+3,j);
			cprintf("³");
			}
		gotoxy(1,j);
		cprintf("À");
		for(i=1;i<=(right-left+1);i++)
			cprintf("Ä");
		cprintf("Ù");
		window(left,top,right,bottom);
		gotoxy(1,1);
		textcolor(foreground);
		return 0;
		}
		else
		{
		cprintf("É");
		for(i=1;i<=(right-left+1);i++)
			cprintf("Í");
		cprintf("»");
		for(j=2;j<=(bottom-top+2);j++)
			{
			gotoxy(1,j);
			cprintf("º");
			gotoxy(right-left+3,j);
			cprintf("º");
			}
		gotoxy(1,j);
		cprintf("È");
		for(i=1;i<=(right-left+1);i++)
			cprintf("Í");
		cprintf("¼");
		window(left,top,right,bottom);
		gotoxy(1,1);
		textcolor(foreground);
		return 0;
		}
	}		
	else
	{
	n_window++;
	wsize=(right-left+1)*(bottom-top+1)*2;
	if((bufferw[n_window][0]=(char *)malloc(wsize))==NULL)
		{
		clrscr();
		printf("Error...");
		return(2);
		}
	if (!gettext(left,top,right,bottom,bufferw[n_window][0]))
		{
		clrscr();
		printf("Error...");
		return 2;
		}
	coordw[n_window][0]=left;
	coordw[n_window][1]=top;
	coordw[n_window][2]=right;
	coordw[n_window][3]=bottom;
	window(left,top,right,bottom);
	textcolor(border);
	textbackground(background);
	clrscr();
	for(j=0;j<=(bottom-top);j++)
		for(i=0;i<=(right-left);i++)
			cprintf(" ");
	for(i=1;i<=(right-left);i++)
			cprintf(" ");
	gotoxy(1,1);
	textcolor(foreground);
	return 0;
	}
}
