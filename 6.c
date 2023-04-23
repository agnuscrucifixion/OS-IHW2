#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>

#define SHM_SIZE 1024

int semid, shmid;
char *shmaddr;

struct sembuf semwait = {0, -1, SEM_UNDO};
struct sembuf semsignal = {0, 1, SEM_UNDO};

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n signal received. Cleaning up\n");
        semctl(semid, 0, IPC_RMID);
        shmctl(shmid, IPC_RMID, NULL);
        exit(0);
    }
}

void doctor(int id) {
    int patient_count = 0;
    int *patient_queue = (int*)shmaddr;
    while (1) {
        printf("Doctor %d is ready to see patients.\n", id);
        while (patient_queue[id] == 0) {
            printf("Doctor %d is waiting for patients.\n", id);
            sleep(1);
        }
        printf("Doctor %d is seeing patient %d.\n", id, patient_queue[id]);
        sleep(rand() % 5 + 1);
        printf("Doctor %d is done seeing patient %d.\n", id, patient_queue[id]);
        patient_queue[id] = 0;
        patient_count++;
        if (patient_count == 10) {
            printf("Doctor %d has seen 10 patients today. Closing the clinic.\n", id);
            semctl(semid, 0, IPC_RMID);
            shmctl(shmid, IPC_RMID, NULL);
            exit(0);
        }
    }
}

void patient() {
    int specialist;
    int *patient_queue = (int*)shmaddr;
    int patient_id = getpid();
    printf("Patient %d has entered the clinic and joined the queue of Doctor %d.\n", patient_id, rand() % 2 + 1);
    while (1) {
        sleep(rand() % 5 + 1);
        specialist = rand() % 3 + 1;
        if (specialist == 1) {
            printf("Patient %d is seeing the dentist.\n", patient_id);
        } else if (specialist == 2) {
            printf("Patient %d is seeing the surgeon.\n", patient_id);
        } else {
            printf("Patient %d is seeing the therapist.\n", patient_id);
        }
        sleep(rand() % 5 + 1);
        printf("Patient %d is done with the specialist.\n", patient_id);
        semop(semid, &semwait, 1);
        if (patient_queue[0] == 0) {
            patient_queue[0] = patient_id;
        } else if (patient_queue[1] == 0) {
            patient_queue[1] = patient_id;
        } else {
            printf("Patient %d is leaving the clinic. All doctors are busy.\n", patient_id);
            semop(semid, &semsignal, 1);
            exit(0);
        }
        semop(semid, &semsignal, 1);
        printf("Patient %d has joined the queue of a doctor.\n", patient_id);
    }
}

int main() {
    signal(SIGINT, signal_handler);
    key_t semkey = ftok("semfile", 'a');
    semid = semget(semkey, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }
    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(1);
    }
    key_t shmkey = ftok("shmfile", 'a');
    shmid = shmget(shmkey, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (char*) -1) {
        perror("shmat");
        exit(1);
    }
    int *patient_queue = (int*)shmaddr;
    patient_queue[0] = 0;
    patient_queue[1] = 0;
    int i;
    for (i = 0; i < 10; i++) {
        if (fork() == 0) {
            patient();
            exit(0);
        }
    }
    if (fork() == 0) {
        doctor(1);
        exit(0);
    }
    if (fork() == 0) {
        doctor(2);
        exit(0);
    }
    while (1);
    return 0;
}