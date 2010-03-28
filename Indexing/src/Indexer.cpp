/*
 * Indexer.cpp
 *
 *  Created on: 23/03/2010
 *      Author: pita
 */

#include "../include/Indexer.h"
#include <iostream>

//using namespace std;

Indexer::Indexer(string collectionDirectory, string collectionIndexFileName) {
	this->collectionDirectory = collectionDirectory;
	this->collectionIndexFileName = collectionIndexFileName;
}

Indexer::~Indexer() {

}

int Indexer::index() {
	this->reader = new CollectionReader(this->collectionDirectory,
			this->collectionIndexFileName);

	Document doc;
	doc.clear();
	int i = 0;

	while (reader->getNextDocument(doc)) {
		cout << "sadasdasd" << endl;

		doc.clear();
		++i;
	}

	return 0;
}
