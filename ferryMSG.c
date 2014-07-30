#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <wait.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Definitions used to control simulation termination
#define DBG_AUTO_STOP_TIME 5000
#define MIN_VEHICLE_ARRIVAL_INTERVAL 100

// Definitions used for msging
#define MSG_TYPE_CAR 1
#define MSG_TYPE_TRUCK 2

typedef struct mymsgbuf
{
	long mtype;
	int num;
} mess_t;

// We will use the same buffer format (and size) for each msg, no matter the queue 
mess_t msg;
int msgSize;

int queueCaptainToVehicles;
int queueCaptainToFerry;
int queueVehiclesToCaptain;

int mainProcessID;
int vehicleCreationProcessPID;
int vehicleProcessGID;
int truckArrivalProb;
int maxTimeToNextArrival;

int timeChange( const struct timeval startTime );
void init();
void cleanup();

int carProcess() {
	int localpid = getpid();
	setpgid(localpid, vehicleProcessGID);
	msg.mtype = MSG_TYPE_CAR;
	// msgsnd sends a _copy_ of the msg to the queue, so no need to instantiate additional msgs
	if(msgsnd(queueVehiclesToCaptain, &msg, msgSize, 0) != 0) {
		return -1;
	}


	return 0;
}

int truckProcess() {
	int localpid = getpid();
	setpgid(localpid, vehicleProcessGID);
	msg.mtype = MSG_TYPE_TRUCK;
	if(msgsnd(queueVehiclesToCaptain, &msg, msgSize, 0) != 0) {
		return -1;
	}


	return 0;
}

int captainProcess() {

	return 0;
}

int vehicleCreationProcess() {
	int localpid = getpid();
	// Time at start of process creation
    struct timeval startTime;
	// Time from start of process creation to current time
    int elapsed = 0;
	// Time at which last vehicle arrived
    int lastArrivalTime = 0;

    printf("CREATECREATECREATECR   vehicleCreationProcess created with PID: %8d \n", localpid);
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
		  printf("CREATECREATECREATECR   elapsed time %d arrival time %d\n", elapsed, lastArrivalTime); 
		  if(lastArrivalTime > 0 ) { 
			  if(rand() % 100 < truckArrivalProb ) {
				  /* This is a truck */
				  // If the fork fails for any reason, stop the simulation
				  printf("CREATECREATECREATECR   Created a truck process\n");
				  int childPID = fork(); 
				  if(childPID == 0) {
					  return truckProcess();
				  } else if(childPID == -1) {
					  return -1;
				  }
			  }
			  else {
				  /* This is a car */
				  printf("CREATECREATECREATECR   Created a car process\n");
				  int childPID = fork(); 
				  if(childPID == 0) {
					  return carProcess();
				  } else if(childPID == -1) {
					  return -1;
				  }
			  }
		  }
		  lastArrivalTime += rand()% maxTimeToNextArrival;
		  printf("CREATECREATECREATECR   present time %d, next arrival time %d\n", elapsed, lastArrivalTime);
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

	  if(elapsed > DBG_AUTO_STOP_TIME)
		  break;
    }

    printf("CREATECREATECREATECR   EXITING FROM CREATE\n");
	return 0;
}

int main() {
	int status;
	init();

    printf("Please enter integer values for the following variables\n");

	// Truck probability
	printf("Enter the percent probability that the next vehicle is a truck (0..100): ");
	scanf("%d", &truckArrivalProb);
	while(truckArrivalProb < 0 || truckArrivalProb > 100) {
		printf("Probability must be between 0 and 100: ");
		scanf("%d", &truckArrivalProb);
	}

	// Vehicle interval
	printf("Enter the maximum length of the interval between vehicles (%d..MAX_INT): ", MIN_VEHICLE_ARRIVAL_INTERVAL);
	scanf("%d", &maxTimeToNextArrival);
	while(maxTimeToNextArrival < MIN_VEHICLE_ARRIVAL_INTERVAL) {
		printf("Interval must be greater than %d: ", MIN_VEHICLE_ARRIVAL_INTERVAL);
		scanf("%d", &maxTimeToNextArrival);
	}

	if((vehicleCreationProcessPID = fork()) == 0) {
		vehicleCreationProcess();
		return 0;
	}

	// Wait for vehicleCreationProcess to finish
    printf("ret: %d\n", waitpid(vehicleCreationProcessPID, &status, 0));
	

    printf("waited\n");

	cleanup();

	return 0;
}

void init() {
	// The mtype is not sent with our msg, so exclude this from the size of each msg
	// Additionally, we will use the same buffer format (and size) for each queue
	msgSize = sizeof(mess_t) - sizeof(long);
	key_t msgkey;
	msgkey = ftok("ferryMSG.c",'v');
	if((queueCaptainToVehicles = msgget(msgkey, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT   failed to create queue queueCaptainToVehicles\n");
		exit(1);
	}
	msgkey = ftok("ferryMSG.c.",'f');
	if((queueCaptainToFerry = msgget(msgkey, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT   failed to create queue queueCaptainToFerry\n");
		exit(1);
	}
	msgkey = ftok("ferryMSG.c.",'c');
	if((queueVehiclesToCaptain = msgget(msgkey, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT   failed to create queue queueVehiclesToCaptain\n");
		exit(1);
	}
	printf("INITINITINITINITINIT   All queues successfully created\n");
	mainProcessID = getpid();
	vehicleProcessGID = mainProcessID - 1;
}

void cleanup() {
	msgctl(queueCaptainToVehicles, IPC_RMID, 0);
	msgctl(queueCaptainToFerry, IPC_RMID, 0);
	msgctl(queueVehiclesToCaptain, IPC_RMID, 0);

	if(killpg(vehicleProcessGID, SIGKILL) == -1 && errno == EPERM) {
		printf("XXXCLEANUPCLEANUPCLE   vehicles not killed\n");
	}
	printf("XXXCLEANUPCLEANUPCLE   all vehicles killed\n");
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
