#ifndef STRINGDUMPER_H
#define STRINGDUMPER_H
#include <string>
using namespace std;
class StringDumper
{
protected:
	string &output;
	void addToDump(char *buffer) {if (!output.empty()) output.append("\t");output.append(buffer);}
public:
	StringDumper(string &output):output(output) {;}


};
#endif