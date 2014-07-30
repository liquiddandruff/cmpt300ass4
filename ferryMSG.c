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
#define DBG_AUTO_STOP_TIME 5000000000
#define MIN_VEHICLE_ARRIVAL_INTERVAL 100
#define MAX_LOADS 11
#define MAX_SPOTS_ON_FERRY 6
#define MAX_TRUCKS_ON_FERRY 2

// Definitions used for msgs
#define MSG_TYPE_CAR 1
#define MSG_TYPE_TRUCK 2

typedef struct mymsgbuf
{
	long mtype; // the type of msg
	int data; // data pertaining to msg
	int pid; // used as process (vehicle) identifier
} mess_t;

// We will use the same buffer format (and size) for each msg, no matter the queue 
mess_t msg;
int msgSize;

// Queue IDS
int queueCaptainToVehicles;
int queueCaptainToFerry;
// Only msgs from vehicles notifying the captain that they are waiting to board the ferry are in this queue
int queueVehiclesToCaptain;
// Queue to tell the other process to terminate when the current process wants to terminate  with other process 
int queueMutualTermination;

// Process IDS
int mainProcessID;
int captainProcessPID;
int vehicleCreationProcessPID;
int vehicleProcessGID;

int truckArrivalProb;
int maxTimeToNextVehicleArrival;

// Forward declarations
int timeChange( const struct timeval startTime );
void init();
void cleanup();

int carProcess() {
	int localpid = getpid();
	setpgid(localpid, vehicleProcessGID);
	printf("CARCAR CARCAR CARCAR   PID %d arrives at ferry dock\n", localpid);
	msg.mtype = MSG_TYPE_CAR; msg.pid = localpid;
	// msgsnd sends a _copy_ of the msg to the queue, so no need to instantiate additional msgs
	// since same msg structure is used, we can use same msgSize for each msgsnd
	// return failure code upon msgsnd failure 
	if(msgsnd(queueVehiclesToCaptain, &msg, msgSize, 0) != 0) {
		return -1;
	}
	//msgrcv..
	printf("CARCAR CARCAR CARCAR   PID %d acks that it is a waiting vehicle\n", localpid);


	return 0;
}

int truckProcess() {
	int localpid = getpid();
	setpgid(localpid, vehicleProcessGID);
	printf("TRUCKTRUCK TRUCKTRUC   PID %d arrives at ferry dock\n", localpid);
	msg.mtype = MSG_TYPE_TRUCK; msg.pid = localpid;
	if(msgsnd(queueVehiclesToCaptain, &msg, msgSize, 0) != 0) {
		return -1;
	}
	printf("TRUCKTRUCK TRUCKTRUC   PID %d acks that it is a waiting vehicle\n", localpid);

	return 0;
}

int captainProcess() {
	char *vehicleType = "";
	int currentLoad = 1;
	int numberOfCarsQueued = 0;
	int numberOfTrucksQueued = 0;
	int numberOfTrucksLoaded = 0;
	int numberOfSpacesFilled = 0;
	int numberOfSpacesEmpty = 0;
	int numberOfVehicles = 0;
	int counter = 0;

	printf("CAPTAIN CAPTAIN CAPT   CaptainProcess created. PID is: %d\n", getpid());

	while(currentLoad <= MAX_LOADS) {
		printf("CAPTAIN CAPTAIN CAPT   load %d/%d started\n", currentLoad, MAX_LOADS);

		numberOfTrucksLoaded = 0;
		numberOfSpacesFilled = 0;
		numberOfVehicles = 0;
		
		printf("CAPTAIN CAPTAIN CAPT   The ferry is about to load!\n");
		// Recieve all vehicles from queue, break until queue is empty
		// Tell these vehicles that they are in the main lane (they are waiting vehicles)
		while(msgrcv(queueVehiclesToCaptain, &msg, msgSize, 0, IPC_NOWAIT) != -1) {
			vehicleType = msg.mtype == MSG_TYPE_CAR ? "Car" : "Truck";
			printf("CAPTAIN CAPTAIN CAPT   %s %d is a waiting vehicle\n", vehicleType, msg.pid);
		}

		//while(numberOfSpacesFilled < MAX_SPOTS_ON_FERRY && numberOfTrucksLoaded < MAX_SPOTS_ON_FERRY) {
		while(numberOfSpacesFilled < MAX_SPOTS_ON_FERRY) {
			/*
			numberOfTrucksQueued = getNumMsgTypeInQueue(queueVehiclesToCaptain, MSG_TYPE_TRUCK);
			numberOfCarsQueued = getNumMsgTypeInQueue(queueVehiclesToCaptain, MSG_TYPE_CAR);
			msgrcv(queueVehiclesToCaptain, &msg, msgSize, MSG_TYPE_TRUCK, IPC_NOWAIT); */
			while(msgrcv(queueVehiclesToCaptain, &msg, msgSize, 0, IPC_NOWAIT) != -1) {
				vehicleType = msg.mtype == MSG_TYPE_CAR ? "Car" : "Truck";
				printf("CAPTAIN CAPTAIN CAPT   %s %d is a late vehicle\n", vehicleType, msg.pid);
				numberOfSpacesFilled += msg.mtype == MSG_TYPE_CAR ? 1 : 2;
			}

		}

		currentLoad++;
	}

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

	printf("CREATE CREATE CREATE   vehicleCreationProcess created. PID is: %d \n", localpid);
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
			printf("CREATE CREATE CREATE   elapsed time %d arrival time %d\n", elapsed, lastArrivalTime); 
			if(lastArrivalTime > 0 ) { 
				if(rand() % 100 < truckArrivalProb ) {
					/* This is a truck */
					// If the fork fails for any reason, stop the simulation
					printf("CREATE CREATE CREATE   Created a truck process\n");
					int childPID = fork(); 
					if(childPID == 0) {
						return truckProcess();
					} else if(childPID == -1) {
						return -1;
					}
				}
				else {
					/* This is a car */
					printf("CREATE CREATE CREATE   Created a car process\n");
					int childPID = fork(); 
					if(childPID == 0) {
						return carProcess();
					} else if(childPID == -1) {
						return -1;
					}
				}
			}
			lastArrivalTime += rand()% maxTimeToNextVehicleArrival;
			printf("CREATE CREATE CREATE   present time %d, next arrival time %d\n", elapsed, lastArrivalTime);
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

	printf("CREATE CREATE CREATE   vehicleCreationProcess ended\n");
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
	scanf("%d", &maxTimeToNextVehicleArrival);
	while(maxTimeToNextVehicleArrival < MIN_VEHICLE_ARRIVAL_INTERVAL) {
		printf("Interval must be greater than %d: ", MIN_VEHICLE_ARRIVAL_INTERVAL);
		scanf("%d", &maxTimeToNextVehicleArrival);
	}

	if((captainProcessPID = fork()) == 0) {
		return captainProcess();
	}

	if((vehicleCreationProcessPID = fork()) == 0) {
		return vehicleCreationProcess();
	}

	// Wait for captainProcess to finish
	waitpid(captainProcessPID, &status, 0);
	printf("captainProcess returnValue: %d\n", WEXITSTATUS(status));
	// Once process captainProcess returns, the simulation should end, so send sigkill to vehicleCreationProcess
	kill(vehicleCreationProcessPID, SIGKILL);
	// Wait for vehicleCreationProcess to finish
	waitpid(vehicleCreationProcessPID, &status, 0);
	printf("vehicleCreationProcess returnValue: %d\n", WEXITSTATUS(status));

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
	int w, status;
	msgctl(queueCaptainToVehicles, IPC_RMID, 0);
	msgctl(queueCaptainToFerry, IPC_RMID, 0);
	msgctl(queueVehiclesToCaptain, IPC_RMID, 0);

	if(killpg(vehicleProcessGID, SIGKILL) == -1 && errno == EPERM) {
		printf("XXXCLEANUP CLEANUPCL   vehicle processes not killed\n");
	}
	printf("XXXCLEANUP CLEANUPCL   all vehicle processes killed\n");

	while( (w = waitpid( -1, &status, WNOHANG)) > 1){
			printf("                           REAPED process in cleanup%d\n", w);
	}

}

int getNumMsgTypeInQueue(int queue_id, int msg_type) {
	int num = 0;


	return num;
}

int timeChange( const struct timeval startTime ) {
	struct timeval nowTime;
	long int elapsed;
	int elapsedTime;

	gettimeofday(&nowTime, NULL);
	elapsed = (nowTime.tv_sec - startTime.tv_sec) * 1000000 + (nowTime.tv_usec - startTime.tv_usec);
	elapsedTime = elapsed / 1000;
	return elapsedTime;
}
