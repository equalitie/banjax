#include "../utils/svm/svm.h"
#include "../utils/std_dev.h"
#include <string.h>
#include <vector>

#include "bot_banger_aggregator.h"
#include "bot_banger_model_listener.h"

using namespace std;


/* Configuration lines for a  model
 */
struct bbmlConfigLineSub
{
	double lowerValue; // lower value to match inclusive
	double upperValue; // upper value to match inclusive
	string action; // action which must be taken if value is matched
	bbmlConfigLineSub(double lowerValue,double upperValue,string action): // constructor
		lowerValue(lowerValue),
		upperValue(upperValue),
		action(action)
	{

	}


};

/* Configuration line per model
 */
struct bbmlConfigLine
{
	svm_model *model; // svm modem, owned
	string modelName; // nodel name
	vector<bbmlConfigLineSub> subLines; // sub lines, owned

	bbmlConfigLine(string &modelName,svm_model *model):
		model(model),
		modelName(modelName)

	{
	}
	/* Add a configuration
	 */
	void addConfig(double lowerValue,double upperValue,string action)
	{
		subLines.push_back(bbmlConfigLineSub(lowerValue,upperValue,action));
	}

	/* determine action, given a predicted value
	 */
	bool determineAction(double predictValue,string &action)
	{
		for(auto i=subLines.begin();i!=subLines.end();i++)
		{
			if (predictValue>=(*i).lowerValue && predictValue<=(*i).upperValue)
			{
				action=(*i).action;
				return true;
			}
		}
		return false;
	}
	/* destructor and cleanup
	 */
	~bbmlConfigLine() {svm_free_and_destroy_model(&model);}

};

/* Add a configuration, lowerValue and upperValue (both inclusive) are matched against the
 * svm_predict outcome and will yield the action, the model will be owned by BotBangerModelListener
 */
void BotBangerModelListener::AddConfigLine(string &modelName,svm_model *model,double lowerValue,double upperValue,string action)
{
	for (auto i=_configuration.begin();i!=_configuration.end();i++) // try to find the model
	{
		if (model==(*i)->model) // model found
		{
			(*i)->addConfig(lowerValue,upperValue,action); // add additional configuration
			return;
		}
	}
	auto item=new bbmlConfigLine(modelName,model); // new model configuration
	item->addConfig(lowerValue,upperValue,action); // add configuration
	_configuration.push_back(item);
}

/* heavy lifting, catches features from BotBangerAggregator, calculates the
 * model values and fires the appropriate events
 */
void BotBangerModelListener::OnFeatureEvent(char *key,double *features,int numFeatures)
{


	if (!_nodes) // create nodes
	{
		_nodes=new svm_node[numFeatures+1];
		memset(_nodes,0,sizeof(svm_node)*(numFeatures+1));
		_nodes[numFeatures].index=-1; // last node has index -1
	}

	// normalization
	// we are lazy, we use the incremental stddev
	// could be optimized
	struct StdDev stdev;
	memset(&stdev,0,sizeof(StdDev));
	for(int i=0;i<numFeatures;i++) // add all features
	{
		stdev.AddNum(features[i]);
	}
	for(int i=0;i<numFeatures;i++) // normalize and generate the nodes
	{
		_nodes[i].index=i;
		double stdvalue=stdev.GetStdDevP();
		if (stdvalue)
		_nodes[i].value=(features[i]-stdev.currentM)/stdvalue;
		else
			_nodes[i].value=0;
	}

	// Let listener know which node values are generated
	OnNodeValues(key,_nodes,numFeatures);
	string action;
	for(auto i=_configuration.begin();i!=_configuration.end();i++)
	{

		// could be moved to bbmlConfigLine
		double val=svm_predict((*i)->model,_nodes);
		// let listener know which predicted value is generated
		OnModelValue(key,(*i)->modelName,val);
		if ((*i)->determineAction(val,action))
		{
			// let listener know which action is triggered
			OnModelAction(key,(*i)->modelName,action);
		}
	}
}

BotBangerModelListener::~BotBangerModelListener()
{
	if (_nodes) delete [] _nodes;
	for(auto i=_configuration.begin();i!=_configuration.end();i++)
	{
		delete (*i);
	}
	_configuration.clear();
}




