
#ifndef _BBS_IO_H_
#define _BBS_IO_H_

/* Flags to getdata input function */
#define XNOECHO	0x00
#define XECHO	0x01
#define XLCASE	0x02
#define NUMECHO	0x04	/* PTT Compatible */
#define GCARRY	0x08	/* PTT Compatible */
#define XNOSP	0x10

/* Formosa */
#define ECHONOSP 	  (XECHO|XNOSP)

/* PTT Compatible */
#define NOECHO	XNOECHO
#define	DOECHO	XECHO
#define LCECHO	XLCASE

#endif /* _BBS_IO_H_ */
