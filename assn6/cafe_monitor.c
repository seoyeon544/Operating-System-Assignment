#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define M 3
#define N 5
#define K 2
#define TOTAL_CARS 15

void print_time() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
}

// ================= Monitor =================

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t car_arrived;
    pthread_cond_t taker_ready;

    int waiting_cars;
    int queue[M];
    int in, out;
} Lane;

Lane lane;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;

    int count;
    int queue[N];
    int in, out;
} Rail;

Rail rail;

void* car_thread(void* arg) {
    int id = *(int*)arg;

    pthread_mutex_lock(&lane.mutex);

    if (lane.waiting_cars >= M) {
        print_time();
        printf("Car %d: Waiting line is too long. Drive away.\n", id);
        pthread_mutex_unlock(&lane.mutex);
        return NULL;
    }

    lane.queue[lane.in] = id;
    lane.in = (lane.in + 1) % M;
    lane.waiting_cars++;

    print_time();
    printf("Car %d enters -> waiting in the lane.\n", id);

    pthread_cond_signal(&lane.car_arrived);

    pthread_cond_wait(&lane.taker_ready, &lane.mutex);

    pthread_mutex_unlock(&lane.mutex);
    return NULL;
}

void* taker_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&lane.mutex);

        while (lane.waiting_cars == 0) {
            pthread_cond_wait(&lane.car_arrived, &lane.mutex);
        }

        int car_id = lane.queue[lane.out];
        lane.out = (lane.out + 1) % M;
        lane.waiting_cars--;

        pthread_cond_signal(&lane.taker_ready);

        pthread_mutex_unlock(&lane.mutex);

        sleep(rand() % 2 + 1);

        pthread_mutex_lock(&rail.mutex);

        while (rail.count >= N) {
            pthread_cond_wait(&rail.not_full, &rail.mutex);
        }

        rail.queue[rail.in] = car_id;
        rail.in = (rail.in + 1) % N;
        rail.count++;

        print_time();
        printf("Order taker: Put Car %d's order on the rail.\n", car_id);

        pthread_cond_signal(&rail.not_empty);

        pthread_mutex_unlock(&rail.mutex);
    }
    return NULL;
}

void* barista_thread(void* arg) {
    int id = *(int*)arg;

    while (1) {
        pthread_mutex_lock(&rail.mutex);

        while (rail.count == 0) {
            pthread_cond_wait(&rail.not_empty, &rail.mutex);
        }

        int car_id = rail.queue[rail.out];
        rail.out = (rail.out + 1) % N;
        rail.count--;


        pthread_cond_signal(&rail.not_full);

        pthread_mutex_unlock(&rail.mutex);

        sleep(rand() % 3 + 3);

        print_time();
        printf("Barista %d: Car %d's Iced Americano is complete!\n",
               id, car_id);
    }
    return NULL;
}

// ================= main =================
int main() {
    srand(time(NULL));

    pthread_mutex_init(&lane.mutex, NULL);
    pthread_cond_init(&lane.car_arrived, NULL);
    pthread_cond_init(&lane.taker_ready, NULL);
    lane.waiting_cars = 0;
    lane.in = lane.out = 0;

    pthread_mutex_init(&rail.mutex, NULL);
    pthread_cond_init(&rail.not_full, NULL);
    pthread_cond_init(&rail.not_empty, NULL);
    rail.count = 0;
    rail.in = rail.out = 0;

    pthread_t taker;
    pthread_t baristas[K];
    pthread_t cars[TOTAL_CARS];

    int barista_ids[K];
    int car_ids[TOTAL_CARS];

    pthread_create(&taker, NULL, taker_thread, NULL);

    for (int i = 0; i < K; i++) {
        barista_ids[i] = i + 1;
        pthread_create(&baristas[i], NULL, barista_thread, &barista_ids[i]);
    }

    for (int i = 0; i < TOTAL_CARS; i++) {
        car_ids[i] = i + 1;
        pthread_create(&cars[i], NULL, car_thread, &car_ids[i]);
        usleep((rand() % 1500 + 500) * 1000);
    }

    for (int i = 0; i < TOTAL_CARS; i++) {
        pthread_join(cars[i], NULL);
    }

    sleep(10);
    print_time();
    printf("Simulation finished.\n");

    return 0;
}