#ifndef TPOOL_H
#define TPOOL_H

#include <pthread.h>
#include <stdbool.h>

// 작업 구조체 정의 (연결 리스트 방식)
typedef struct tpool_work {
    void (*function)(void *arg);          // 작업 함수
    void *arg;                            // 작업 함수에 전달할 인자
    struct tpool_work *next;              // 다음 작업
} tpool_work_t;

// 스레드 풀 구조체 정의
typedef struct tpool {
    tpool_work_t    *work_first;          // 작업 큐의 첫 번째 작업
    tpool_work_t    *work_last;           // 작업 큐의 마지막 작업
    pthread_mutex_t  work_mutex;          // 작업 큐 접근을 위한 뮤텍스
    pthread_cond_t   work_cond;           // 작업 대기를 위한 조건 변수
    pthread_cond_t   working_cond;        // 작업 상태 조건 변수
    size_t           working_cnt;         // 현재 작업 중인 스레드 수
    size_t           thread_cnt;          // 전체 스레드 수
    bool             stop;                // 스레드 풀 종료 플래그
} tpool;

// 스레드 풀 생성
tpool *tpool_create(size_t thread_cnt);

// 작업 추가
bool tpool_add_work(tpool *pool, void (*function)(void *), void *arg);

// 스레드 풀 삭제 (모든 작업 완료 후)
void tpool_destroy(tpool *pool);

// 대기 (모든 작업 완료 시까지)
void tpool_wait(tpool *pool);

#endif
