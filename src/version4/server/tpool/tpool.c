#include "tpool.h"
#include <stdlib.h>
#include <stdio.h>

// 내부 함수 선언
static void *tpool_worker(void *arg);
static tpool_work_t *tpool_work_create(void (*function)(void *), void *arg);
static void tpool_work_destroy(tpool_work_t *work);

// 스레드 풀을 생성
tpool *tpool_create(size_t num_threads) {
    if (num_threads == 0) return NULL;

    // 스레드 풀 구조체 메모리 할당 및 초기화
    tpool *pool = calloc(1, sizeof(tpool));
    if (!pool) return NULL;

    // 동기화를 위한 뮤텍스와 조건변수 초기화
    pthread_mutex_init(&pool->work_mutex, NULL);
    pthread_cond_init(&pool->work_cond, NULL);
    pthread_cond_init(&pool->working_cond, NULL);

    pool->thread_cnt = num_threads;

    // 요청된 개수만큼 worker 스레드 생성
    for (size_t i = 0; i < num_threads; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, tpool_worker, pool) != 0) {
            perror("pthread_create");
            tpool_destroy(pool);
            return NULL;
        }
        // 스레드를 detach 상태로 설정 (자동 리소스 정리)
        pthread_detach(thread);
    }

    return pool;
}


// 스레드 풀에 새로운 작업을 추가
bool tpool_add_work(tpool *pool, void (*function)(void *), void *arg) {
    if (!pool || !function) return false;

    // 새로운 작업 구조체 생성
    tpool_work_t *work = tpool_work_create(function, arg);
    if (!work) return false;

    // 작업 큐에 추가하기 위해 뮤텍스 락 획득
    pthread_mutex_lock(&pool->work_mutex);

    // 작업 큐에 새 작업 추가
    if (!pool->work_first) {
        // 큐가 비어있는 경우
        pool->work_first = work;
        pool->work_last = work;
    } else {
        // 큐의 마지막에 작업 추가
        pool->work_last->next = work;
        pool->work_last = work;
    }

    // 대기 중인 워커 스레드 하나를 깨움
    pthread_cond_signal(&pool->work_cond);
    pthread_mutex_unlock(&pool->work_mutex);
    return true;
}


// 모든 작업이 완료될 때까지 대기
void tpool_wait(tpool *pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->work_mutex);
    // 작업 중인 스레드가 있거나 대기 중인 작업이 있는 경우 대기
    while (pool->working_cnt > 0 || pool->work_first) {
        pthread_cond_wait(&pool->working_cond, &pool->work_mutex);
    }
    pthread_mutex_unlock(&pool->work_mutex);
}


// 스레드 풀을 제거하고 리소스를 정리
void tpool_destroy(tpool *pool) {
    if (!pool) return;

    // 스레드 풀 종료 신호 설정
    pthread_mutex_lock(&pool->work_mutex);
    pool->stop = true;
    pthread_cond_broadcast(&pool->work_cond);
    pthread_mutex_unlock(&pool->work_mutex);

    // 모든 작업 완료 대기
    tpool_wait(pool);

    // 남은 작업 큐 정리
    while (pool->work_first) {
        tpool_work_t *work = pool->work_first;
        pool->work_first = work->next;
        tpool_work_destroy(work);
    }

    // 동기화 객체 제거
    pthread_mutex_destroy(&pool->work_mutex);
    pthread_cond_destroy(&pool->work_cond);
    pthread_cond_destroy(&pool->working_cond);

    // 풀 메모리 해제
    free(pool);
}


// 새로운 작업 구조체를 생성
static tpool_work_t *tpool_work_create(void (*function)(void *), void *arg) {
    tpool_work_t *work = malloc(sizeof(tpool_work_t));
    if (!work) return NULL;
    work->function = function;
    work->arg = arg;
    work->next = NULL;
    return work;
}

// 작업 구조체를 제거
static void tpool_work_destroy(tpool_work_t *work) {
    if (work) free(work);
}

// 워커 스레드가 실행
static void *tpool_worker(void *arg) {
    tpool *pool = (tpool *)arg;

     while (1) {
        pthread_mutex_lock(&pool->work_mutex);

        // 작업이 없고 종료 신호도 없는 경우 대기
        while (!pool->work_first && !pool->stop) {
            pthread_cond_wait(&pool->work_cond, &pool->work_mutex);
        }

        // 종료 신호가 있고 작업이 없는 경우 스레드 종료
        if (pool->stop && !pool->work_first) {
            pthread_mutex_unlock(&pool->work_mutex);
            break;
        }

        // 작업 가져오기
        tpool_work_t *work = pool->work_first;
        if (work) {
            pool->work_first = work->next;
            if (!pool->work_first) {
                pool->work_last = NULL;
            }
            pool->working_cnt++;
        }

        pthread_mutex_unlock(&pool->work_mutex);

        // 작업 실행
        if (work) {
            work->function(work->arg);
            tpool_work_destroy(work);
        }

        // 작업 완료 처리
        pthread_mutex_lock(&pool->work_mutex);
        pool->working_cnt--;
        if (!pool->working_cnt && !pool->work_first) {
            pthread_cond_signal(&pool->working_cond);
        }
        pthread_mutex_unlock(&pool->work_mutex);
    }

    return NULL;
}