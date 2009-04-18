
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <syslog.h>
#include "libproto.h"

	/* ref. semctl() */
#ifdef SOLARIS
	union semun {
		long val;
		struct semid_ds *buf;
		ushort *array;
	};
#elif	defined(LINUX)
	/* according to X/OPEN we have to define it ourselves */
	union semun {
		int val;                    /* value for SETVAL */
		struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
		unsigned short int *array;  /* array for GETALL, SETALL */
		struct seminfo *__buf;      /* buffer for IPC_INFO */
	};
#endif

int sem_init(key_t key)
{
	int semid;
	union semun arg;

	arg.val = 1;
	semid = semget(key, 1, 0);
	if (semid == -1)
	{
		semid = semget(key, 1, IPC_CREAT | 0600);
		if (semid == -1)
		{
			fprintf(stderr, "[semget error] key = %d\n", key);
			fflush(stderr);
			exit(1);
		}
		semctl(semid, 0, SETVAL, (union semun)arg);
	}
	return semid;
}


void sem_cleanup(int sem_id)
{
#if 0
	union semun ick;

	/* this is ignored anyhow */
	ick.val = 0;
	semctl(sem_id, 0, IPC_RMID, ick);
#endif
	semctl(sem_id, 0, IPC_RMID);
}

void sem_lock(int semid, int op)
{
	struct sembuf sops;
	int rc;

	sops.sem_num = 0;
	sops.sem_flg = SEM_UNDO;
	sops.sem_op = op;
	do {
		rc = semop(semid, &sops, 1);
	} while (rc == -1 &&
		(errno == EAGAIN || errno == EINTR));

	if (rc == -1) {
		openlog("bbsd", LOG_NDELAY | LOG_NOWAIT, LOG_LOCAL0);
		syslog(LOG_ERR, "Getting utmp Lock error.");
		exit(1);
	}
}
