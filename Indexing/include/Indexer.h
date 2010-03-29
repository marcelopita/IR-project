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

using namespace std;
using namespace RICPNS;

class Indexer {

public:

	Indexer(string collectionDirectory, string collectionIndexFileName);

	~Indexer();

	int index();

private:

	string collectionDirectory;
	string collectionIndexFileName;
	CollectionReader *reader;

	char **vocabulary;
	char **documents;

};

#endif /* INDEXER_H_ */
