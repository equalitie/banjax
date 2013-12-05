#include "BotBangerAggregrator.h"
class BotBangerLogger:public BotBangerEventListener
{
public:
	void OnFeatureEvent(char *key,double *features,int numFeatures) {};
};
