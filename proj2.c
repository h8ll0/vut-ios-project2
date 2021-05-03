/*
IOS     Project 2       Multiprocess program with semafores     Autor: Koval Maksym (xkoval20)
*/

#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/shm.h>

#define SharedMemoryKey 4242

//shared memory struct
typedef struct{

    int var;
    sem_t sem;
    int shmid;

} shm_t;

//function for creating ang using elves processes
int
processElfes(int NE, int TE, FILE *fp, shm_t *messageSem, shm_t *elfesSem, shm_t *santaHelpingElfesSem,
             shm_t *elfesInWorkshop, int *error);

//function for creating ang using deers processes
int processDeers(int NR, int TR, FILE *fp, shm_t *messageSem, shm_t *deersSem, shm_t *santaHitchingSem,
                 shm_t *santaChristmasStartSem, int *error);

//function for creating ang using Santa process
int
processSanta(int NE, int NR, FILE *fp, shm_t *messageSem, shm_t *santaHitchingSem, shm_t *elfesSem, shm_t *deersSem,
             shm_t *santaHelpingElfesSem, shm_t *santaChristmasStartSem, shm_t *elfesInWorkshop, int *error);

//returns random time in interval <min_time, max_time>
int random_time(int min_time, int max_time);

//prints message of creature
void print_msg(FILE *fp, shm_t *messageSem, int creature_code, int msg_code, int i);

//prints error message
void print_err_msg(int err_code);

//deinitialize for single shared memory instance
void deinitialize(int i, shm_t *shm);

//initialize for single shared memory instance
shm_t *init_memory(int key, int *error);

//checks user arguments
int check_args(int *NE, int *NR, int *TE, int *TR, int argc, char **argv);

//deinitialize all shared memory instances in array **semafores
void deinitialize_all(int error, shm_t *messageSem, shm_t *elvesSem, shm_t *deersSem, shm_t *elfesInWorkshop,
                      shm_t *santaHelpingElvesSem, shm_t *santaHitchingSem, shm_t *santaChristmasStartSem);


enum Creature {

    SANTA,
    ELF,
    DEER

};


enum MessageType {

    STARTED,
    NEED_HELP,
    GET_HELP,
    TAKING_HOLIDAYS,
    RSTARTED,
    RETURN_HOME,
    GET_HITCHED,
    GOING_SLEEP,
    HELPING_ELVES,
    CLOSING_WORKSHOP,
    CHRISTMAS_STARTED

};


enum Errors {

    SHM_GET_ERR = 1,
    SHM_MAT_ERR,
    FORK_ERR,
    WRONG_ARGS,
    FILE_ERR

};


int main(int argc, char *argv[]) {

    FILE *log;
    int error = 0;      //var for checking error
    int NE = 0;
    int NR = 0;
    int TE = 0;
    int TR = 0;

    log = fopen ("proj2.out", "w");
    if (log == NULL) {
        print_err_msg(FILE_ERR);
        return 1;
    }

    setbuf(stdout,NULL);
    setbuf(stderr,NULL);


    error = check_args(&NE, &NR, &TE, &TR, argc, argv);
    if (error > 0) {
        print_err_msg( WRONG_ARGS);
        return 1;
    }

    //creating shared memory, initializing semafores and variables
    shm_t *messageSem = init_memory(SharedMemoryKey, &error);
    sem_init(&messageSem->sem, 1, 1);
    messageSem->var = 1;                           //number of action
    if (error > 0) {                               //checking for error
        deinitialize_all(error, messageSem, NULL, NULL, NULL, NULL, NULL,
                         NULL);
        return 1;
    }

    shm_t *elvesSem = init_memory(SharedMemoryKey + 1, &error);
    sem_init(&elvesSem->sem, 1, 0);
    elvesSem->var = 0;                              //holiday (if 1 == elves go on holidays)
    if (error > 0) {
        deinitialize_all(error, messageSem, elvesSem, NULL, NULL, NULL, NULL,
                         NULL);
        return 1;
    }

    shm_t *elfesInWorkshop = init_memory(SharedMemoryKey + 3, &error);
    sem_init(&elfesInWorkshop->sem, 1, 0);
    elfesInWorkshop->var = 0;                       //how many elves in workshop at the moment
    if (error > 0) {
        deinitialize_all(error, messageSem, elvesSem, NULL, elfesInWorkshop, NULL, NULL,
                         NULL);
        return 1;
    }

    shm_t *santaHelpingElvesSem = init_memory(SharedMemoryKey + 4, &error);
    sem_init(&santaHelpingElvesSem->sem, 1, 0);
    santaHelpingElvesSem->var = 0;                  //how many elves in need help at the moment
    if (error > 0) {
        deinitialize_all(error, messageSem, elvesSem, NULL, elfesInWorkshop, NULL, NULL,
                         NULL);
        return 1;
    }

    shm_t *deersSem = init_memory(SharedMemoryKey + 5, &error);
    sem_init(&deersSem->sem, 1, 0);
    deersSem->var = 0;                              //how many deers are at home at the moment
    if (error > 0) {
        deinitialize_all(error, messageSem, elvesSem, deersSem, elfesInWorkshop, NULL, NULL,
                         NULL);
        return 1;
    }

    shm_t *santaHitchingSem = init_memory(SharedMemoryKey + 6, &error);
    sem_init(&santaHitchingSem->sem, 1, 0);
    santaHitchingSem->var = 0;                      //how many deers are hitched
    if (error > 0) {
        deinitialize_all(error, messageSem, elvesSem, deersSem, elfesInWorkshop, santaHelpingElvesSem, santaHitchingSem,
                         NULL);
        return 1;
    }

    shm_t *santaChristmasStartSem = init_memory(SharedMemoryKey + 7, &error);
    sem_init(&santaChristmasStartSem->sem, 1, 0);
    if (error > 0) {
        deinitialize_all(error, messageSem, elvesSem, deersSem, elfesInWorkshop, santaHelpingElvesSem, santaHitchingSem,
                         santaChristmasStartSem);
        return 1;
    }
    //end of section

    //creating santa, NE processes of elves, NR processes of deers
    int elvesPID = processElfes(NE, TE, log, messageSem, elvesSem, santaHelpingElvesSem, elfesInWorkshop, &error);
    int deersPID = processDeers(NR, TR, log, messageSem, deersSem, santaHitchingSem, santaChristmasStartSem, &error);
    int santaPID = processSanta(NE, NR, log, messageSem, santaHitchingSem, elvesSem, deersSem, santaHelpingElvesSem,
                                santaChristmasStartSem, elfesInWorkshop, &error);
    if (error > 0) {
        deinitialize_all(error, messageSem, elvesSem, deersSem, elfesInWorkshop, santaHelpingElvesSem, santaHitchingSem,
                         santaChristmasStartSem);
        return 1;
    }

    //waiting till all processes are done
    waitpid(elvesPID, NULL, 0);
    waitpid(deersPID, NULL, 0);
    waitpid(santaPID, NULL, 0);


    //clearing shared memory
    deinitialize_all(0, messageSem, elvesSem, deersSem, elfesInWorkshop, santaHelpingElvesSem, santaHitchingSem,
                     santaChristmasStartSem);

    //closing .out file
    fclose(log);
    return 0;
}


//deinitialize all shared memory instances in array **semafores
void deinitialize_all(int error, shm_t *messageSem, shm_t *elvesSem, shm_t *deersSem, shm_t *elfesInWorkshop,
                      shm_t *santaHelpingElvesSem, shm_t *santaHitchingSem, shm_t *santaChristmasStartSem) {

    deinitialize(error, messageSem);
    deinitialize(error, elvesSem);
    deinitialize(error, deersSem);
    deinitialize(error, elfesInWorkshop);
    deinitialize(error, santaHelpingElvesSem);
    deinitialize(error, santaHitchingSem);
    deinitialize(error, santaChristmasStartSem);

}


//checks user arguments
int check_args(int *NE, int *NR, int *TE, int *TR, int argc, char **argv) {

    if (argc != 5) {
        return WRONG_ARGS;
    }

    *NE = atoi(argv[1]);
    *NR = atoi(argv[2]);
    *TE = atoi(argv[3]);
    *TR = atoi(argv[4]);

    if ((*NE <= 0) || (*NE >= 1000) \
    || (*NR <= 0 ) || (*NR >= 20) \
    || (*TE < 0 ) || (*TE > 1000) \
    || (*TR < 0 ) || (*TR > 1000)) {
        return WRONG_ARGS;
    }
    return 0;
}


//initialize for single shared memory instance
shm_t *init_memory(int key, int *error) {

    int shmid = shmget(key, sizeof(shm_t), IPC_CREAT | 0666);
    if(shmid < 0) {
        *error = SHM_GET_ERR;
        return (shm_t *) SHM_GET_ERR;
    }
    shm_t *shm = shmat(shmid, NULL, 0);
    if(shm == (shm_t *) -1) {
        *error = SHM_MAT_ERR;
        return (shm_t*) SHM_MAT_ERR;
    }
    shm->shmid = shmid;

    return shm;

}


//deinitialize for single shared memory instance
void deinitialize(int i, shm_t *shm) {

    if (i > 0) {
        print_err_msg(i);
    }

    if (shm != NULL) {

        sem_destroy(&shm->sem);

        //remove the shared memory
        int shmid = shm->shmid;
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);

    }

    return;
}


//function for creating ang using Santa process
int
processSanta(int NE, int NR, FILE *fp, shm_t *messageSem, shm_t *santaHitchingSem, shm_t *elfesSem, shm_t *deersSem,
             shm_t *santaHelpingElfesSem, shm_t *santaChristmasStartSem, shm_t *elfesInWorkshop, int *error) {

    int santaPID = fork();
    if (santaPID == 0) {


        sem_post(&elfesSem->sem);
        sem_post(&deersSem->sem);

        print_msg(fp, messageSem, SANTA, GOING_SLEEP, 0);

        while (1) {
            if  (deersSem->var == NR) {             //checking if all deers at home

                print_msg(fp, messageSem, SANTA, CLOSING_WORKSHOP, 0);

                sem_post(&santaHitchingSem->sem);
                elfesSem->var = 1;  //holidays
                for (int i = 0; i < NE; ++i) {
                    sem_post(&santaHelpingElfesSem->sem);
                }
                break;
            }

            if  (santaHelpingElfesSem->var >= 3) {      //checking if some elves need help

                print_msg(fp, messageSem, SANTA, HELPING_ELVES, 0);
                santaHelpingElfesSem->var -= 3;
                sem_post(&santaHelpingElfesSem->sem);
                sem_post(&santaHelpingElfesSem->sem);
                sem_post(&santaHelpingElfesSem->sem);

                sem_wait(&elfesInWorkshop->sem);
                sem_wait(&elfesInWorkshop->sem);
                sem_wait(&elfesInWorkshop->sem);
                print_msg(fp, messageSem, SANTA, GOING_SLEEP, 0);

            }


        }

            sem_wait(&santaChristmasStartSem->sem);
            print_msg(fp, messageSem, SANTA, CHRISTMAS_STARTED, 0);
            sem_post(&santaChristmasStartSem->sem);

            exit(0);

        }else if (santaPID > 0) {

        }else {
            *error = FORK_ERR;
        }

    return santaPID;
}


//function for creating ang using elves processes
int
processElfes(int NE, int TE, FILE *fp, shm_t *messageSem, shm_t *elfesSem, shm_t *santaHelpingElfesSem,
             shm_t *elfesInWorkshop, int *error) {

    int elfesPID = fork();
    if (elfesPID == 0) {

        sem_wait(&elfesSem->sem);

        for (int i=0; i < NE; i++) {

            if(fork() == 0){

                print_msg(fp, messageSem, ELF, STARTED, i + 1);

                while (1) {

                    usleep(random_time(0,TE)*1000);
                    print_msg(fp, messageSem, ELF, NEED_HELP, i + 1);
                    santaHelpingElfesSem->var++;

                    sem_wait(&santaHelpingElfesSem->sem);

                    if (elfesSem->var) {

                        print_msg(fp, messageSem, ELF, TAKING_HOLIDAYS, i + 1);
                        break;

                    } else {

                        print_msg(fp, messageSem, ELF, GET_HELP, i + 1);
                        sem_post(&elfesInWorkshop->sem);

                    }

                }

                exit(0);

            }

        }

        sem_post(&elfesSem->sem);

        for (int i = 0; i < NE; ++i) {
            wait(NULL);
        }

        exit(0);

    }else if (elfesPID > 0) {    //parent


    }else {
        *error = FORK_ERR;
    }

    return elfesPID;
}


//function for creating ang using deers processes
int processDeers(int NR, int TR, FILE *fp, shm_t *messageSem, shm_t *deersSem, shm_t *santaHitchingSem,
                 shm_t *santaChristmasStartSem, int *error) {

    int deersPID = fork();
    if (deersPID == 0) {

        sem_wait(&deersSem->sem);
        for (int i = 0; i < NR; i++) {

            if(fork() == 0){

                print_msg(fp, messageSem, DEER, RSTARTED, i + 1);
                usleep(random_time(TR/2,TR)*1000);
                print_msg(fp, messageSem, DEER, RETURN_HOME, i + 1);
                deersSem->var++;

                sem_wait(&santaHitchingSem->sem);

                print_msg(fp, messageSem, DEER, GET_HITCHED, i + 1);
                santaHitchingSem->var++;
                sem_post(&santaHitchingSem->sem);

                if (santaHitchingSem->var == NR) {
                    sem_post(&santaChristmasStartSem->sem);
                }

                exit(0);
            }

        }
        sem_post(&deersSem->sem);
        for (int j = 0; j < NR; ++j) {
            wait(NULL);
        }

        exit(0);

    }else if (deersPID > 0) {


    }else {
        *error = FORK_ERR;
    }

    return deersPID;
}


//returns random time in interval <min_time, max_time>
int random_time(int min_time, int max_time) {

    srand(getpid());
    int time = (rand() % (max_time - min_time + 1)) + min_time;
    return time;

}

//prints message of creature
void print_msg(FILE *fp, shm_t *messageSem, int creature_code, int msg_code, int i) {

    char *creature[] = {
            "Santa",
            "Elf",
            "RD"
    };
    char *msg_type[] = {
            "started",
            "need help",
            "get help",
            "taking holidays",
            "rstarted",
            "return home",
            "get hitched",
            "going to sleep",
            "helping elves",
            "closing workshop",
            "Christmas started"

    };

    sem_wait(&messageSem->sem);
    switch (creature_code) {

        case SANTA:
            fprintf(fp, "%i: %s: %s\n", messageSem->var++, creature[creature_code], msg_type[msg_code]);
            fflush(fp);
            break;
        default:
            fprintf(fp, "%i: %s %i: %s\n", messageSem->var++, creature[creature_code], i, msg_type[msg_code]);
            fflush(fp);


    }
    sem_post(&messageSem->sem);

    return;
}

//prints error message
void print_err_msg(int err_code) {

    char *err_type[] = {
            "can't create shared memory (shm_get error)",
            "can't create shared memory (shm_mat error)",
            "can't fork",
            "wrong args",
            "can't create file"
    };
    fprintf(stderr,"proj2: %s\n",err_type[err_code-1]);
    return;
}
