#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <wait.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MIN_VEHICLE_ARRIVAL_INTERVAL 100

typedef struct mymsgbuf
{
	long mtype;
	int num;
} mess_t;

int queueCaptainToVehicles;
int queueCaptainToFerry;
int queueVehiclesToCaptain;

int parentProcessID = -1;
int vehicleProcessGID = -1;
int truckArrivalProb = 0;
int maxTimeToNextArrival = 0;

int timeChange( const struct timeval startTime );
void init();
void cleanup();

void carProcess() {
	int localpid = getpid();
	setpgid(localpid, vehicleProcessGID);

	return;
}

void truckProcess() {
	int localpid = getpid();
	setpgid(localpid, vehicleProcessGID);

	return;
}

void vehicleCreationProcess() {
	printf("vehicleCreation\n");
    struct timeval startTime;   /* time at start of program execution */
    int elapsed = 0;          /* time from start of program execution */
    int lastArrivalTime = 0;    /* time from start of program execution */
                              /* at which the last vehicle arrived */

    printf("CREATECREATECREATECREATE       creation process \n");
    
    /*  Initialize start time */
    gettimeofday(&startTime, NULL);
    elapsed = timeChange(startTime);
    srand (elapsed*1000+44);
	int zombieTick = 0;
    while(1) { 
      /* If present time is later than arrival time of the next vehicle */
      /*         Determine if the vehicle is a car or truck */
      /*         Have the vehicle put a message in the queue indicating */
      /*         that it is in line */
      /* Then determine the arrival time of the next vehicle */ 
	
      elapsed = timeChange(startTime);
	  if(elapsed >= lastArrivalTime) {
		  printf("CREATECREATECREATECREATE      elapsed time %d arrival time %d\n", elapsed, lastArrivalTime); 
		  if(lastArrivalTime > 0 ) { 
			  if(rand() % 100 < truckArrivalProb ) {
				  /* This is a truck */
				  printf("CREATECREATECREATECREATE   ");
				  printf("   Created a truck process\n");
				  if(fork() == 0) {
					  truckProcess();
					  return;
				  }
			  }
			  else {
				  /* This is a car */
				  printf("CREATECREATECREATECREATE   ");
				  printf("   Created a car process\n");
				  if(fork() == 0) {
					  carProcess();
					  return;
				  }
			  }
		  }
		  lastArrivalTime += rand()% maxTimeToNextArrival;
		  printf("CREATECREATECREATECREATE   ");
		  printf("   present time %d, next arrival time %d\n", elapsed, lastArrivalTime);
	  }
	  zombieTick++;
	  // Purge all zombies every 10 iteratinos
	  if(zombieTick % 10 == 0) {
		  zombieTick -= 10;
		  // arg1 is -1 to wait for all child processes
		  int w = 0; int status = 0;
		  while( (w = waitpid( -1, &status, WNOHANG)) > 1){
			  printf("                           REAPED zombie process %d from vehicle creation process\n", w);
		  }
	  }

    }
    printf("CREATECREATECREATECREATE      EXITING FROM CREATE\n");
}

int main() {
	init();

    printf("Please enter integer values for the following variables\n");

    /* timing of car and truck creation */
	printf("Enter the percent probability that the next vehicle is a truck (0..100): ");
	scanf("%d", &truckArrivalProb);
	while(truckArrivalProb < 0 || truckArrivalProb > 100) {
		printf("Probability must be between 0 and 100: ");
		scanf("%d", &truckArrivalProb);
	}

	printf("Enter the maximum length of the interval between vehicles (%d..MAX_INT): ", MIN_VEHICLE_ARRIVAL_INTERVAL);
	scanf("%d", &maxTimeToNextArrival);
	while(maxTimeToNextArrival < MIN_VEHICLE_ARRIVAL_INTERVAL) {
		printf("Interval must be greater than %d: ", MIN_VEHICLE_ARRIVAL_INTERVAL);
		scanf("%d", &maxTimeToNextArrival);
	}

	if(fork() == 0) {
		vehicleCreationProcess();
		return 0;
	}
	printf("parent\n");

	cleanup();

	return 0;
}

void init() {
	key_t msgkey;
	msgkey = ftok("ferryMSG.c",'v');
	if((queueCaptainToVehicles = msgget(msgkey, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT failed to create queue queueCaptainToVehicles\n");
		exit(1);
	}
	msgkey = ftok("ferryMSG.c.",'f');
	if((queueCaptainToFerry = msgget(msgkey, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT failed to create queue queueCaptainToFerry\n");
		exit(1);
	}
	msgkey = ftok("ferryMSG.c.",'c');
	if((queueVehiclesToCaptain = msgget(msgkey, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT failed to create queue queueVehiclesToCaptain\n");
		exit(1);
	}
	parentProcessID = getpid();
	vehicleProcessGID = parentProcessID - 1;
}

void cleanup() {
	msgctl(queueCaptainToVehicles, IPC_RMID, 0);
	msgctl(queueCaptainToFerry, IPC_RMID, 0);
	msgctl(queueVehiclesToCaptain, IPC_RMID, 0);

	if(killpg(vehicleProcessGID, SIGKILL) == -1 && errno == EPERM) {
		printf("XXCLEANUPCLEANUPCLEA   vehicles not killed\n");
	}
	printf("XXCLEANUPCLEANUPCLEA   vehicles killed\n");
}

int timeChange( const struct timeval startTime ) {
    struct timeval nowTime;
    long int elapsed;
    int elapsedTime;

    gettimeofday(&nowTime, NULL);
    elapsed = (nowTime.tv_sec - startTime.tv_sec) * 1000000
          + (nowTime.tv_usec - startTime.tv_usec);
    elapsedTime = elapsed / 1000;
    return elapsedTime;
}
