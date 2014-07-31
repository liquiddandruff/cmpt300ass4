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
#define MIN_VEHICLE_ARRIVAL_INTERVAL 100
#define MAX_LOADS 11
#define MAX_SPACES_ON_FERRY 6
#define MAX_TRUCKS_ON_FERRY 2

// Definitions used for msgs
#define MSG_TYPE_CAR 1
#define MSG_TYPE_TRUCK 2
#define MSG_TYPE_ONBOARD 3
#define MSG_TYPE_TRAVELING 4
#define MSG_TYPE_ARRIVED 5
#define MSG_TYPE_UNLOAD 6
#define MSG_VEHICLE_IS_WAITING 7
#define MSG_VEHICLE_IS_LATE 8
#define MSG_SWITCH_TO_WAITING 9

#define SIZE_OF_CAR 1
#define SIZE_OF_TRUCK 2

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
// Only msgs from vehicles notifying the captain that they are waiting to board the ferry are in this queue
int queueVehiclesToCaptain;
int queueVehiclesWaiting;
int queueVehiclesLate;
int queueVehiclesBoarding;
int queueVehiclesOnboard;

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
	printf("CARCAR CARCAR CARCAR   Car %d arrives at ferry dock\n", localpid);
	msg.mtype = MSG_TYPE_CAR; msg.pid = localpid;
	// msgsnd sends a _copy_ of the msg to the queue, so no need to instantiate additional msgs
	// since same msg structure is used, we can use same msgSize for each msgsnd
	// return failure code upon msgsnd failure 
	if(msgsnd(queueVehiclesToCaptain, &msg, msgSize, 0) == -1) { return -1; }

	// Wait for signal from captain telling this vehicle that the ferry is about to load
	// Blocks. Only looks at the mtypes concerning the PID of this process. Checks msg.data 
	// to determine if the vehicle is waiting or late
	while(1) {
		printf("CARCAR CARCAR CARCAR   Car %d was late from last load, but is still here.\n", localpid);
		msg.mtype = MSG_TYPE_CAR; msg.pid = localpid;
		if(msgsnd(queueVehiclesToCaptain, &msg, msgSize, 0) == -1) { return -1; }

		if(msgrcv(queueCaptainToVehicles, &msg, msgSize, localpid, 0) == -1) { return -1; }

		msg.mtype = MSG_TYPE_CAR; msg.pid = localpid;
		if(msg.data == MSG_VEHICLE_IS_LATE) {
			printf("CARCAR CARCAR CARCAR   Car %d acks that it is %s\n", localpid,  "late");
			if(msgsnd(queueVehiclesLate, &msg, msgSize, 0) == -1) { return -1; }
		} else {
			printf("CARCAR CARCAR CARCAR   Car %d acks that it is %s\n", localpid,  "waiting");
			if(msgsnd(queueVehiclesWaiting, &msg, msgSize, 0) == -1) { return -1; }
		}

		// Block until vehicle receives signal to leave its current queue and start boarding the ferry
		if(msgrcv(queueVehiclesBoarding, &msg, msgSize, localpid, 0) == -1) { return -1; }
		if(msg.data != MSG_SWITCH_TO_WAITING)
			break;
	}
	printf("CARCAR CARCAR CARCAR   Car %d acks Captain's signal and is leaving queue to be loaded onto ferry\n", localpid);

	printf("CARCAR CARCAR CARCAR   Car %d is onboard the ferry\n", localpid);
	msg.mtype = MSG_TYPE_ONBOARD; msg.data = MSG_TYPE_CAR; msg.pid = localpid;
	if(msgsnd(queueVehiclesOnboard, &msg, msgSize, 0) == -1) { return -1; }

	// Block until vehicle receives signal to sail
	if(msgrcv(queueVehiclesOnboard, &msg, msgSize, MSG_TYPE_TRAVELING, 0) == -1) { return -1; }
	printf("CARCAR CARCAR CARCAR   Car %d acks Captain's signal that it is sailing\n", localpid);

	// Block until vehicle receives signal that it has arrived to destination
	if(msgrcv(queueVehiclesOnboard, &msg, msgSize, MSG_TYPE_ARRIVED, 0) == -1) { return -1; }
	printf("CARCAR CARCAR CARCAR   Car %d acks Captain's signal that it has arrived\n", localpid);

	// Block until vehicle receives signal to unload
	if(msgrcv(queueVehiclesOnboard, &msg, msgSize, MSG_TYPE_UNLOAD, 0) == -1) { return -1; }
	printf("CARCAR CARCAR CARCAR   Car %d acks Captain's signal to unload. Car unloads from ferry and exits.\n", localpid);

	return 0;
}

int truckProcess() {
	int localpid = getpid();
	setpgid(localpid, vehicleProcessGID);
	printf("TRUCKTRUCK TRUCKTRUC   Truck %d arrives at ferry dock\n", localpid);
	msg.mtype = MSG_TYPE_TRUCK; msg.pid = localpid;
	if(msgsnd(queueVehiclesToCaptain, &msg, msgSize, 0) == -1) { return -1; }
	
	while(1) {
		printf("TRUCKTRUCK TRUCKTRUC   Truck %d was late from last load, but is still here\n", localpid);
		msg.mtype = MSG_TYPE_TRUCK; msg.pid = localpid;
		if(msgsnd(queueVehiclesToCaptain, &msg, msgSize, 0) == -1) { return -1; }

		if(msgrcv(queueCaptainToVehicles, &msg, msgSize, localpid, 0) == -1) { return -1; }

		msg.mtype = MSG_TYPE_TRUCK; msg.pid = localpid;
		if(msg.data == MSG_VEHICLE_IS_LATE) {
			printf("TRUCKTRUCK TRUCKTRUC   Truck %d acks that it is %s\n", localpid,  "late");
			if(msgsnd(queueVehiclesLate, &msg, msgSize, 0) == -1) { return -1; }
		} else {
			printf("TRUCKTRUCK TRUCKTRUC   Truck %d acks that it is %s\n", localpid,  "waiting");
			if(msgsnd(queueVehiclesWaiting, &msg, msgSize, 0) == -1) { return -1; }
		}

		if(msgrcv(queueVehiclesBoarding, &msg, msgSize, localpid, 0) == -1) { return -1; }
		if(msg.data != MSG_SWITCH_TO_WAITING)
			break;
	}
	printf("TRUCKTRUCK TRUCKTRUC   Truck %d acks Captain's signal and is leaving queue to be loaded onto ferry\n", localpid);

	printf("TRUCKTRUCK TRUCKTRUC   Truck %d is onboard the ferry\n", localpid);
	msg.mtype = MSG_TYPE_ONBOARD; msg.data = MSG_TYPE_TRUCK; msg.pid = localpid;
	if(msgsnd(queueVehiclesOnboard, &msg, msgSize, 0) == -1) { return -1; }

	if(msgrcv(queueVehiclesOnboard, &msg, msgSize, MSG_TYPE_TRAVELING, 0) == -1) { return -1; }
	printf("TRUCKTRUCK TRUCKTRUC   Truck %d acks Captain's signal that it is sailing\n", localpid);

	if(msgrcv(queueVehiclesOnboard, &msg, msgSize, MSG_TYPE_ARRIVED, 0) == -1) { return -1; }
	printf("TRUCKTRUCK TRUCKTRUC   Truck %d acks Captain's signal that it has arrived\n", localpid);

	if(msgrcv(queueVehiclesOnboard, &msg, msgSize, MSG_TYPE_UNLOAD, 0) == -1) { return -1; }
	printf("TRUCKTRUCK TRUCKTRUC   Truck %d acks Captain's signal to unload. Truck unloads from ferry and exits\n", localpid);

	return 0;
}

int captainProcess() {
	char *vehicleType = "";
	int currentLoad = 1;
	int numberOfCarsLoaded = 0;
	int numberOfTrucksLoaded = 0;
	int numberOfSpacesFilled = 0;
	int counter = 0;

	printf("CAPTAIN CAPTAIN CAPT   CaptainProcess created. PID is: %d\n", getpid());

	while(currentLoad < MAX_LOADS) {
		numberOfTrucksLoaded = 0;
		numberOfCarsLoaded = 0;
		numberOfSpacesFilled = 0;
		
		printf("CAPTAIN CAPTAIN CAPT   load %d/%d started\n", currentLoad, MAX_LOADS);
		printf("CAPTAIN CAPTAIN CAPT   The ferry is about to load!\n");
		// Receive all msgs from queue sent from vehicles, break until queue is empty
		// Tell these vehicles that they are in the main lane (they are waiting vehicles)
		while(msgrcv(queueVehiclesToCaptain, &msg, msgSize, 0, IPC_NOWAIT) != -1) {
			vehicleType = msg.mtype == MSG_TYPE_CAR ? "Car" : "Truck";
			printf("CAPTAIN CAPTAIN CAPT   Captain tells %s %d that it is waiting\n", vehicleType, msg.pid);
			msg.mtype = msg.pid; msg.data = MSG_VEHICLE_IS_WAITING;
			if(msgsnd(queueCaptainToVehicles, &msg, msgSize, 0) == -1) {
				return -1;
			}
		}

		while(numberOfSpacesFilled < MAX_SPACES_ON_FERRY) {
			// Tell all vehicles that are just now arriving that they are late
			while(msgrcv(queueVehiclesToCaptain, &msg, msgSize, 0, IPC_NOWAIT) != -1) {
				vehicleType = msg.mtype == MSG_TYPE_CAR ? "Car" : "Truck";
				printf("CAPTAIN CAPTAIN CAPT   Captain tells %s %d that it is late\n", vehicleType, msg.pid);
				msg.mtype = msg.pid; msg.data = MSG_VEHICLE_IS_LATE;
				if(msgsnd(queueCaptainToVehicles, &msg, msgSize, 0) == -1) {
					return -1;
				}
			}

			// Signal first two waiting trucks, if conditions are satisfied
			while(numberOfTrucksLoaded < MAX_TRUCKS_ON_FERRY && 
					numberOfSpacesFilled + SIZE_OF_TRUCK <= MAX_SPACES_ON_FERRY &&
					msgrcv(queueVehiclesWaiting, &msg, msgSize, MSG_TYPE_TRUCK, IPC_NOWAIT) != -1) {
				printf("CAPTAIN CAPTAIN CAPT   Captain tells Truck %d to leave waiting queue\n", msg.pid);
				msg.mtype = msg.pid;
				if(msgsnd(queueVehiclesBoarding, &msg, msgSize, 0) == -1) {
					return -1;
				}
				numberOfTrucksLoaded++;
				numberOfSpacesFilled += SIZE_OF_TRUCK;
			}

			// Signal the rest of the waiting cars
			while(numberOfSpacesFilled + SIZE_OF_CAR <= MAX_SPACES_ON_FERRY &&
					msgrcv(queueVehiclesWaiting, &msg, msgSize, MSG_TYPE_CAR, IPC_NOWAIT) != -1) {
				printf("CAPTAIN CAPTAIN CAPT   Captain tells Car %d to leave waiting queue\n", msg.pid);
				msg.mtype = msg.pid;
				if(msgsnd(queueVehiclesBoarding, &msg, msgSize, 0) == -1) {
					return -1;
				}
				numberOfCarsLoaded++;
				numberOfSpacesFilled += SIZE_OF_CAR;
			}

			// Do the same with late vehicles
			while(numberOfTrucksLoaded < MAX_TRUCKS_ON_FERRY && 
					numberOfSpacesFilled + SIZE_OF_TRUCK <= MAX_SPACES_ON_FERRY &&
					msgrcv(queueVehiclesLate, &msg, msgSize, MSG_TYPE_TRUCK, IPC_NOWAIT) != -1) {
				printf("CAPTAIN CAPTAIN CAPT   Captain tells Truck %d to leave late queue\n", msg.pid);
				msg.mtype = msg.pid;
				if(msgsnd(queueVehiclesBoarding, &msg, msgSize, 0) == -1) {
					return -1;
				}
				numberOfTrucksLoaded++;
				numberOfSpacesFilled += SIZE_OF_TRUCK;
			}
			while(numberOfSpacesFilled + SIZE_OF_CAR <= MAX_SPACES_ON_FERRY &&
					msgrcv(queueVehiclesLate, &msg, msgSize, MSG_TYPE_CAR, IPC_NOWAIT) != -1) {
				printf("CAPTAIN CAPTAIN CAPT   Captain tells Car %d to leave late queue\n", msg.pid);
				msg.mtype = msg.pid;
				if(msgsnd(queueVehiclesBoarding, &msg, msgSize, 0) == -1) {
					return -1;
				}
				numberOfCarsLoaded++;
				numberOfSpacesFilled += SIZE_OF_CAR;
			}

			// Check for number of vehicles onboard
			while(msgrcv(queueVehiclesOnboard, &msg, msgSize, MSG_TYPE_ONBOARD, IPC_NOWAIT) != -1) {
				vehicleType = msg.data == MSG_TYPE_CAR ? "Car" : "Truck";
				printf("CAPTAIN CAPTAIN CAPT   Captain acks that %s %d is onboard\n", vehicleType, msg.pid);
			}

			// If we are indeed at the maximum that we can carry, signal all vehicles to travel
			if(numberOfCarsLoaded * SIZE_OF_CAR + numberOfTrucksLoaded * SIZE_OF_TRUCK  == MAX_SPACES_ON_FERRY) {
				printf("CAPTAIN CAPTAIN CAPT   The ferry is full!\n");
				printf("CAPTAIN CAPTAIN CAPT   Captain tells the boarded vehicles that they are sailing\n");
				msg.mtype = MSG_TYPE_TRAVELING;
				for(counter = 0; counter < numberOfCarsLoaded + numberOfTrucksLoaded; counter++) 
					if(msgsnd(queueVehiclesOnboard, &msg, msgSize, 0) == -1) { return -1; }
				printf("CAPTAIN CAPTAIN CAPT   All vehicles ack'd they are sailing\n");
				// Teleport!
				printf("CAPTAIN CAPTAIN CAPT   Arrived at destination and docked\n");
				printf("CAPTAIN CAPTAIN CAPT   Captain tells the boarded vehicles that they have arrived\n");
				msg.mtype = MSG_TYPE_ARRIVED;
				for(counter = 0; counter < numberOfCarsLoaded + numberOfTrucksLoaded; counter++) 
					if(msgsnd(queueVehiclesOnboard, &msg, msgSize, 0) == -1) { return -1; }
				printf("CAPTAIN CAPTAIN CAPT   All vehicles ack'd they have arrived\n");
				// Unload!
				printf("CAPTAIN CAPTAIN CAPT   Captain tells the boarded vehicles to unload\n");
				msg.mtype = MSG_TYPE_UNLOAD;
				for(counter = 0; counter < numberOfCarsLoaded + numberOfTrucksLoaded; counter++) 
					if(msgsnd(queueVehiclesOnboard, &msg, msgSize, 0) == -1) { return -1; }
				printf("CAPTAIN CAPTAIN CAPT   All vehicles have unloaded\n");
				// Done, but we need to move all currently late vehicles to waiting
				while(msgrcv(queueVehiclesLate, &msg, msgSize, 0, IPC_NOWAIT) != -1) {
					msg.mtype = msg.pid; msg.data = MSG_SWITCH_TO_WAITING;
					if(msgsnd(queueVehiclesBoarding, &msg, msgSize, 0) == -1) { return -1; }
				}
				printf("CAPTAIN CAPTAIN CAPT   All late vehicles now waiting\n");
				
				
			}

		}

		printf("\n\n\n");
		printf("CAPTAIN CAPTAIN CAPT   Arrived at main loading dock from destination\n");
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
	if((queueCaptainToVehicles = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT   failed to create queue queueCaptainToVehicles\n");
		exit(1);
	}
	if((queueVehiclesToCaptain = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT   failed to create queue queueVehiclesToCaptain\n");
		exit(1);
	}
	if((queueVehiclesWaiting = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT   failed to create queue queueVehiclesWaiting\n");
		exit(1);
	}
	if((queueVehiclesLate = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT   failed to create queue queueVehiclesLate\n");
		exit(1);
	}
	if((queueVehiclesBoarding = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT   failed to create queue queueVehiclesBoarding\n");
		exit(1);
	}
	if((queueVehiclesOnboard = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("INITINITINITINITINIT   failed to create queue queueVehiclesOnboard\n");
		exit(1);
	}
	printf("INITINITINITINITINIT   All queues successfully created\n");
	mainProcessID = getpid();
	vehicleProcessGID = mainProcessID - 1;
}

void cleanup() {
	int w, status;
	msgctl(queueCaptainToVehicles, IPC_RMID, 0);
	msgctl(queueVehiclesToCaptain, IPC_RMID, 0);
	msgctl(queueVehiclesWaiting, IPC_RMID, 0);
	msgctl(queueVehiclesLate, IPC_RMID, 0);
	msgctl(queueVehiclesBoarding, IPC_RMID, 0);
	msgctl(queueVehiclesOnboard, IPC_RMID, 0);

	if(killpg(vehicleProcessGID, SIGKILL) == -1 && errno == EPERM) {
		printf("XXXCLEANUP CLEANUPCL   vehicle processes not killed\n");
	}
	printf("XXXCLEANUP CLEANUPCL   all vehicle processes killed\n");

	while( (w = waitpid( -1, &status, WNOHANG)) > 1){
			printf("                           REAPED process in cleanup%d\n", w);
	}

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
