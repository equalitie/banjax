#include <math.h>
using namespace std;

struct StdDev
{
	bool cumulative;
	double currentM;
	double currentS;
	int n;

	double lastvalue;


	double GetStdDevP()
	{
		if (n<2) return 0;
		double k=n;
		return sqrt(currentS/(k));
	}

	double GetStdDev()
	{
		if (n<2) return 0;
		double k=n;
		return sqrt(currentS/(k-1.0));
	}

	// see http://mathcentral.uregina.ca/QQ/database/QQ.09.02/carlos1.html

	void AddCumulative(double num)
	{

		double current=num-lastvalue;
		lastvalue=num;
		if (n==0 && !cumulative)
		{
			current=0;
			currentM=0;
			currentS=0;
			cumulative=true;
			return;
		}
		addNum(current);
	}
	void AddNum(double num)
	{
		addNum(num);
	}

protected:
	void addNum(double num)
	{
		n++; // population
		int k=n;

		double previousM=currentM;
		double previousS=currentS;
		double S=0;
		double M=0;

		if (k==1) // special case, first one
		{
			M=num;
			S=0;

		}
		else
		{
			M=previousM+((double) num-previousM)/(double)k;
			S=previousS+((double) num-previousM)*((double) num-M);
			//*featureValue=sqrt(S/((double)k)); // not k-1 but k
		}
		currentM=M;
		currentS=S;
	}
};
