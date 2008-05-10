
#include "CLogFileLoader.h"
#include <hash_set>
using namespace std;
using namespace stdext;
int main (int argc, char **argv)
{
	wstring wstr(L"C:\\all.LOG");
	CLogFileLoader cLogFileLoader(wstr);
	cLogFileLoader.PreProcessing();
	//print all tags
	cLogFileLoader.PrintInfo();
	
	//Test case:
	//cLogFileLoader.SetLineNumberFilter(10000, 20000);
	
	//cLogFileLoader.SetTimeFilter(cLogFileLoader.GetMinTime()+20.0, cLogFileLoader.GetMinTime()+40.0);
	
	//Test case: filter process
	/*
		vector<int> vec;
		vec.clear();
		vec.push_back(532);
		vec.push_back(2756);
		vec.push_back(1968);
		cLogFileLoader.SetProcessFilter(&vec);
	*/
	//Test case: filter [CLRec4]
		vector<wstring> vec2;
		vec2.push_back(L"[CLSchMgr]");
		vec2.push_back(L"[CLRec4]");
		vec2.push_back(L"EPG");
		vec2.push_back(L"(SQL)");
		cLogFileLoader.SetTagsFilter(&vec2);

	cLogFileLoader.RunFilterResult();
	cLogFileLoader.PrintResult();
	//system("PAUSE");
	return 0;
}