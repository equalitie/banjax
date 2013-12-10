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
	virtual int GetMaxFeatureIndex() {return 1;};

	virtual int GetMemoryNeeded() {return sizeof(double)+_features[0].first->GetDataSize();}
	virtual ~FeatureProviderTest() {delete _features[0].first;}

};

class FeatureContainerTest:public FeatureContainer
{
	FeatureProviderTest *_fpt;
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
