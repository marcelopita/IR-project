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
		string url = doc.getURL();
		string text = doc.getText();

		cout << url << endl;

		doc.clear();
		++i;

		if (i == 200)
			break;
	}

	return 0;
}
