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
	unsigned int _maxsize;
	std::queue<T> _queue;
	pthread_mutex_t _mutex;
	pthread_cond_t _condv;
	bool internalAdd(T data,bool forced)
	{
		lock lockme(_mutex);
		if (!forced && _maxsize>0 && _queue.size()==_maxsize)
		{
			return false;
		}
		_queue.push(data);
		pthread_cond_signal(&_condv);
		return true;

	}
public:
	int GetMaxSize()
	{
		return _maxsize;
	}
	bool Add(T data)
	{
		return internalAdd(data,false);
	}
	bool ForcedAdd(T data)
	{
		return internalAdd(data,true);

	}
	ConcurrentFifo(int maxsize=0):
		_maxsize(maxsize)
	{
		pthread_mutex_init(&_mutex,NULL);
		pthread_cond_init(&_condv,NULL);


	}
	~ConcurrentFifo()
	{
		pthread_cond_destroy(&_condv);
		pthread_mutex_destroy(&_mutex);
	}

	T Get()
	{

		lock lockme(_mutex);
		while (_queue.empty())
		{
			pthread_cond_wait(&_condv,&_mutex);
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
