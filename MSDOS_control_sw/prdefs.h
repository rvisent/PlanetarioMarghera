// PRDEFS.H

#ifndef _PRDEFS_
#define _PRDEFS_

#ifndef _TYP_
#define _TYP_

typedef enum { FALSE, TRUE } BOOL;
typedef unsigned char BYTE;
typedef unsigned int WORD;
typedef unsigned long DWORD;

#endif

// prototipi

// print.c
int print(char *, ...);
void PrintReinit();
char *WinGets(char *pszString);

#endif

