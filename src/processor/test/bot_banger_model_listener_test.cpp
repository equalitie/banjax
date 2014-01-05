#include <gtest/gtest.h>
#include "../botbanger/bot_banger_aggregator.h"
#include "../botbanger/bot_banger_model_listener.h"
#include "log_entry_test.h"
#include "feature_set_value.h"

class BotBangerModelListenerTest:public BotBangerModelListener
{
public:
	double *values;
	int numNodes;
	string key;


	string modelKey;
	string modelName;
	string modelAction;

	BotBangerModelListenerTest():BotBangerModelListener(),values(NULL),numNodes(-1)
	{

	}
	virtual void OnNodeValues(char *key,svm_node *values,int num)
	{
		this->key=string(key);
		this->numNodes=num;
		if (this->values) delete [] values;
		this->values=new double[num<=0 ? 1 : num];
		for (int c=0;c<num;c++)
			this->values[c]=values[c].value;
	}

	virtual void OnModelAction(char *key,string &modelName,string &action)
	{
		this->modelAction=action;
		this->modelKey=string(key);
		this->modelName=string(modelName);
	}
	~BotBangerModelListenerTest()
	{
		if (this->values) delete [] values;
		this->values=NULL;
	}
};

void TestFeaturesAgainstNodeValues(
		const double *featureValues,
		const double *normalizedNodeValues,
		int numValues
		)
{
	BotBangerAggregator bag(100,1800);
	auto listener=new BotBangerModelListenerTest();
	bag.RegisterEventListener(listener);
	for(int i=0;i<numValues;i++)
		bag.RegisterFeature(new FeatureSetValue(featureValues[i]),i);
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	bag.Aggregate(&le);
	EXPECT_EQ(numValues,listener->numNodes);
	for (int i=0;i<numValues;i++)
		EXPECT_EQ(normalizedNodeValues[i],listener->values[i]);

}

void TestFeaturesAgainstActions(
		const double *featureValues,
		string action,
		BotBangerModelListenerTest *listener,
		int numValues
		)
{
	BotBangerAggregator bag(100,1800);

	bag.RegisterEventListener(listener);
	for(int i=0;i<numValues;i++)
		bag.RegisterFeature(new FeatureSetValue(featureValues[i]),i);
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	bag.Aggregate(&le);
	EXPECT_EQ(action,listener->modelAction);

}

/*
 * Test the normalization of the model listener
 * we do not need a model for this test
 */
TEST(bot_banger_aggregator_model_listener,normalization_test)
{
	double f1[]={0.0,0.0,0.0,0.0};
	double n1[]={0.0,0.0,0.0,0.0};
	TestFeaturesAgainstNodeValues(
			f1,
			n1,
			4
			);

	double f2[]={1.0,1.0,1.0,1.0};
	double n2[]={0.0,0.0,0.0,0.0};
	TestFeaturesAgainstNodeValues(
			f2,
			n2,
			4
			);

	double f3[]={1.0,0.0,1.0,0.0};
	double n3[]={1.0,-1.0,1.0,-1.0};
	TestFeaturesAgainstNodeValues(
			f3,
			n3,
			4
			);
}

/*
 * Test a model, this test does not work yet, we need a model which is
 * predictable
 */
TEST(bot_banger_aggregator_model_listener,model_test_not_implemented)
{
	EXPECT_EQ(1,0);
	return ;
/*
	double f1[]={0.0,0.0,0.0,0.0};
	svm_model *model=NULL;
*/

}


