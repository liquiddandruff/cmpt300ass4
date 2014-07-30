all: clean ferryMSG q3 ql ferryThread

ferryMSG:
	gcc -o ferryMSG ferryMSG.c

q3:
	gcc -o mq3proc mq3proc.c

ql:
	gcc -o mqlproc mqlproc.c

ferryThread:
	gcc -o ferryThread ferryThread.c -lpthread

clean:
	rm ferryMSG mq3proc mqlproc ferryThread
