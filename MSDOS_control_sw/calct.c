void CalcTime(float ar,float arS,int *thr,int *tmn,float *tcalc)
{
	*tcalc=ar-arS+12;
	if(*tcalc>=24)
		*tcalc-=24;
	if(*tcalc<0)
		*tcalc+=24;
	*thr=(int)*tcalc;
	*tmn=(int)((*tcalc-*thr)*60);
}
