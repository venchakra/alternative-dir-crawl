#ifndef _WORKQ_H_
#define _WORKQ_H_

#include <queue>
#include <memory>
#include <pthread.h>
#include <cstdio>

class Job {
 public:
        Job() {}
        ~Job() {}

        virtual int run(void* arg = NULL) = 0;
        virtual int jobType() = 0;

        /* int wait() { */
        /*         pthread_mutex_lock(&_mtx); */
        /*         pthread_cond_wait(&_cv, &_mtx); */
        /*         pthread_mutex_unlock(&_mtx); */
        /*         return _ret; */
        /* } */

        /* void done(int ret) { */
        /*         _ret = ret; */
        /*         pthread_mutex_lock(&_mtx); */
        /*         pthread_cond_signal(&_cv); */
        /*         pthread_mutex_unlock(&_mtx); */
        /* } */

 private:
        /* int _ret; */
        pthread_mutex_t _mtx;
        pthread_cond_t _cv;
};

typedef std::shared_ptr<Job> JobPtr;

class DirCrawlJob : public Job {
public:
    DirCrawlJob(char* path) : _dirPath(path) {}
    ~DirCrawlJob() { free(_dirPath); }
    int run(void* args);
    int jobType();

private:
    char* _dirPath;
};

class FileCount {
public:
    FileCount() {
        _fileCount = 0;
        pthread_mutex_init(&_mtx, NULL);
    }

    ~FileCount() {
        pthread_mutex_destroy(&_mtx);
    }

    void incFileCount() {
        pthread_mutex_lock(&_mtx);
        ++_fileCount;
        pthread_mutex_unlock(&_mtx);
    }

    int getFileCount() {
        int fc = 0;
        pthread_mutex_lock(&_mtx);
        fc = _fileCount;
        pthread_mutex_unlock(&_mtx);
        return fc;
    }

private:
    pthread_mutex_t _mtx;
    int _fileCount;
};

enum ThreadState {
        INIT=0,
        RUNNING=1,
        DONE=2
};

class WorkQueue;

typedef struct ThreadInfo {
        pthread_t ptid;
        int tid;
        WorkQueue* pQ;
        ThreadState state;
} ThreadInfo;

class WorkQueue {
        typedef void*(*TEFPtr)(void*);
 public:
        WorkQueue(int numWorkers, FileCount* pFc);
        ~WorkQueue();

        void initWorkers();
        void waitForWorkers();
        void enqueue(JobPtr job, int tid);
        JobPtr dequeue(int tid);
        void setRunning(int tid);
        void setDone(int tid);
        void incFc();
        bool getAllDone() {return _allDone;}

 private:
        static void* TEF(void* args);

        int _numWorkers;
        struct ThreadInfo* _pWorkerInfoArr;
        std::queue<JobPtr> _q;
        pthread_mutex_t _mtx;
        pthread_cond_t _cv;
        bool _allDone;
        FileCount* _pFc;
};

#endif // _WORKQ_H_
