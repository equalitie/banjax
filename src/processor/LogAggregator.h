#ifndef LOGAGGREGATOR_H
#define LOGAGGREGATOR_H
struct LogEntry;
class LogAggregator
{
public:
	virtual void Aggregate(LogEntry *)=0;
	virtual ~LogAggregator() {;}
};
#endif
