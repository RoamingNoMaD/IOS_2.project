/* IOS - 2. project
 * Author: Janos Laszlo Vasik (xvasik05) 
 * File: proj2.c 
 * Program description: This software synchronizes processes to solve a problem based on "The barbershop problem",
 * which can be found in the following document in English: 
 * (https://greenteapress.com/semaphores/LittleBookOfSemaphores.pdf).
 * Or in the file "IOS-project2.pdf" in Czech. */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h> 
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h> // Shared memory
#include <semaphore.h> // Semaphore data type
#include <unistd.h> // Standard symbolic constants and types (getpid())

// Definition of the services of the post office.
#define MAIL 1
#define PACKAGE 2
#define MONEY 3

// ***************************** Variables ***************************** //

// Structure for parameters
typedef struct parameters{
    int NZ; // Number of cutomers.
    int NU; // Number of workers.
    int TZ; // Maximum time for customer to get to the post office.
    int TU; // Maximum length of break for worker.
    int F;  // Maximum time to close the post office.
} Param;

FILE *fileP;
bool *isOpen; // Post office open flag (1 = open || 0 = closed) 

// Counters 
int *commandNO;     // Number of current action.
int *mailLine;      // Number of people waiting for mail
int *packageLine;   // Number of people waiting for a package
int *moneyLine;     // Number of people waiting for money

// Semaphores 
sem_t   *fileAcc,
        *postOpen,
        *lineCtrl,
        *mail,
        *package,
        *money;

// ***************************** Helper functions ***************************** //

// Function to deallocate shared memory and destroy semaphores.
void release(){ 
    if( (sem_close(fileAcc)) == -1  ||
        (sem_close(postOpen)) == -1 ||
        (sem_close(lineCtrl)) == -1 ||
        (sem_close(mail)) == -1     ||
        (sem_close(package)) == -1  ||
        (sem_close(money)) == -1    ||
        (sem_unlink("/xvasik05_fileAcc")) == -1     ||
        (sem_unlink("/xvasik05_postOpen")) == -1    ||
        (sem_unlink("/xvasik05_lineCtrl")) == -1    ||
        (sem_unlink("/xvasik05_mail")) == -1        ||
        (sem_unlink("/xvasik05_package")) == -1     ||
        (sem_unlink("/xvasik05_money")) == -1       ){
            fprintf(stderr,"Error while closing semaphore(s).\n");
            exit(1);
    } else if ( (munmap(commandNO, sizeof(int)) == -1)      ||
                (munmap(isOpen, sizeof(bool)) == -1)        ||
                (munmap(mailLine, sizeof(int)) == -1)       ||
                (munmap(packageLine, sizeof(int)) == -1)    ||
                (munmap(moneyLine, sizeof(int)) == -1)      ){
            fprintf(stderr,"Error while deallocating shared memory.\n");
            exit(1);
    }
}

// Error print and cleanup function.
int error(char* msg, bool allocated, bool fileOpen){ 
    fprintf(stderr,"%s\n", msg);
    if (allocated == true){
        release();
    }
    if (fileOpen == true){
        fclose(fileP);
    }
    exit(1);
}

// Function to search for any character that's not a number.
void nanCheck(char *str){
    if(*str != 0){
        error("ERROR: Invalid format of parameters. Use numeric values.", false, false);
    }
}

// Function to check the emptyness of lines
int lineEmpty(){
    if(*mailLine != 0){
        return 1;
    } else if (*packageLine != 0){
        return 2;
    } else if (*moneyLine != 0){
        return 3;
    } else {
        return 0;
    }
}

// ***************************** Child process functions ***************************** //

int worker(int maxTime, int wID){
    srand(time(NULL) * getpid());
    bool stop = false;
    int service;

    sem_wait(fileAcc);
    printf("%d: U %d: started\n", *commandNO, wID);
    *commandNO += 1;
    sem_post(fileAcc);

    while(stop == false){
        sem_wait(lineCtrl);
        if ((service = lineEmpty()) == 0){ // no waiting customers
            sem_post(lineCtrl);

            sem_wait(postOpen);
            if(*isOpen == true){
                sem_wait(fileAcc);
                printf("%d: U %d: taking break\n", *commandNO, wID);
                *commandNO += 1;
                sem_post(fileAcc);
                sem_post(postOpen);
                
                usleep(1000 * (rand() % (maxTime + 1)));

                sem_wait(fileAcc);
                printf("%d: U %d: break finished\n", *commandNO, wID);
                *commandNO += 1;
                sem_post(fileAcc);
            } else {
                sem_post(postOpen);
                stop = true;
                sem_wait(fileAcc);
                printf("%d: U %d: going home\n", *commandNO, wID);
                *commandNO += 1;
                sem_post(fileAcc);
            }
        } else {
            sem_wait(fileAcc);
            printf("%d: U %d: serving a service of type %d\n", *commandNO, wID, service);
            *commandNO += 1;
            sem_post(fileAcc);

            switch (service)
            {
            case MAIL:
                *mailLine -= 1; 
                sem_post(mail);
                break;
            case PACKAGE:
                *packageLine -= 1; 
                sem_post(package);
                break;
            case MONEY:
                *moneyLine -= 1; 
                sem_post(money);
                break;
            }
            sem_post(lineCtrl);

            usleep(1000 * (rand() % 11));

            sem_wait(fileAcc);
            printf("%d: U %d: service finished\n", *commandNO, wID);
            *commandNO += 1;
            sem_post(fileAcc);
        }
    }

    fclose(fileP);
    exit(0);
}

int customer(int maxTime, int cID){
    srand(time(NULL) * getpid());

    sem_wait(fileAcc);
    printf("%d: Z %d: started\n", *commandNO, cID);
    *commandNO += 1;
    sem_post(fileAcc);

    usleep(1000 * (rand() % (maxTime + 1)));

    // checking open post office
    sem_wait(postOpen);
    sem_wait(fileAcc);
    if(*isOpen == false){
        printf("%d: Z %d: going home\n", *commandNO, cID);
        *commandNO += 1;
        sem_post(fileAcc);
        sem_post(postOpen);

        fclose(fileP);
        exit(0);
    }
    int service = (rand() % 3) + 1;
    printf("%d: Z %d: entering office for a service %d\n", *commandNO, cID, service);
    *commandNO += 1;
    sem_post(fileAcc);
    sem_post(postOpen);

    // waiting in line for given service (sync point with worker)
    sem_wait(lineCtrl);
    switch (service) 
    {
    case MAIL:
        *mailLine += 1;
        sem_post(lineCtrl);
        sem_wait(mail);
        break;
    case PACKAGE:
        *packageLine += 1;
        sem_post(lineCtrl);
        sem_wait(package);
        break;
    case MONEY:
        *moneyLine += 1;
        sem_post(lineCtrl);
        sem_wait(money);
        break;
    }
    
    sem_wait(fileAcc);
    printf("%d: Z %d: called by office worker\n", *commandNO, cID);
    *commandNO += 1;
    sem_post(fileAcc);

    usleep(1000 * (rand() % 11));

    sem_wait(fileAcc);
    printf("%d: Z %d: going home\n", *commandNO, cID);
    *commandNO += 1;
    sem_post(fileAcc);
    
    fclose(fileP);
    exit(0);
}

// ************************************ Main function ************************************ //

int main(int argc, char* argv[]){

// ***************************** Parameter parsing ***************************** //
    
    Param params;
    char *end = NULL; // Variable for any non-numeric values from conversion.

    // Cheking for correct number of parameters.
    if(argc != 6){
        error("ERROR: Invalid format of parameters. Use all 5 of them.", false, false);
    }

    // Cheking for empty parameters.
    for (int i = 1; i < 5; i++){
        if (strcmp(argv[i], "") == 0){
            error("ERROR: Invalid format of parameters. Missing parameter.", false, false);
        }
    }

    // Converting arguments from char* to long. 
    params.NZ = strtol(argv[1], &end, 10);
    nanCheck(end);
    params.NU = strtol(argv[2], &end, 10);
    nanCheck(end);
    params.TZ = strtol(argv[3], &end, 10);
    nanCheck(end);
    params.TU = strtol(argv[4],&end, 10);
    nanCheck(end);
    params.F  = strtol(argv[5],&end, 10);
    nanCheck(end);

    // Cheking for invalid range of time parameters, and 0 workers.
    if(params.TZ < 0 || params.TU < 0 || params.F < 0 ||params.TZ > 10000 || params.TU > 100 || params.F > 10000 || params.NU <= 0 || params.NZ < 0){
        error("ERROR: Invalid range of parameters.", false, false);
    }

// ***************************** Output file operations ***************************** //

    if (access("proj2.out",F_OK) == 0){ // file exists
        remove("proj2.out");
    }
    
    fileP = freopen("proj2.out", "w+", stdout);
    if(fileP == NULL){
        error("ERROR: While opening output file.", false, false);
    }
    setbuf(fileP, NULL); // Setting file to non-buffered.

// ***************************** Memory allocations ***************************** //

    // Allocating isOpen flag and counters in shared memory.
    if( (commandNO = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))    == MAP_FAILED ||
        (isOpen = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))      == MAP_FAILED ||
        (mailLine = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))     == MAP_FAILED ||
        (moneyLine = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))    == MAP_FAILED ||
        (packageLine = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))  == MAP_FAILED ){
            error("ERROR: Allocation of shared memory failed.", false, true);
    // Allocating and initializing semaphores.
    } else if(  (fileAcc = sem_open("xvasik05_fileAcc", O_CREAT, 0666, 1))     == SEM_FAILED ||
                (postOpen = sem_open("xvasik05_postOpen", O_CREAT, 0666, 1))   == SEM_FAILED ||
                (lineCtrl = sem_open("xvasik05_lineCtrl", O_CREAT, 0666, 1))   == SEM_FAILED ||
                (mail = sem_open("xvasik05_mail", O_CREAT, 0666, 0))           == SEM_FAILED ||
                (package = sem_open("xvasik05_package", O_CREAT, 0666, 0))     == SEM_FAILED ||
                (money = sem_open("xvasik05_money", O_CREAT, 0666, 0))         == SEM_FAILED ){
                    error("ERROR: Initialization of semaphore(s) failed.", false, true);
    }

    // Defining counter starting values.
    *commandNO += 1;
    *isOpen = true;

    int retVal = 0; // Return value (success by default)

// ***************************** Creating customers and waiting for children to die ***************************** //

    for(int i = 1; i <= params.NZ; i++){ // Loop of forking until enough customers are present.
        pid_t pID = fork();
        if(pID == 0){
            customer(params.TZ, i);
        } else if(pID < 0){
            fprintf(stderr, "ERROR: While forking customers.\n");
            retVal = 1;
            goto forkErr;
        }
    }

    for(int i = 1; i <= params.NU; i++){ // Loop of forking until enough childs are present.
        pid_t pID = fork();
        if(pID == 0){
            worker(params.TU, i);
        } else if(pID < 0){
            fprintf(stderr, "ERROR: While forking workers.\n");
            retVal = 1;
            goto forkErr;
        }
    }

    // Waiting random time between <F/2,F>
    srand(time(NULL) * getpid());
    usleep(1000 * (rand() % ((params.F/2) + 1) + (params.F/2)));

    sem_wait(postOpen);
    sem_wait(fileAcc);
    printf("%d: closing\n", *commandNO);
    *commandNO += 1;
    *isOpen = false;
    sem_post(fileAcc);
    sem_post(postOpen);

forkErr:
    while(wait(NULL) > 0); // Waiting for children to die.

    release();
    fclose(fileP);

    return retVal;
}
