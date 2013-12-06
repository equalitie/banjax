#ifndef CONCURRENTFIFO_H
#define CONCURRENTFIFO_H
#include <queue>
#include <pthread.h>
#include <string.h>





class lock
{
	pthread_mutex_t &_mutex;
public:
	lock(pthread_mutex_t &mutex):_mutex(mutex)
	{
		pthread_mutex_lock(&_mutex);
	}
	~lock()
	{
		pthread_mutex_unlock(&_mutex);
	}
};

template<typename T>
class ConcurrentFifo
{
private:
	std::queue<T> _queue;
	pthread_mutex_t _mutex;
	pthread_cond_t _condv;
public:
	ConcurrentFifo()
	{
		pthread_mutex_init(&_mutex,NULL);
		pthread_cond_init(&_condv,NULL);


	}
	~ConcurrentFifo()
	{
		pthread_cond_destroy(&_condv);
		pthread_mutex_destroy(&_mutex);
	}
	void Add(T data)
	{
		lock lockme(_mutex);
		__sync_synchronize();
		_queue.push(data);
		pthread_cond_signal(&_condv);

	}
	T Get()
	{

		lock lockme(_mutex);
		while (_queue.empty())
		{
			pthread_cond_wait(&_condv,&_mutex);
			__sync_synchronize();
		}

		T ret=_queue.front();


		_queue.pop();
		return ret;


	}
};


struct FifoMessage
{
		size_t messagesize;
	char data[2];
	void deleteMessage() {
		delete [] ((char *) this);
	}
	static FifoMessage *create(void *data,size_t length)
	{
		FifoMessage *v=(FifoMessage *) new char [sizeof(FifoMessage)+length+1];
		v->messagesize=length;
		memcpy(v->data,data,length);
		return v;
	}
	void CopyMessageData(void *data)
	{
		memcpy(data,this->data,messagesize);
	}
	int size() {return messagesize;}
};
#endif
