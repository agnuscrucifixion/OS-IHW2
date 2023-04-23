#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define MAX_PATIENTS 10
int* num_patients_seen;
sem_t* doctor1_sem;
sem_t* doctor2_sem;
sem_t* dentist_sem;
sem_t* surgeon_sem;
sem_t* therapist_sem;
sem_t* patient_sem;

void handle_sigint(int sig) {
    munmap(num_patients_seen, sizeof(int));
    sem_close(doctor1_sem);
    sem_close(doctor2_sem);
    sem_close(dentist_sem);
    sem_close(surgeon_sem);
    sem_close(therapist_sem);
    sem_close(patient_sem);
    sem_unlink("/doctor1_sem");
    sem_unlink("/doctor2_sem");
    sem_unlink("/dentist_sem");
    sem_unlink("/surgeon_sem");
    sem_unlink("/therapist_sem");
    sem_unlink("/patient_sem");
    exit(0);
}
void doctor1_process() {
    int num_patients = 0;
    while (num_patients < MAX_PATIENTS) {
        sem_wait(patient_sem);
        printf("Doctor 1 is seeing patient %d\n", num_patients+1);
        num_patients_seen[0]++;
        sem_post(doctor1_sem);
        sem_wait(patient_sem);
        int specialist = rand() % 3;
        printf("Doctor 1 referred patient %d to specialist %d\n", num_patients+1, specialist);
        sem_post(doctor1_sem);
        switch (specialist) {
            case 0:
                sem_post(dentist_sem);
                break;
            case 1:
                sem_post(surgeon_sem);
                break;
            case 2:
                sem_post(therapist_sem);
                break;
        }
        num_patients++;
    }
}
void doctor2_process() {
    int num_patients = 0;
    while (num_patients < MAX_PATIENTS) {
        sem_wait(patient_sem);
        printf("Doctor 2 is seeing patient %d\n", num_patients+1);
        num_patients_seen[0]++;
        sem_post(doctor2_sem);
        sem_wait(patient_sem);
        int specialist = rand() % 3;
        printf("Doctor 2 referred patient %d to specialist %d\n", num_patients+1, specialist);
        sem_post(doctor2_sem);
        switch (specialist) {
            case 0:
                sem_post(dentist_sem);
                break;
            case 1:sem_post(surgeon_sem);
                break;
            case 2:
                sem_post(therapist_sem);
                break;
        }
        num_patients++;
    }
}
void dentist_process() {
    int num_patients = 0;
    while (num_patients < MAX_PATIENTS) {
        sem_wait(dentist_sem);
        printf("Dentist is treating patient %d\n", num_patients_seen[0]);
        sem_post(patient_sem);
        num_patients++;
    }
}
void surgeon_process() {
    int num_patients = 0;
    while (num_patients < MAX_PATIENTS) {
        sem_wait(surgeon_sem);
        printf("Surgeon is treating patient %d\n", num_patients_seen[0]);
        sem_post(patient_sem);
        num_patients++;
    }
}
void therapist_process() {
    int num_patients = 0;
    while (num_patients < MAX_PATIENTS) {
        sem_wait(therapist_sem);
        printf("Therapist is treating patient %d\n", num_patients_seen[0]);
        sem_post(patient_sem);
        num_patients++;
    }
}
void patient_process() {
    sem_post(patient_sem);
    sem_wait(doctor1_sem);
    sem_post(patient_sem);
    sem_wait(doctor1_sem);
    sem_wait(patient_sem);
    printf("Patient is waiting to be treated by specialist\n");
    sem_post(patient_sem);
    sem_wait(patient_sem);
    printf("Patient has been treated\n");
}
int main() {
    int fd = shm_open("/num_patients_seen", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(int));
    num_patients_seen = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    num_patients_seen[0] = 0;
    doctor1_sem = sem_open("/doctor1_sem", O_CREAT | O_RDWR, 0666, 0);
    doctor2_sem = sem_open("/doctor2_sem", O_CREAT | O_RDWR, 0666, 0);
    dentist_sem = sem_open("/dentist_sem", O_CREAT | O_RDWR, 0666, 0);
    surgeon_sem = sem_open("/surgeon_sem", O_CREAT | O_RDWR, 0666, 0);
    therapist_sem = sem_open("/therapist_sem", O_CREAT | O_RDWR, 0666, 0);
    patient_sem = sem_open("/patient_sem", O_CREAT | O_RDWR, 0666, 0);
    signal(SIGINT, handle_sigint);
    pid_t pid;
    int i;
    for (i = 0; i < 2; i++) {
        pid = fork();
        if (pid < 0) {
            printf("Error in forking doctor process\n");
            exit(1);
        } else if (pid == 0) {
            if (i == 0) {
                doctor1_process();
            } else {
                doctor2_process();
            }
            exit(0);
        }
    }
    for (i = 0; i < 3; i++) {
        pid = fork();
        if (pid < 0) {
            printf("Error in forking specialist process\n");
            exit(1);
        } else if (pid == 0) {
            if (i == 0) {
                dentist_process();
            } else if (i == 1) {
                surgeon_process();
            } else {
                therapist_process();
            }
            exit(0);
        }
    }
    for (i = 0; i < MAX_PATIENTS; i++) {
        pid = fork();
        if (pid < 0) {
            printf("Error in forking patient process\n");
            exit(1);
        } else if (pid == 0) {
            patient_process();
            exit(0);
        }
    }
    for (i = 0; i < 2 + 3 + MAX_PATIENTS; i++) {
        wait(NULL);
    }
    handle_sigint(0);
    return 0;
}