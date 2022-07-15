//
// Created by zy on 22-6-24.
//
#include <pthread.h>
#include <stdio.h>


int done=0;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  // 用于保护done
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


void thr_exit() {
    pthread_mutex_lock(&mtx);
    done = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mtx);
}

void *child(void* arg) {
    printf("child process\n");
    thr_exit();
    return NULL;
}


void thr_join() {
    pthread_mutex_lock(&mtx);
    while (done == 0) {  // 这里用done是为了保证子进程如果完成的早（早早置done为1），我就不wait了；无条件wait会使得子进程结束的早的情况下（signal没有唤醒任何进程，因为没有进程等待），永远wait。
        pthread_cond_wait(&cond, &mtx);
    }
    pthread_mutex_unlock(&mtx);
}

int main(int argc, char* argv[]) {
    pthread_t p;
    printf("father thread: begin\n");
    pthread_create(&p, NULL, child, NULL);
    thr_join();
    printf("father thread:end\n");
    return 0;
}
