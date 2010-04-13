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

struct Triple;
struct RunTriple;

class Indexer {

public:

	Indexer(string collectionDirectory, string collectionIndexFileName,
			string tempFileName, string indexFileName, int runSize);

	~Indexer();

	int index();

	const static int DOC_OK = 0;

	const static int DOC_BLOCKED = 1;

private:

	string collectionDirectory;
	string collectionIndexFileName;
	string tempDir;
	string tempFilePrefix;
	string finalTempFileName;
	string indexFileNamePrefix;
	int runSize;
	int k;
	int lastDocumentEntry;
	int lastPositionEntry;

	CollectionReader *reader;

	map<string, int> vocabulary;
	vector<string> documents;
	vector<Triple> kTriples;
	vector<Triple> triplesPerTerm;
	int numRuns;
	int numTriplesSaved;

	int separateHeaderContent(string&, string&, string&);

	int getContentTypeHeader(string&, string&);

	void getCharSetHeader(string&, string&);

	void getCharSetMetaTag(string&, string&);

	int changeCharSet(const string&, const string&, string&);

	void extractUsefulContent(string&, string&);

	void compress(char*, char*);

	void uncompress(char*, char*);

	void compressTriple(char*, char*);

	void uncompressTriple(char*, char*);

	void saveTriplesTempFile(vector<string>&, int);

	void saveTripleTempFile(string&, int&, int&);

	void saveTriplesVectorTempFile();

	void writeInvertedFile();

	void mergeSortedRuns();

	void trim(std::string &str, const std::string &whitespaces);

	void tokenize2(const string&, vector<string>&, bool);

	void tokenize(const string&, vector<string>&, const string&, bool);

};

#endif /* INDEXER_H_ */
