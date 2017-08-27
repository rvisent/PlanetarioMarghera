#include  <math.h>
#include	<stdio.h>
#include	"defs.h"
#include	<string.h>

int read_setup(void)
{
	FILE *in;
	char	chin,oneline[100],*token;
	float	hms;
	int		i;

	if((in=fopen("setup.pla","rt"))==NULL)
		return(1);

	i=0;																//prima riga: qui ci deve essere
	while((chin=toupper(fgetc(in)))!='\n')	//la versione
		oneline[i++]=chin;
	oneline[i]='\0';
	token=(char *)strtok(oneline,"=");

	if(!strcmp(token,"VERSIONE"))
	{
		token=(char *)strtok(NULL,token);
		f_ver=atoi(token);
	}

	i=0;																//seconda riga: AR del Sole
	while((chin=toupper(fgetc(in)))!='\n')
		oneline[i++]=chin;
	oneline[i]='\0';
	token=(char *)strtok(oneline,"=");

	if(!strcmp(token,"AR SOLE"))
	{
		token=(char *)strtok(NULL,"\t ;");
		sun_ar=hms2d(atof(token));
	}

	i=0;																//terza riga: AR della Luna
	while((chin=toupper(fgetc(in)))!='\n')
		oneline[i++]=chin;
	oneline[i]='\0';
	token=(char *)strtok(oneline,"=");

	if(!strcmp(token,"AR LUNA"))
	{
		token=(char *)strtok(NULL,"\t ;");
		moon_ar=hms2d(atof(token));
	}

	i=0;																//quarta riga: Latitudine iniziale
	while((chin=toupper(fgetc(in)))!='\n')
		oneline[i++]=chin;
	oneline[i]='\0';

	token=(char *)strtok(oneline,"=");
	if(!strcmp(token,"LATITUDINE"))
	{
		token=(char *)strtok(NULL,"\t ;");
		latitude=hms2d(atof(token));
	}

	i=0;																//quinta riga: fondo del cielo
	while((chin=toupper(fgetc(in)))!='\n')
		oneline[i++]=chin;
	oneline[i]='\0';

	token=(char *)strtok(oneline,"=");
	if(!strcmp(token,"LUCE CIELO"))
	{
		token=(char *)strtok(NULL,"\t ;");
		horlight=atoi(token);
	}

	i=0;																//sesta riga: AR Mercurio
	while((chin=toupper(fgetc(in)))!='\n')
		oneline[i++]=chin;
	oneline[i]='\0';

	token=(char *)strtok(oneline,"=");
	if(!strcmp(token,"AR MERCURIO"))
	{
		token=(char *)strtok(NULL,"\t ;");
		mercury_ar=hms2d(atof(token));
	}

	i=0;																//settima riga: AR Venere
	while((chin=toupper(fgetc(in)))!='\n')
		oneline[i++]=chin;
	oneline[i]='\0';

	token=(char *)strtok(oneline,"=");
	if(!strcmp(token,"AR VENERE"))
	{
		token=(char *)strtok(NULL,"\t ;");
		venus_ar=hms2d(atof(token));
	}

	i=0;																//ottava riga: AR Marte
	while((chin=toupper(fgetc(in)))!='\n')
		oneline[i++]=chin;
	oneline[i]='\0';

	token=(char *)strtok(oneline,"=");
	if(!strcmp(token,"AR MARTE"))
	{
		token=(char *)strtok(NULL,"\t ;");
		mars_ar=hms2d(atof(token));
	}

	i=0;																//nona riga: AR Giove
	while((chin=toupper(fgetc(in)))!='\n')
		oneline[i++]=chin;
	oneline[i]='\0';

	token=(char *)strtok(oneline,"=");
	if(!strcmp(token,"AR GIOVE"))
	{
		token=(char *)strtok(NULL,"\t ;");
		jupiter_ar=hms2d(atof(token));
	}

	i=0;																//decima riga: AR Saturno
	while((chin=toupper(fgetc(in)))!='\n')
		oneline[i++]=chin;
	oneline[i]='\0';

	token=(char *)strtok(oneline,"=");
	if(!strcmp(token,"AR SATURNO"))
	{
		token=(char *)strtok(NULL,"\t ;");
		saturn_ar=hms2d(atof(token));
	}

	i=0;																//undecima riga: max val stelle
	while((chin=toupper(fgetc(in)))!='\n')
		oneline[i++]=chin;
	oneline[i]='\0';

	token=(char *)strtok(oneline,"=");
	if(!strcmp(token,"MAX STELLE"))
	{
		token=(char *)strtok(NULL,"\t ;");
		maxstars=atoi(token);
	}

	i=0;																//undecima riga: max val stelle
	while((chin=toupper(fgetc(in)))!='\n')
		oneline[i++]=chin;
	oneline[i]='\0';

	token=(char *)strtok(oneline,"=");
	if(!strcmp(token,"VELOCITA"))
	{
		token=(char *)strtok(NULL,"\t ;");
		speed=atoi(token);
	}

	fclose(in);
	return(0);
}
