#ifndef LOGAGGREGATOR_H
#define LOGAGGREGATOR_H
struct LogEntry;

/* base class for the aggregators,
 * there is not much functionality yet, but if we
 * want to build more generic aggregators this will be the starting point
 */
class LogAggregator
{
public:
	virtual void Aggregate(LogEntry *)=0;
	virtual ~LogAggregator() {;}
};
#endif
