#ifndef BOTBANGERMODELLISTENER_H
#define BOTBANGERMODELLISTENER_H
#include "../utils/svm/svm.h"
#include "../utils/std_dev.h"
#include <string.h>
#include <vector>

#include "bot_banger_aggregator.h"

using namespace std;

struct bbmlConfigLine;
/* This class listens to the BotBangerAggregator and calculates a normalized version of the features
 * for the model(s) registered via AddConfigLine, the model is owned by this listener
 * For inheritance this class exposes
 * OnModelAction an event which exposes the key (ip address,action and modelname)
 * OnNodeValues an event which exposes the node values which are fed to the model
 * OnModelValue an event which exposes the value of the model after calculation
 */
class BotBangerModelListener:public BotBangerEventListener
{
	svm_node *_nodes; // memory for the model nodes
	vector<bbmlConfigLine *> _configuration; // configuration lines


public:
	BotBangerModelListener():  // constructor
		BotBangerEventListener(), // we inherit from BotBangerEventListener
		_nodes(NULL)
	{

	}

	/* Add a configuration, lowerValue and upperValue (both inclusive) are matched against the
	 * svm_predict outcome and will yield the action, the model will be owned by BotBangerModelListener
	 */
	void AddConfigLine(string &modelName,svm_model *model,double lowerValue,double upperValue,string action);

	/* heavy lifting, catches features from BotBangerAggregator, calculates the
	 * model values and fires the appropriate events
	 */
	void OnFeatureEvent(char *key,double *features,int numFeatures);

	/* The event which exposes the key (ip address,action and modelname)
	 */
	virtual void OnModelAction(char *key,string &modelName,string &action)
	{
		UNUSED(key);
		UNUSED(modelName);
		UNUSED(action);
	}

	/* The event which exposes the node values which are fed to the model
	 */
	virtual void OnNodeValues(char *key,svm_node *values,int num)
	{
		UNUSED(key);
		UNUSED(values);
		UNUSED(num);
	}

	/* The event which exposes the value of the model after calculation
	 */
	virtual void OnModelValue(char *key,string &modelName,double value)
	{
		UNUSED(key);
		UNUSED(modelName);
		UNUSED(value);
	}
	virtual ~BotBangerModelListener();
};
#endif
