//
// Created by zy on 22-6-25.
//
#include <pthread.h>
#include <stdio.h>


typedef struct sem_t_ {
    int count;
    pthread_cond_t cond;
    pthread_mutex_t mtx;
} sem_t;

int loops = 1000;
sem_t empty;
sem_t full;
sem_t mutex;


const int MAX = 10;
int buffer[MAX];
int fill = 0;
int use = 0;

void put(int value) {
    buffer[fill] = value;
    fill = (fill+1) % MAX;
}

int get() {
    int tmp = buffer[use];
    use = (use+1) % MAX;
    return tmp;
}

void* sem_init(sem_t* s, int a, int count) {
    s->count=count;
    // 锁和条件变量也要初始化
    pthread_mutex_init(&s->mtx, NULL);
    pthread_cond_init(&s->cond, NULL);
    return NULL;
}

void sem_wait(sem_t* s) {
    pthread_mutex_lock(&s->mtx);
    while (s->count <= 0) {
        pthread_cond_wait(&s->cond, &s->mtx);
    }
    s->count--;
    pthread_mutex_unlock(&s->mtx);
}

void sem_post(sem_t* s) {
    pthread_mutex_lock(&s->mtx);
    s->count++;
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->mtx);
}

void* producer(void* arg) {
    int i;
    for (i=0; i<loops; i++) {
        sem_wait(&empty);
        sem_wait(&mutex);
        put(i);
        sem_post(&mutex);
        sem_post(&full);
    }
    return NULL;
}

void* consumer(void* arg) {
    int i;
    for (i=0; i<loops; i++) {
        sem_wait(&full);
        sem_wait(&mutex);
        int tmp = get();
        sem_post(&mutex);
        sem_post(&empty);
        printf("%d\n", tmp);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    sem_init(&empty, 0, MAX);
    sem_init(&full, 0, 0);
    sem_init(&mutex, 0, 0);
}
