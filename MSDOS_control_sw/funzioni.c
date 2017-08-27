float d2hms(float dd)
{
	int		hours;
	float	mins;

	hours=(int)dd;
	mins=(dd-hours)*0.6;
	return(hours+mins);
}
/******************************************************/
float hms2d(float hms)
{
	int 	hours;
	float	decimals;

	hours=(int)hms;
	decimals=(hms-hours)*10/6;
	return(hours+decimals);
}
