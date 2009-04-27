#ifndef _BBS_IPC_H_
#define _BBS_IPC_H_

/* lang.c */
#define ELANGSHM_KEY		0x1529
#define CLANGSHM_KEY		0x1629
#define UTMPSEM_KEY             1129

/*
 * bbsweb
	define share memory & cache parameter
*/
#define SERVER_SHM_KEY				((key_t) 0x1928)
#define SERVER_SEM_KEY				((key_t) 0x0100)
#define FILE_SHM_KEY				((key_t) 0x1939-1)
#define HTML_SHM_KEY				((key_t) 0x1940+1)

/* mod_shm.c */
#define UTMPSHM_KEY	0x1129	/* shared memory key, should be unique */
#define BRDSHM_KEY	0x1329
#define CLASSHM_KEY	0x1429

/* mod_crosscheck.c */
#define CROSSHM_KEY	0x1829

#endif /* _BBS_IPC_H_ */
