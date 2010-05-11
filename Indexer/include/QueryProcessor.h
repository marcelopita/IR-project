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

	enum Model {
		BOOLEAN = 1, VECTOR_SPACE = 2, BM25 = 3, HYBRID = 4, MY_MODEL = 5
	};

	enum QueryResult {
		FAILURE = 0, SUCCESS = 1
	};

	QueryProcessor(string&, string&, string&);

	virtual QueryResult query(vector<string>&, vector<string>&) = 0;

protected:

	map<string, int> vocabulary;

	vector<int> docsSizes;

	vector<string> urls;

	string indexFileNamePrefix;

};

#endif /* QUERYPROCESSOR_H_ */
