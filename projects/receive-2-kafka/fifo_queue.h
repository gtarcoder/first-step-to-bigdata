#ifndef FIFO_QUEUE_H_
#define FIFO_QUEUE_H_
#include<stdint.h>
#include"mutex.h"
using namespace Mutex;

#define MAX_PACK_SIZE 65535
#define INIT_BUF_SIZE 500000000
#define END_FLAG 0x0000
#define PACK_LEN_SIZE 2
#define CACHE_LINE_SIZE 64

class FifoQueue{
public:
    FifoQueue(uint32_t buf_size = INIT_BUF_SIZE);
    ~FifoQueue();

    void Reset();

    uint16_t PeekNextPackLen();

    bool PushPacketMutex(char* packet, uint16_t len);

    bool PopPacketMutex(char* pacet, uint16_t * len);

    bool PushPacketSpin(char* packet, uint16_t len);

    bool PopPacketSpin(char* pacet, uint16_t * len);

    bool PushPacketLockFree(char* packet, uint16_t len);

    bool PopPacketLockFree(char* pacet, uint16_t * len);


private:
    char* buffer_;
   // char paddings1[CACHE_LINE_SIZE - sizeof(char*)];
    char* buf_end_; 
   // char paddings2[CACHE_LINE_SIZE - sizeof(char*)];
    volatile char* read_pos_;
  //  char paddings3[CACHE_LINE_SIZE - sizeof(char*)];
    volatile char* write_pos_;
  //  char paddings4[CACHE_LINE_SIZE - sizeof(char*)];

    volatile uint64_t read_bytes_;
  //  char paddings5[CACHE_LINE_SIZE - sizeof(uint64_t)];
    volatile uint64_t write_bytes_;
  //  char paddings6[CACHE_LINE_SIZE - sizeof(uint64_t)];
    uint32_t buf_size_;

    MutexLock mutex_lock_;
    Condition condition_;
    SpinLock spin_lock_;
};
#endif
