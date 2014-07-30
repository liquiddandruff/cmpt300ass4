#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
/* Redefines the message structure */
typedef struct mymsgbuf
{
	long mtype;
	int num;
} mess_t;

int main()
{
	int qid;
	key_t msgkey;
	pid_t pid;
	mess_t buf;
	int length;
	int cont;
	int count = 0;
	length = sizeof(mess_t) - sizeof(long);
	msgkey = ftok(".",'m');
	qid = msgget(msgkey, IPC_CREAT | 0660);
	if(!(pid = fork())){
		printf("SON - QID = %d\n", qid);
		srand (time (0));
		for(cont = 0; cont < 10; cont++){
			sleep (rand()%4);
			buf.mtype = 1;
			buf.num = rand()%100;
			msgsnd(qid, &buf, length, 0);
			printf("SON - MESSAGE NUMBER %d: %d %ld\n", cont+1, buf.num, buf.mtype);
		}
		for(cont = 0; cont < 10; cont++){
			sleep (rand()%4);
			buf.mtype = 2;
			buf.num = rand()%100;
			msgsnd(qid, &buf, length, 0);
			printf("SON - MESSAGE NUMBER %d: %d %ld\n", cont+1, buf.num, buf.mtype);
		}
		return 0;
	}
	printf("FATHER - QID = %d\n", qid);
	for(cont = 0; cont < 10; cont++){
		count = 0;
		sleep (rand()%4);
		while( msgrcv(qid, &buf, length, 2, IPC_NOWAIT) == -1) {
			count++;
			if(count%10000 == 0)printf ("%d", cont);
		}
		printf("FATHER - MESSAGE NUMBER %d: %d %ld\n", cont+1, buf.num, buf.mtype);
	}
	for(cont = 0; cont < 10; cont++){
		sleep (rand()%4);
		while( msgrcv(qid, &buf, length, 1, IPC_NOWAIT) == -1) {
			printf ("waiting on msg %d of type 1\n", cont);
		}
		printf("FATHER - MESSAGE NUMBER %d: %d %ld\n", cont+1, buf.num, buf.mtype);
	}
	msgctl(qid, IPC_RMID, 0);
	return 0;
}

