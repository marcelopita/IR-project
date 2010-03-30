/*
 * Indexer.cpp
 *
 *  Created on: 23/03/2010
 *      Author: pita
 */

#include "../include/Indexer.h"
#include <iostream>
#include <vector>
#include <sstream>

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

		vector<string> httpHeaderLines;
		string tempHttpHeaderBuffer;
		stringstream ss(text);

		while (getline(ss, tempHttpHeaderBuffer)) {
			if (tempHttpHeaderBuffer.compare("\r") != 0) {
				httpHeaderLines.push_back(tempHttpHeaderBuffer);

			} else {
				break;
			}
		}

		if (httpHeaderLines.at(0).find("200") == string::npos &&
				httpHeaderLines.at(0).find("203") == string::npos &&
				httpHeaderLines.at(0).find("206") == string::npos) {
			cout << httpHeaderLines.at(0);

		} else {

		}

		//		for (unsigned j = 0; j < httpHeaderLines.size(); j++) {
		//			cout << httpHeaderLines.at(j) << endl;
		//		}

		//		cout << "==================================================" << endl;
		//		cout << text << endl;
		//		cout << "==================================================" << endl;

		doc.clear();
		++i;

//		if (i == 1000)
//			break;
	}

	return 0;
}
