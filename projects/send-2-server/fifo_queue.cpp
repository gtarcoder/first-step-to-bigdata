#include<assert.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include"fifo_queue.h"

FifoQueue::FifoQueue(uint32_t buf_size):buf_size_(buf_size),
    condition_(mutex_lock_){
    buffer_ = new char[buf_size_];
    buf_end_ = buffer_ + buf_size_;

    read_pos_ = write_pos_ = buffer_;
    read_bytes_ = write_bytes_ = 0;
}

FifoQueue::~FifoQueue(){
    delete[] buffer_;
}

void FifoQueue::Reset(){
    read_pos_ = write_pos_ = buffer_;
    read_bytes_ = write_bytes_ = 0;
}

uint16_t FifoQueue::PeekNextPackLen(){
    uint16_t len;
    if (read_bytes_ >= write_bytes_){
//      printf("peek next pack size, read bytes == write_bytes, buffer empty!\n");
        return 0;
    }

    if (read_pos_ + PACK_LEN_SIZE > buf_end_){
        read_pos_ = buffer_;
    }
    len = *(uint16_t*)read_pos_;
    if (len == END_FLAG){
        read_pos_ = buffer_;
        len = *(uint16_t*)read_pos_;
    }
    return len;
}


bool FifoQueue::PushPacketMutex(char* packet, uint16_t len){
    assert(len < MAX_PACK_SIZE);
    MutexLockGuard lock_guard(mutex_lock_);

    if (write_bytes_ + 2*(len + PACK_LEN_SIZE - 1) > read_bytes_ + buf_size_){
        printf("push data failed, buffer overflow!\n");
        return false;
    }
    if (write_pos_ + PACK_LEN_SIZE > buf_end_){
        write_pos_ = buffer_;
    }else if (write_pos_ + len + sizeof(len) > buf_end_){ 
        *(uint16_t*)write_pos_ = END_FLAG;
        write_pos_ = buffer_;
    }
    
    *(uint16_t*)write_pos_ = len;
    write_pos_ += PACK_LEN_SIZE;
    memcpy((void*)write_pos_, packet, len);
    write_pos_ += len;
    write_bytes_ += (len + PACK_LEN_SIZE);

    if (write_bytes_ > read_bytes_){
        condition_.Notify();
    }
    return true;
}

bool FifoQueue::PopPacketMutex(char* packet, uint16_t* len){
    *len = 0;
    MutexLockGuard lock_guard(mutex_lock_);
    while(read_bytes_ >= write_bytes_){
        condition_.Wait();
    }

    if (read_pos_ + PACK_LEN_SIZE > buf_end_){
        read_pos_ = buffer_;
    }
    *len = *(uint16_t*)read_pos_;
    if (*len == END_FLAG){
        read_pos_ = buffer_;
        *len = *(uint16_t*)read_pos_;
    }
    read_pos_ += PACK_LEN_SIZE;
    memcpy(packet,(void*)read_pos_, *len);
    read_pos_ += *len;
    read_bytes_ += (*len + PACK_LEN_SIZE);
    return true;
}


bool FifoQueue::PushPacketSpin(char* packet, uint16_t len){
    assert(len < MAX_PACK_SIZE);
    SpinLockGuard lock_guard(spin_lock_);
    if (write_bytes_ + 2*(len + PACK_LEN_SIZE - 1) > read_bytes_ + buf_size_){
        printf("push data failed, buffer overflow!\n");
        return false;
    }
    if (write_pos_ + PACK_LEN_SIZE > buf_end_){
        write_pos_ = buffer_;
    }else if (write_pos_ + len + sizeof(len) > buf_end_){ 
        *(uint16_t*)write_pos_ = END_FLAG;
        write_pos_ = buffer_;
    }
    
    *(uint16_t*)write_pos_ = len;
    write_pos_ += PACK_LEN_SIZE;
    memcpy((void*)write_pos_, packet, len);
    write_pos_ += len;
    write_bytes_ += (len + PACK_LEN_SIZE);
    return true;
}

bool FifoQueue::PopPacketSpin(char* packet, uint16_t* len){
    *len = 0;
    spin_lock_.Lock();
    if(read_bytes_ >= write_bytes_){
        spin_lock_.UnLock();
        return false;
    }

    if (read_pos_ + PACK_LEN_SIZE > buf_end_){
        read_pos_ = buffer_;
    }
    *len = *(uint16_t*)read_pos_;
    if (*len == END_FLAG){
        read_pos_ = buffer_;
        *len = *(uint16_t*)read_pos_;
    }
    read_pos_ += PACK_LEN_SIZE;
    memcpy(packet,(void*)read_pos_, *len);
    read_pos_ += *len;
    read_bytes_ += (*len + PACK_LEN_SIZE);
    spin_lock_.UnLock();
    return true;
}

bool FifoQueue::PushPacketLockFree(char* packet, uint16_t len){
    assert(len < MAX_PACK_SIZE);
    if (write_bytes_ + 2*(len + PACK_LEN_SIZE - 1) > read_bytes_ + buf_size_){
        printf("push data failed, buffer overflow!\n");
        return false;
    }
    if (write_pos_ + PACK_LEN_SIZE > buf_end_){
        write_pos_ = buffer_;
    }else if (write_pos_ + len + sizeof(len) > buf_end_){ 
        *(uint16_t*)write_pos_ = END_FLAG;
        write_pos_ = buffer_;
    }
    
    *(uint16_t*)write_pos_ = len;
    write_pos_ += PACK_LEN_SIZE;
    memcpy((void*)write_pos_, packet, len);
    write_pos_ += len;
    write_bytes_ += (len + PACK_LEN_SIZE);
    return true;
}

bool FifoQueue::PopPacketLockFree(char* packet, uint16_t* len){
    *len = 0;
    while(read_bytes_ >= write_bytes_){
        sched_yield();
    }

    if (read_pos_ + PACK_LEN_SIZE > buf_end_){
        read_pos_ = buffer_;
    }
    *len = *(uint16_t*)read_pos_;
    if (*len == END_FLAG){
        read_pos_ = buffer_;
        *len = *(uint16_t*)read_pos_;
    }
    read_pos_ += PACK_LEN_SIZE;
    memcpy(packet,(void*)read_pos_, *len);
    read_pos_ += *len;
    read_bytes_ += (*len + PACK_LEN_SIZE);
    return true;
}



