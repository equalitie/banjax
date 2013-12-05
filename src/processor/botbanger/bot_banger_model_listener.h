#ifndef BOTBANGERMODELLISTENER_H
#define BOTBANGERMODELLISTENER_H
#include "../utils/svm/svm.h"
#include "../utils/std_dev.h"
#include <string.h>
#include <vector>

#include "bot_banger_aggregator.h"

using namespace std;


struct bbmlConfigLineSub
{
	double lowerValue;
	double upperValue;
	string action;
	bbmlConfigLineSub(double lowerValue,double upperValue,string action):
		lowerValue(lowerValue),
		upperValue(upperValue),
		action(action)
	{

	}


};
struct bbmlConfigLine
{
	svm_model *model;
	string modelName;
	vector<bbmlConfigLineSub> subLines;
	bbmlConfigLine(string &modelName,svm_model *model):
		model(model),
		modelName(modelName)

	{
	}
	void addConfig(double lowerValue,double upperValue,string action)
	{
		subLines.push_back(bbmlConfigLineSub(lowerValue,upperValue,action));
	}
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
	~bbmlConfigLine() {svm_free_and_destroy_model(&model);}

};

class BotBangerModelListener:public BotBangerEventListener
{
	svm_node *_nodes;
	svm_model *_model;
	vector<bbmlConfigLine *> _configuration;


public:
	BotBangerModelListener():
		BotBangerEventListener(),
		_nodes(NULL),
		_model(NULL)
	{

	}

	void AddConfigLine(string &modelName,svm_model *model,double lowerValue,double upperValue,string action)
	{
		for (auto i=_configuration.begin();i!=_configuration.end();i++)
		{
			if (model==(*i)->model)
			{
				(*i)->addConfig(lowerValue,upperValue,action);
				return;
			}
		}
		auto item=new bbmlConfigLine(modelName,model);
		item->addConfig(lowerValue,upperValue,action);
		_configuration.push_back(item);
	}

	void OnFeatureEvent(char *key,double *features,int numFeatures)
	{		


		if (!_nodes)
		{
			_nodes=new svm_node[numFeatures+1];
			memset(_nodes,0,sizeof(svm_node)*(numFeatures+1));
			_nodes[numFeatures].index=-1;
		}

		// we are lazy, this could be done more efficient
		struct StdDev stdev;
		memset(&stdev,0,sizeof(StdDev));
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

		OnNodeValues(key,_nodes,numFeatures);
		string action;
		for(auto i=_configuration.begin();i!=_configuration.end();i++)
		{

			// should be moved to bbmlConfigLine
			double val=svm_predict((*i)->model,_nodes);
			OnModelValue(key,(*i)->modelName,val);
			if ((*i)->determineAction(val,action))
			{
				OnModelAction(key,(*i)->modelName,action);
			}
		}
		
	}
	virtual void OnModelAction(char *key,string &modelName,string &action)
	{
		UNUSED(key);
		UNUSED(modelName);
		UNUSED(action);
	}
	virtual void OnNodeValues(char *key,svm_node *values,int num)
	{
		UNUSED(key);
		UNUSED(values);
		UNUSED(num);
	}

	virtual void OnModelValue(char *key,string &modelName,double value)
	{
		UNUSED(key);
		UNUSED(modelName);
		UNUSED(value);
	}
	virtual ~BotBangerModelListener()
	{
		if (_nodes) delete [] _nodes;
		for(auto i=_configuration.begin();i!=_configuration.end();i++)
		{
			delete (*i);
		}
		_configuration.clear();
	}
};
#endif
