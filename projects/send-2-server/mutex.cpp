#include<sys/time.h>
#include"mutex.h"
namespace Mutex{
//MutexLock
MutexLock::MutexLock(){
    pthread_mutex_init(&mutex_, NULL);
}

MutexLock::~MutexLock(){
    pthread_mutex_destroy(&mutex_);
}


void MutexLock::Lock(){
    pthread_mutex_lock(&mutex_);
}

void MutexLock::UnLock(){
    pthread_mutex_unlock(&mutex_);
}

pthread_mutex_t* MutexLock::GetPthreadMutex(){
    return &mutex_;
}

//Condition
Condition::Condition(MutexLock& lock):
    mutex_lock_(lock)
{
    pthread_cond_init(&pcond_, NULL);
}

Condition::~Condition(){
    pthread_cond_destroy(&pcond_);
}

void Condition::Wait(){
    pthread_cond_wait(&pcond_, mutex_lock_.GetPthreadMutex());
}

int Condition::WaitTimeOut(int timeout){
    struct timespec timer;
    timer.tv_sec = time(0) + timeout;
    timer.tv_nsec = 0;
    return pthread_cond_timedwait(&pcond_, mutex_lock_.GetPthreadMutex(), &timer);
}

void Condition::Notify(){
    pthread_cond_signal(&pcond_);
}

void Condition::NotifyAll(){
    pthread_cond_broadcast(&pcond_);
}

SpinLock::SpinLock(){
    pthread_spin_init(&spin_lock_, 0);
}

SpinLock::~SpinLock(){
    pthread_spin_destroy(&spin_lock_);
}

//Spin lock
void SpinLock::Lock(){
    pthread_spin_lock(&spin_lock_);
}

void SpinLock::UnLock(){
    pthread_spin_unlock(&spin_lock_);
}
};
