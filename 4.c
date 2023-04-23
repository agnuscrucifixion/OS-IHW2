#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SHARED_MEMORY_NAME "/shared_memory"
#define DOCTOR_SEMAPHORE_NAME "/doctor_semaphore"
#define DENTIST_SEMAPHORE_NAME "/dentist_semaphore"
#define SURGEON_SEMAPHORE_NAME "/surgeon_semaphore"
#define THERAPIST_SEMAPHORE_NAME "/therapist_semaphore"

typedef struct {
    int patient_id;
    int doctor_id;
    int specialist_id;
} Patient;

void doctor_process(sem_t *doctor_semaphore, sem_t *dentist_semaphore, sem_t *surgeon_semaphore, sem_t *therapist_semaphore, Patient *shared_memory) {
    while(1) {
        sem_wait(doctor_semaphore);
        printf("Doctor %d is seeing patient %d\n", getpid(), shared_memory->patient_id);
        switch(shared_memory->specialist_id) {
            case 0:
                sem_post(dentist_semaphore);
                break;
            case 1:
                sem_post(surgeon_semaphore);
                break;
            case 2:
                sem_post(therapist_semaphore);
                break;
        }
    }
}
void dentist_process(sem_t *dentist_semaphore, sem_t *doctor_semaphore, Patient *shared_memory) {
    while(1) {
        sem_wait(dentist_semaphore);
        printf("Dentist is treating patient %d\n", shared_memory->patient_id);
        sem_post(doctor_semaphore);
    }
}
void surgeon_process(sem_t *surgeon_semaphore, sem_t *doctor_semaphore, Patient *shared_memory) {
    while(1) {
        sem_wait(surgeon_semaphore);
        printf("Surgeon is treating patient %d\n", shared_memory->patient_id);
        sem_post(doctor_semaphore);
    }
}

void therapist_process(sem_t *therapist_semaphore, sem_t *doctor_semaphore, Patient *shared_memory) {
    while(1) {
        sem_wait(therapist_semaphore);
        printf("Therapist is treating patient %d\n", shared_memory->patient_id);
        sem_post(doctor_semaphore);
    }
}

int main() {
    int shared_memory_descriptor = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shared_memory_descriptor, sizeof(Patient));
    Patient *shared_memory = mmap(NULL, sizeof(Patient), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_descriptor, 0);
    sem_t *doctor_semaphore = sem_open(DOCTOR_SEMAPHORE_NAME, O_CREAT | O_EXCL, 0666, 2);
    sem_t *dentist_semaphore = sem_open(DENTIST_SEMAPHORE_NAME, O_CREAT | O_EXCL, 0666, 0);
    sem_t *surgeon_semaphore = sem_open(SURGEON_SEMAPHORE_NAME, O_CREAT | O_EXCL, 0666, 0);
    sem_t *therapist_semaphore = sem_open(THERAPIST_SEMAPHORE_NAME, O_CREAT | O_EXCL, 0666, 0);
    pid_t doctor_pid = fork();
    if(doctor_pid == 0) {
        doctor_process(doctor_semaphore, dentist_semaphore, surgeon_semaphore, therapist_semaphore, shared_memory);
        exit(0);
    }
    pid_t dentist_pid = fork();
    if(dentist_pid == 0) {
        dentist_process(dentist_semaphore, doctor_semaphore, shared_memory);
        exit(0);
    }
    pid_t surgeon_pid = fork();
    if(surgeon_pid == 0) {
        surgeon_process(surgeon_semaphore, doctor_semaphore, shared_memory);
        exit(0);
    }
    pid_t therapist_pid = fork();
    if(therapist_pid == 0) {
        therapist_process(therapist_semaphore, doctor_semaphore, shared_memory);
        exit(0);
    }
    int patient_count = 1;
    while(1) {
        sleep(1);
        printf("Patient %d is arriving\n", patient_count);
        shared_memory->patient_id = patient_count;
        shared_memory->doctor_id = (patient_count % 2) + 1;
        shared_memory->specialist_id = patient_count % 3;
        sem_post(doctor_semaphore);
        patient_count++;
    }
    sem_unlink(DOCTOR_SEMAPHORE_NAME);
    sem_unlink(DENTIST_SEMAPHORE_NAME);
    sem_unlink(SURGEON_SEMAPHORE_NAME);
    sem_unlink(THERAPIST_SEMAPHORE_NAME);
    shm_unlink(SHARED_MEMORY_NAME);
    return 0;
}