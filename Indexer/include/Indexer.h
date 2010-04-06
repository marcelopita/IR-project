/*
 * Indexer.h
 *
 *  Created on: 23/03/2010
 *      Author: pita
 */

#ifndef INDEXER_H_
#define INDEXER_H_

#include <string>
#include <fstream>
#include "CollectionReader.h"
#include <vector>
#include <map>

using namespace std;
using namespace RICPNS;

class Indexer {

public:

	Indexer(string collectionDirectory, string collectionIndexFileName);

	~Indexer();

	int index();

	const static int DOC_OK = 0;

	const static int DOC_BLOCKED = 1;

private:

	string collectionDirectory;
	string collectionIndexFileName;
	CollectionReader *reader;

	map<string, int> vocabulary;
	vector<string> documents;

	string whitespaces1;
	string whitespaces2;

	void trim(std::string &str, std::string &whitespaces);

	void tokenize(const string& str, vector<string>& tokens,
			const string& delimiters);

	int filterInvalidHTTPHeader(string &docText);

	int preprocessDocument(Document &doc, string &usefulText,
			int &numDocsInvalidHTTPHeader, int &numDocsInvalidContentType,
			int &numDocsUnknownCharSet);

	void sortTempFile();

	void sortTriples();

};

#endif /* INDEXER_H_ */
