#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/time.h>


/* Constants */
#define TOTAL_SPOTS_ON_FERRY 6
#define LOAD_MAX 3


/* Threads to create and control vehiclepthreads */
pthread_t vehicleCreationThread;
pthread_t captainThread;
pthread_t vehicleThread[200];
int threadCounter = 0;


/* mutexes for counter protection and the counters they protect */
pthread_mutex_t protectCarsQueued;
int carsQueuedCounter = 0;
pthread_mutex_t protectTrucksQueued;
int trucksQueuedCounter = 0;
pthread_mutex_t protectCarsUnloaded;
int carsUnloadedCounter = 0;
pthread_mutex_t protectTrucksUnloaded;
int trucksUnloadedCounter = 0;


/* counting semaphores to control cars and trucks */
sem_t carsQueued;
sem_t trucksQueued;
sem_t carsLoaded;
sem_t trucksLoaded;
sem_t vehiclesSailing;
sem_t vehiclesArrived;
sem_t waitUnload;
sem_t readyUnload;
sem_t carsUnloaded;
sem_t trucksUnloaded;
sem_t waitToExit;


struct timespec tm;
int fullSpotsOnFerry = 0;  
int isTruck = 0;          /* probability that */
                          /* next vehicle is a truck */
int maxTimeToNextArrival; /* maximum number of milliseconds */
                          /* between vehicle arrivals*/
int truckArrivalProb;     /* User supplied probability that the */


/* pointers to functions that will become threads */
void* captain();
void* truck();
void* car();
void* createVehicle();
int sem_waitChecked(sem_t *semaphoreID);
int sem_postChecked(sem_t *semaphoreID);
int sem_initChecked(sem_t *semaphoreID, int pshared, unsigned int value);
int sem_destroyChecked(sem_t *semaphoreID);
int pthread_mutex_lockChecked(pthread_mutex_t *mutexID);
int pthread_mutex_unlockChecked(pthread_mutex_t *mutexID); 
int pthread_mutex_initChecked(pthread_mutex_t *mutexID, 
                              const pthread_mutexattr_t *attrib);
int pthread_mutex_destroyChecked(pthread_mutex_t *mutexID);


int timeChange( const struct timeval startTime );

int main()
{
    /* Create semaphores */
    sem_initChecked(&carsQueued, 0, 0);
    sem_initChecked(&trucksQueued, 0, 0);
    sem_initChecked(&carsLoaded, 0, 0);
    sem_initChecked(&trucksLoaded, 0, 0);
    sem_initChecked(&vehiclesSailing, 0, 0);
    sem_initChecked(&vehiclesArrived, 0, 0);
    sem_initChecked(&waitUnload, 0, 0);
    sem_initChecked(&readyUnload, 0, 0);
    sem_initChecked(&carsUnloaded, 0, 0);
    sem_initChecked(&trucksUnloaded, 0, 0);
    sem_initChecked(&waitToExit, 0, 0);

    /* set time value for timed wait */
    tm.tv_sec = 0;
    tm.tv_nsec = maxTimeToNextArrival * 300000;

    printf("Please enter integer values for the following variables\n");
    /* timing of car and truck creation */
    printf("Enter the percent probability that the next vehicle is a truck\n");
    scanf("%d", &truckArrivalProb );
    maxTimeToNextArrival = 0;
    while( maxTimeToNextArrival < 100) {
        printf("Enter the maximum length of the interval between vehicles\n");
        printf("time interval should be >1000\n");
        scanf("%d", &maxTimeToNextArrival );
    }



    /* Beginning the vehicle creation process */
    /* Beginning the captain process */
    pthread_create(&vehicleCreationThread, NULL, createVehicle, NULL);
    pthread_create(&captainThread, NULL, captain, NULL);
    pthread_mutex_initChecked(&protectCarsQueued, NULL);
    pthread_mutex_initChecked(&protectTrucksQueued, NULL);
    pthread_mutex_initChecked(&protectCarsUnloaded, NULL);
    pthread_mutex_initChecked(&protectTrucksUnloaded, NULL);


    /* Waiting for the vechicle creation process to end */
    /* Waiting for the captain process to end */
    pthread_join(vehicleCreationThread, NULL);
    pthread_join(captainThread, NULL);
    pthread_mutex_destroyChecked(&protectTrucksQueued);
    pthread_mutex_destroyChecked(&protectCarsQueued);
    pthread_mutex_destroyChecked(&protectCarsUnloaded);
    pthread_mutex_destroyChecked(&protectTrucksUnloaded);


    /* destroy all semaphores created */
    sem_destroyChecked(&carsQueued);
    sem_destroyChecked(&trucksQueued);
    sem_destroyChecked(&carsLoaded);
    sem_destroyChecked(&trucksLoaded);
    sem_destroyChecked(&vehiclesSailing);
    sem_destroyChecked(&vehiclesArrived);
    sem_destroyChecked(&waitUnload);
    sem_destroyChecked(&readyUnload);
    sem_destroyChecked(&carsUnloaded);
    sem_destroyChecked(&trucksUnloaded);
    sem_destroyChecked(&waitToExit);

    return 0;
}
 

void* createVehicle()
{
    struct timeval startTime;   /* time at start of program execution */
    int elapsed = 0;          /* time from start of program execution */
    int lastArrivalTime = 0;    /* time from start of program execution */
                              /* at which the last vehicle arrived */

    printf("CREATECREATECREATECREATE       creation thread \n");
    
    /*  Initialize start time */
    gettimeofday(&startTime, NULL);
    elapsed = timeChange(startTime);
    srand (elapsed*1000+44);
    while(1) { 
      /* If present time is later than arrival time of the next vehicle */
      /*         Determine if the vehicle is a car or truck */
      /*         Have the vehicle put a message in the queue indicating */
      /*         that it is in line */
      /* Then determine the arrival time of the next vehicle */ 
      elapsed = timeChange(startTime);
      while(elapsed >= lastArrivalTime) {
      printf("CREATECREATECREATECREATE      elapsed time %d arrival time %d\n", 
		elapsed, lastArrivalTime); 
          if(lastArrivalTime > 0 ) { 
              isTruck = rand() % 100;
              if(isTruck >  truckArrivalProb ) {
                  /* This is a car */
		    pthread_create(&(vehicleThread[threadCounter]), 
		    NULL, car, NULL);
		    printf("CREATECREATECREATECREATE   ");
		    printf("   Created a car thread\n");
              }
              else {
                  /* This is a truck */
		    /*pthread_t vehicleThread;*/
		    pthread_create(&(vehicleThread[threadCounter]), 
		    NULL, truck, NULL);
		    printf("CREATECREATECREATECREATE   ");
		    printf("   Created a truck thread\n");
              }
		threadCounter++;
          }
          lastArrivalTime += rand()% maxTimeToNextArrival;
          printf("CREATECREATECREATECREATE   ");
	    printf("   present time %d, next arrival time %d\n",
          elapsed, lastArrivalTime);
      }
    }
    printf("CREATECREATECREATECREATE      EXITING FROM CREATE\n");
}


void* truck()
{
    unsigned long int *threadId;

    threadId = (unsigned long int *)pthread_self();
    /* truck arrives in the queue */
    pthread_mutex_lockChecked(&protectTrucksQueued);
    trucksQueuedCounter++;
    pthread_mutex_unlockChecked(&protectTrucksQueued);
    printf("TRUCK%5luTRUCK     truck Queued\n", *threadId);
    sem_waitChecked(&trucksQueued);
    printf("TRUCK%5luldTRUCK   truck leaving queue to load\n", *threadId);
 
   
    /* Captain has signalled this car */
    /* Truck loads onto ferry the signals captain */
    printf("TRUCK%5luldTRUCK   truck onboard ferry\n", *threadId);
    sem_postChecked(&trucksLoaded);


    /* wait until ferry is full then sail */
    sem_waitChecked(&vehiclesSailing);
    printf("TRUCK%5luldTRUCK   truck travelling\n", *threadId);


    /* Sail across the river */
    /* Wait until the captain signals that the ferry has arrived */
    sem_waitChecked(&vehiclesArrived);
    printf("TRUCK%5luldTRUCK   truck arrived at destination\n", *threadId);
    sem_postChecked(&readyUnload);


    /* Unload ferry */
    sem_waitChecked(&waitUnload);
    pthread_mutex_lockChecked(&protectTrucksUnloaded);
    trucksUnloadedCounter++;
    pthread_mutex_unlockChecked(&protectTrucksUnloaded);
    printf("TRUCK%5luTRUCK     Truck unloaded from ferry\n", *threadId);
    sem_postChecked(&trucksUnloaded);

    sem_waitChecked(&waitToExit);
    printf("TRUCK%5luTRUCK     truck exits\n", *threadId);
    pthread_exit(0);
}


void* car()
{
    unsigned long int *threadId;

    threadId = (unsigned long int *)pthread_self();


    /* car arrives in the queue */
    pthread_mutex_lockChecked(&protectCarsQueued);
    carsQueuedCounter++;
    pthread_mutex_unlockChecked(&protectCarsQueued);
    printf("CARCAR%5luCARCAR   car Queued\n", *threadId);
    sem_waitChecked(&carsQueued);
    printf("CARCAR%5luCARCAR   car leaving queue to load\n", *threadId);
    
    /* Captain has signalled this car */
    /* Car loads onto ferry */
    printf("CARCAR%5luCARCAR   car onboard ferry \n", *threadId);
    sem_postChecked(&carsLoaded);

    /* wait till the ferry is full and will across the river */
    sem_waitChecked(&vehiclesSailing);
    printf("CARCAR%5luCARCAR   car traveling\n", *threadId);
    sem_waitChecked(&vehiclesArrived);
    printf("CARCAR%5luCARCAR   car arrived at destination\n", *threadId);
    sem_postChecked(&readyUnload);

    /* Unload ferry */
    sem_waitChecked(&waitUnload);
    pthread_mutex_lockChecked(&protectCarsUnloaded);
    carsUnloadedCounter++;
    pthread_mutex_unlockChecked(&protectCarsUnloaded);
    printf("CARCAR%5luCARCAR   unloaded from ferry\n", *threadId);
    sem_postChecked(&carsUnloaded);

    sem_waitChecked(&waitToExit);
    printf("CARCAR%5luCARCAR   car exits\n", *threadId);
    pthread_exit(0);
}


void* captain()
{
    /* counting of cars and trucks to determine full ferry load */
    int loads = 0;            /* # of loads of vehicles delivered*/
    int numberOfCarsQueued = 0;
    int numberOfTrucksQueued = 0;
    int numberOfTrucksLoaded = 0;
    int numberOfSpacesFilled = 0;
    int numberOfSpacesEmpty = 0;
    int numberOfVehicles = 0;
    int counter = 0;
 
    printf("CAPTAINCAPTAIN             captain process started \n");
    while (loads < LOAD_MAX) 
    {
      printf("CAPTAINCAPTAIN                loop started\n");
      /* load vehicles from the waiting queues. */
	/* first initialize local counters for this load */
	numberOfTrucksLoaded = 0;
	numberOfSpacesFilled = 0;
	numberOfVehicles = 0;

	/* The first time through the looop load trucks and cars */
	/* from loading queue ignoring cars and trucks that arrive */
	/* during loading by using */
	/* (use local values taken at beginning of loading) */
	/* Next consider the vehicles that arrive during and after loading */
	/* by repeating the same loop until the ferry is full */
	while(numberOfSpacesFilled < TOTAL_SPOTS_ON_FERRY) {
	    pthread_mutex_lockChecked(&protectTrucksQueued);
	    pthread_mutex_lockChecked(&protectCarsQueued);
	    numberOfTrucksQueued = trucksQueuedCounter;
	    numberOfCarsQueued = carsQueuedCounter;
   	    pthread_mutex_unlockChecked(&protectCarsQueued);
   	    pthread_mutex_unlockChecked(&protectTrucksQueued);
	    while(numberOfTrucksQueued > 0 && 
	        numberOfSpacesFilled < TOTAL_SPOTS_ON_FERRY-1 && 
		numberOfTrucksLoaded < 2) {
    	    	pthread_mutex_lockChecked(&protectTrucksQueued);
	    	trucksQueuedCounter--;
		printf(
                "CAPTAINCAPTAIN                Truck signalled to leave queue \n");
	    	sem_postChecked(&trucksQueued);
    	    	pthread_mutex_unlockChecked(&protectTrucksQueued);
	    	numberOfTrucksQueued--;
	    	numberOfTrucksLoaded++;
	    	numberOfSpacesFilled+=2;
                numberOfVehicles++;
    	    }
	    while(numberOfCarsQueued > 0 && 
		numberOfSpacesFilled < TOTAL_SPOTS_ON_FERRY) {
    	    	pthread_mutex_lockChecked(&protectCarsQueued);
	    	carsQueuedCounter--;
		printf(
		"CAPTAINCAPTAIN                Car signalled to leave queue\n");
	    	sem_postChecked(&carsQueued);
    	    	pthread_mutex_unlockChecked(&protectCarsQueued);
		numberOfCarsQueued--;
	    	numberOfSpacesFilled++;
                numberOfVehicles++;
    	    }
	}


        for(counter = 0; counter < numberOfTrucksLoaded; counter++) {
	    sem_waitChecked(&trucksLoaded);
	    printf("CAPTAINCAPTAIN                Truck loaded\n");
    	}
        for(counter = 0; counter < numberOfVehicles - numberOfTrucksLoaded;
	    counter++) {
	    sem_waitChecked(&carsLoaded);
	    printf("CAPTAINCAPTAIN                Car loaded\n");
	}
	printf("\nCAPTAINCAPTAIN              		ferry full, sailing\n\n");

 
        /* signal all vehicles waiting to sail */
        for(counter = 0; counter < numberOfVehicles; counter++) {
	    printf(
          "CAPTAINCAPTAIN                vehicle %d has acked it is sailing\n",
		            counter);
	    sem_postChecked(&vehiclesSailing);
        }
	printf("CAPTAINCAPTAIN           	all vehicles sailing\n");


        /* when all vehicles have replied they are sailing */
        /* the ferry sets sail then reaches its destination */
        for(counter = 0; counter < numberOfVehicles; counter++) {
	    printf(
          "CAPTAINCAPTAIN                vehicle %d has acked it has arrived\n",
		            counter);
	    sem_postChecked(&vehiclesArrived);
        }
        /* signal vehicles to unload */
        for(counter = 0; counter < numberOfVehicles; counter++) {
            sem_waitChecked(&readyUnload);
        }
	printf( "\nCAPTAINCAPTAIN                		arrived at destination and docked\n\n");

        /* signal vehicles to unload */
        for(counter = 0; counter < numberOfVehicles; counter++) {
	      sem_postChecked(&waitUnload);
        }

      	/* the captain signals the vehicles to unload */
      	/* then waits for vehicles to unload */
	numberOfSpacesEmpty = 0;
	while(numberOfSpacesEmpty < TOTAL_SPOTS_ON_FERRY) {
    	    pthread_mutex_lockChecked(&protectCarsUnloaded);
	    if( carsUnloadedCounter > 0 ){
              sem_waitChecked(&carsUnloaded);
		carsUnloadedCounter--;
		numberOfSpacesEmpty++;
	      printf(
              "CAPTAINCAPTAIN                car has unloaded from the ferry\n");
	    }
    	    pthread_mutex_unlockChecked(&protectCarsUnloaded);
		
    	    pthread_mutex_lockChecked(&protectTrucksUnloaded);
	    if( trucksUnloadedCounter > 0 ){
		sem_waitChecked(&trucksUnloaded);
		trucksUnloadedCounter--;
		numberOfSpacesEmpty+=2;
	    	printf(
            "CAPTAINCAPTAIN                truck has unloaded from the ferry\n");
	    }
    	    pthread_mutex_unlockChecked(&protectTrucksUnloaded);
	}
	printf(
	"\nCAPTAINCAPTAIN              		unloading complete\n\n");

        for(counter = 0; counter < numberOfVehicles; counter++) {
	    printf(
          "CAPTAINCAPTAIN                unloaded vehicle %d is about to exit\n",
		            counter);
    	    sem_post(&waitToExit);
        }
	printf("\n\n\n\n");
	printf(
	"CAPTAINCAPTAIN              		arrived at loading dock\n\n");
	loads++;
    }
    exit(0);
}


int timeChange( const struct timeval startTime )
{
    struct timeval nowTime;
    long int elapsed;
    int elapsedTime;

    gettimeofday(&nowTime, NULL);
    elapsed = (nowTime.tv_sec - startTime.tv_sec) * 1000000
          + (nowTime.tv_usec - startTime.tv_usec);
    elapsedTime = elapsed / 1000;
    return elapsedTime;
}


int sem_waitChecked(sem_t *semaphoreID)
{
        int returnValue;
	returnValue = sem_wait(semaphoreID);
	if (returnValue == -1 ) {
	    printf("semaphore wait failed: simulation terminating\n");
	    exit(0);
	}
	return returnValue;
}

int sem_postChecked(sem_t *semaphoreID) 
{
        int returnValue;
	returnValue = sem_post(semaphoreID);
	if (returnValue < 0 ) {
	    printf("semaphore post operation failed: simulation terminating\n");
	    exit(0);
	}
	return returnValue;
}

int sem_initChecked(sem_t *semaphoreID, int pshared, unsigned int value) 
{
        int returnValue;
	returnValue = sem_init(semaphoreID, pshared, value);
	if (returnValue < 0 ) {
	    printf("semaphore init operation failed: simulation terminating\n");
	    exit(0);
	}
	return returnValue;
}

int sem_destroyChecked(sem_t *semaphoreID) 
{
        int returnValue;
	returnValue = sem_destroy(semaphoreID);
	if (returnValue < 0 ) {
	    printf("semaphore destroy operation failed: terminating\n");
	    exit(0);
	}
	return returnValue;
}

int pthread_mutex_lockChecked(pthread_mutex_t *mutexID) 
{
        int returnValue;
	returnValue = pthread_mutex_lock(mutexID);
	if (returnValue < 0 ) {
	    printf("pthread mutex lock operation failed: terminating\n");
	    exit(0);
	}
	return returnValue;
}

int pthread_mutex_unlockChecked(pthread_mutex_t *mutexID) 
{
        int returnValue;
	returnValue = pthread_mutex_unlock(mutexID);
	if (returnValue < 0 ) {
	    printf("pthread mutex unlock operation failed: terminating\n");
	    exit(0);
	}
	return returnValue;
}

int pthread_mutex_initChecked(pthread_mutex_t *mutexID, 
                                       const pthread_mutexattr_t *attrib) 
{
        int returnValue;
	returnValue = pthread_mutex_init(mutexID, attrib);
	if (returnValue < 0 ) {
	    printf("pthread init operation failed: simulation terminating\n");
	    exit(0);
	}
	return returnValue;
}

int pthread_mutex_destroyChecked(pthread_mutex_t *mutexID)
{
        int returnValue;
	returnValue = pthread_mutex_destroy(mutexID);
	if (returnValue < 0 ) {
	    printf("pthread destroy failed: simulation terminating\n");
	    exit(0);
	}
	return returnValue;
}
