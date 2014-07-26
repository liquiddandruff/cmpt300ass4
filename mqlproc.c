/* Code from http://www.linuxfocus.org/English/March2003/article287.shtml */
#include <stdio.h>
#include <stdlib.h>
#include <linux/ipc.h>
#include <linux/msg.h>
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
			printf("SON - MESSAGE NUMBER %d: %d\n", cont+1, buf.num);
		}
		return 0;
	}
	printf("FATHER - QID = %d\n", qid);
	for(cont = 0; cont < 10; cont++){
		sleep (rand()%4);
		msgrcv(qid, &buf, length, 1, 0);
		printf("FATHER - MESSAGE NUMBER %d: %d\n", cont+1, buf.num);
	}
	msgctl(qid, IPC_RMID, 0);
	return 0;
}

