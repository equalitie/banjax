/*
 * FeatureContainerTest.h
 *
 *  Created on: Dec 10, 2013
 *      Author: kees
 */

#ifndef FEATURECONTAINERTEST_H_
#define FEATURECONTAINERTEST_H_
#include "../features/feature_container.h"
#include "../features/feature.h"

/* Needed for the test harness this will setup a FeatureProviderInterface for
 * 1 feature
 */
class FeatureProviderTest:public FeatureProviderInterface
{
	vector<pair<Feature *,int> > _features;
public:
	FeatureProviderTest(Feature *f):
		FeatureProviderInterface()
	{
		SetFeature(f);
	}
	void SetFeature(Feature *feature)
	{
		_features.clear();
		_features.push_back(pair<Feature *,int>(feature,0));
	}
	virtual vector<pair<Feature *,int>> &GetFeatures() {return _features;}
	virtual int GetMaxFeatureIndex() {return 0;};

	virtual int GetFeatureSupportMemory() {return sizeof(double)+_features[0].first->GetDataSize();}
	virtual ~FeatureProviderTest() {delete _features[0].first;}

};


/* Our test harnass, this will register one feature
 *
 */
class FeatureContainerTest:public FeatureContainer
{
	FeatureProviderTest *_fpt; // we need this to delete it afterwards
public:
	FeatureContainerTest(Feature *f):
		FeatureContainer(_fpt=new FeatureProviderTest(f))
	{
	}

	double GetFeatureValue()
	{
		return GetFeatureData()[0];
	}

	virtual ~FeatureContainerTest() { delete _fpt;}
};




#endif /* FEATURECONTAINERTEST_H_ */
