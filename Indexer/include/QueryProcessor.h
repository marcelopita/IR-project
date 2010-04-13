/*
 * QueryProcessor.h
 *
 *  Created on: 11/04/2010
 *      Author: pita
 */

#ifndef QUERYPROCESSOR_H_
#define QUERYPROCESSOR_H_

#include <map>
#include <vector>
#include <set>
#include <string>

using namespace std;

class QueryProcessor {

public:

	QueryProcessor(string&, string&, string&);

	~QueryProcessor();

	//	void query(set<string>&, int, set<string>&);

	void querySingleTerm(string&, set<string>&);

	//	const static int AND = 0;
	//
	//	const static int OR = 1;

private:

	map<string, int> vocabulary;

	vector<string> urls;

	string indexFileNamePrefix;

	//	void getDocsAllWords(set<int>&, set<int>&);
	//
	//	void getDocsSomeWord(set<int>&, set<int>&);

	int changeCharSet(const string&, const string&, string&);

	//	void unionOp(set<set<int> >&, set<int>&);
	//
	//	void intersectionOp(set<set<int> >&, set<int>&);

};

#endif /* QUERYPROCESSOR_H_ */
