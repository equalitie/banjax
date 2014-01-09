#ifndef HOST_HIT_MISS_ACTIONS_H
#define HOST_HIT_MISS_ACTIONS_H
#include <time.h>
#include "host_hit_miss_aggregator.h"
#include "../utils/string_dumper.h"
#include <iostream>


/* configuration match line for hosts */
struct HostHitMissConfigLine
{
	int requestLower; // lower bound (inclusive) for requests per period
	int requestUpper; // upper bound (inclusive) for requests per period
	int runTime; // runTime for this action
	double ratioLower; // lower bound (inclusive) for ratio
	double rationUpper; // upper bound (inclusive) for ratio
	string host; // host to match
	string action; // action to generate
};

/* configuration per host */
struct HostConfig
{
	string currentAction; // current action
	time_t expires; // expiration date for action, as long as this is not reached no new actions will be generated
	vector<HostHitMissConfigLine *> configLines; // sub config

	~HostConfig() // cleanup
	{
		for (auto i=configLines.begin();i<configLines.end();i++) delete (*i);
	}
};

/* this class listens to the HostHitMissAggregaor events and determines an action
 * For inheritance this class exposes
 * OnScheduleAction if an action configuration matches this will be called
 */
class HostHitMissActions:public HostHitMissEventListener
{
	map<string,HostConfig> configuration; // host, configuration mapping
public:
	/* add host/action match,
	 * action is the action to be generated if the match is successful
	 * requestLower/requestUpper matches the number of requests in a period
	 * ratioLower/ratioUpper matches the ratio hits/total
	 * runTime is the runtime for this action, during this runtime, no other actions will be generated
	 */
	void AddConfigLine(string host,string action,int requestLower,int requestUpper,double ratioLower,double ratioUpper,int runTime);
	/* add a configuration struct */
	void AddConfigLine(struct HostHitMissConfigLine *l);
	/* catches the HostHitMissEvent and determines the actions to be generated
	 */
	void OnHostHitMissEvent(char *host,HitMissRange *hmrange);
	/* fires when an action is generated
	 */
	virtual void OnScheduleAction(HitMissRange *hmr,string &host,string &action,string &currentaction)=0;
	virtual ~HostHitMissActions();

};


#endif
