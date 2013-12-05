#include <math.h>
using namespace std;

struct StdDev
{
	double currentM;
	double currentS;
	int n;
	double lastvalue;


	double GetStdDevP()
	{
		if (n<2) return 0;
		double k=n-1;
		return sqrt(currentS/k);
	}

	double GetStdDev()
	{
		if (n<=2) return 0;
		double k=n-1.0;
		return sqrt(currentS/(k-1.0)); // we should optimize this, but readability above all
	}

	// see http://mathcentral.uregina.ca/QQ/database/QQ.09.02/carlos1.html

	void AddCumulative(double num)
	{
		double current=num-lastvalue;
		lastvalue=num;
		if (n==0) current=0;
		AddNum(current);
	}
	void AddNum(double num)
	{
		n++; // population
		int k=n-1;

		if (k==0) // special case, first number
		{
			currentM=0;
			currentS=0;
		}
		else
		{
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
	}



};
