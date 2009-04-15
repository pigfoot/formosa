
#ifndef _BBS_SCREEN_H_
#define _BBS_SCREEN_H_


/*
 *  有關於螢幕處理的定義與資料結構
 */

#define ANSILINELEN (246)       /* Maximum Screen width in chars */

/* Line buffer modes */
#define MODIFIED (1)      /* if line has been modifed, output to screen */
#define STANDOUT (2)      /* if this line has a standout region */

struct screenline {
    unsigned char oldlen ;       /* previous line length              */
    unsigned char len ;          /* current length of line            */
    unsigned char mode ;         /* status of line, as far as update  */
    unsigned char smod ;         /* start of modified data            */
    unsigned char emod ;         /* end of modified data              */
    unsigned char sso ;          /* start stand out */
    unsigned char eso ;          /* end stand out */
    unsigned char data[ANSILINELEN] ;
#if 1
    unsigned char width ;
    unsigned char scmod ;
    unsigned char ecmod ;
#endif
} ;

typedef struct {
    int row, col;
    int y, x;
    void *raw_memory;
} screen_backup_t;


#endif	/*_BBS_SCREEN_H_ */
