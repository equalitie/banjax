#include <time.h>
#include "HostHitMissAggregator.h" 
#include <iostream>
class HostHitMissDumper:public HostHitMissEventListener
{
	string &output;
public:
	HostHitMissDumper(string &output):
		output(output)
	{
		this->output=output;
	}
	void OnHostHitMissEvent(char *host,HitMissRange *hmrange)
	{
		if ((!hmrange->reported) && hmrange->total>10)
		{
			hmrange->reported=1;
			struct tm time;
			gmtime_r(&hmrange->from,&time);

			
			std::cout << time.tm_hour <<":" << time.tm_min << ":" << time.tm_sec<< "\t" << host << "\t" << hmrange->totalCount << "\t" << hmrange->total << "\t" <<  hmrange->hits << "\t" << hmrange->ratio << endl;
		}
	}

};
