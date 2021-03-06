#ifndef SYNCQUEUE_H
#define SYNCQUEUE_H

#include <windows.h>
#include <stdio.h>

struct QueueIsFullException{};
struct QueueIsEmptyExcetption{};
class SyncQueue
{
    int* queue_;
    int size_;
    int count_;  
    int head_;
    int tail_;
    CRITICAL_SECTION cs_;
	HANDLE hSemaphoreFill;
	HANDLE hSemaphoreEmpty;	 

public:
    SyncQueue(int size_);
    ~SyncQueue();
    void insert(int x); // add element to queue
    int remove (); // remove element frm queue
    int capacity() {return size_;}
    int size() {return count_;}
    bool isFull () {return count_==size_;}
    bool isEmpty () {return !count_;}
    void print ()
    {
        for (int i =0;i<count_;i++)
        {
            printf ("%4d",queue_[(size_+head_+i)%size_]);
        }
        printf ("\n");
    }
};

#endif
