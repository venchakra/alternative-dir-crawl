#include "WorkQueue.h"
#include <cstddef>
#include <cassert>

WorkQueue::WorkQueue(int numWorkers, FileCount* pFc) : _numWorkers(numWorkers) {
    _pWorkerInfoArr = new ThreadInfo[numWorkers];
    _pFc = pFc;
    assert(NULL != _pWorkerInfoArr);

    _allDone = false;
    (void)pthread_mutex_init(&_mtx, NULL);
    (void)pthread_cond_init(&_cv, NULL);

    initWorkers();
}

WorkQueue::~WorkQueue() {
    assert(_q.empty() == true);
    (void)pthread_mutex_destroy(&_mtx);
    (void)pthread_cond_destroy(&_cv);
    delete [] _pWorkerInfoArr;
}

void
WorkQueue::initWorkers() {
    for (int i = 0; i < _numWorkers; i++) {
        _pWorkerInfoArr[i].tid = i;
        _pWorkerInfoArr[i].pQ = this;
        _pWorkerInfoArr[i].state = INIT;
        printf("Creating Worker [%d]\n", i);
        int ret = pthread_create(&(_pWorkerInfoArr[i].ptid),
                                 NULL,
                                 WorkQueue::TEF,
                                 &_pWorkerInfoArr[i]);
        if (0 != ret) {
            printf("Failed to create thread: %d\n", i);
            exit(-1);
        }
    }
}

void
WorkQueue::waitForWorkers() {
    printf("Waiting for the threads to complete\n");
    for (int i = 0; i < _numWorkers; i++) {
        pthread_join(_pWorkerInfoArr[i].ptid, NULL);
    }
}

void
WorkQueue::enqueue(JobPtr job, int tid) {
    printf("Thread[%d]: enqueuing work\n", tid);
    pthread_mutex_lock(&_mtx);
    bool empty = _q.empty();
    _q.push(job);
    if (empty) {
        pthread_cond_signal(&_cv);
    }
    pthread_mutex_unlock(&_mtx);
}

JobPtr
WorkQueue::dequeue(int tid) {
    printf("Thread[%d]: attempting to dequeue work\n", tid);

    pthread_mutex_lock(&_mtx);

    if (_allDone) {
        printf("Thread[%d]: All work is done, no need to dequeue\n", tid);
        pthread_mutex_unlock(&_mtx);
        return nullptr;
    }

    if (_q.empty()) {
        while (true) {
            printf("Thread[%d]: going to sleep\n", tid);
            pthread_cond_wait(&_cv, &_mtx);
            if (!_q.empty() || _allDone) {
                break;
            }
        }
    }

    if (_allDone) {
        assert(_q.empty() == true);
        printf("Thread[%d]: All work is done and queue is empty\n", tid);
        pthread_mutex_unlock(&_mtx);
        return nullptr;
    }

    printf("Thread[%d]: got job to work on\n", tid);
    JobPtr job = _q.front();
    _q.pop();
    pthread_mutex_unlock(&_mtx);
    return job;
}

void
WorkQueue::setRunning(int tid) {
    pthread_mutex_lock(&_mtx);
    printf("Thread[%d]: Setting state to RUNNING\n", tid);
    _pWorkerInfoArr[tid].state = RUNNING;
    pthread_mutex_unlock(&_mtx);
}

void
WorkQueue::setDone(int tid) {
    pthread_mutex_lock(&_mtx);
    printf("Thread[%d]: Setting state to DONE\n", tid);
    _pWorkerInfoArr[tid].state = DONE;
    bool allDone = true;
    for (int i = 0; i < _numWorkers; i++) {
        if (_pWorkerInfoArr[i].state == RUNNING) {
            allDone = false;
            break;
        }
    }

    if (allDone && _q.empty()) {
        _allDone = true;
        pthread_cond_signal(&_cv); // Signal any sleeping threads
    }

    pthread_mutex_unlock(&_mtx);    
}

void
WorkQueue::incFc() {
    _pFc->incFileCount();
}

void*
WorkQueue::TEF(void* args) {
    ThreadInfo* tinfo = (ThreadInfo*)args;
    WorkQueue* pQ = tinfo->pQ;

    printf("Thread[%d]: starting\n", tinfo->tid);

    while (!(pQ->getAllDone())) {
        JobPtr job = pQ->dequeue(tinfo->tid);
        if (nullptr != job) {
            pQ->setRunning(tinfo->tid);
            (void)job->run(tinfo); // Not doing wait/done here
        }
        pQ->setDone(tinfo->tid);
    }

    printf("Thread[%d]: exiting\n", tinfo->tid);

    return NULL;
}
