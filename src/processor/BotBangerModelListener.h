#include "svm/svm.h"
#include <string.h>
#include "FeatureStdDevData.h"
#include "BotBangerAggregrator.h"

class BotBangerModelListener:public BotBangerEventListener
{
	svm_node *_nodes;
	svm_model *_model;


public:
	BotBangerModelListener(const char *model_file_name):
		BotBangerEventListener(),
		_nodes(NULL),
		_model(NULL)
	{
		_model=svm_load_model(model_file_name);
	}
	~BotBangerModelListener()
	{
		if (_nodes) delete [] _nodes;
		svm_free_and_destroy_model(&_model);
	}

	void OnFeatureEvent(char *key,float *features,int numFeatures) 
	{		
		struct FeatureStdDevData stdev;
		memset(&stdev,0,sizeof(FeatureStdDevData));

		if (!_nodes)
		{
			_nodes=new svm_node[numFeatures+1];
			memset(_nodes,0,sizeof(svm_node)*(numFeatures+1));
			_nodes[numFeatures].index=-1;
		}
		for(int i=0;i<numFeatures;i++)
		{
			stdev.AddNum(features[i]);
		}
		for(int i=0;i<numFeatures;i++)
		{
			_nodes[i].index=i;
			double stdvalue=stdev.GetStdDevP();
			if (stdvalue)
			_nodes[i].value=(features[i]-stdev.currentM)/stdvalue;
			else
				_nodes[i].value=0;

		}		

		double val=svm_predict(_model,_nodes);
		OnModelValue(key,val);
		
	}
	virtual void OnModelValue(char *key,double value)
	{
		UNUSED(key);
		UNUSED(value);
	}
};
