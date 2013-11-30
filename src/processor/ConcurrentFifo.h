#include <queue>
#include <pthread.h>

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
		pthread_mutex_lock(&_mutex);
		_queue.push(data);
		pthread_cond_signal(&_condv);
		pthread_mutex_unlock(&_mutex);
	}
	T &Get()
	{
		pthread_mutex_lock(&_mutex);
		while (_queue.empty())
		{
			pthread_cond_wait(&_condv,&_mutex);
		}
		T &ret=_queue.front();
		_queue.pop();
		pthread_mutex_unlock(&_mutex);
		return ret;

	}
};
