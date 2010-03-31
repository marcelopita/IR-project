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
#include <fstream>
#include <htmlcxx/html/ParserDom.h>

Indexer::Indexer(string collectionDirectory, string collectionIndexFileName) {
	this->collectionDirectory = collectionDirectory;
	this->collectionIndexFileName = collectionIndexFileName;
}

Indexer::~Indexer() {

}

int Indexer::index() {
	fstream tempFile("/tmp/tempIndex", ios_base::app);

	this->reader = new CollectionReader(this->collectionDirectory,
			this->collectionIndexFileName);

	Document doc;
	doc.clear();
	int i = 0;

	while (reader->getNextDocument(doc)) {
		string url = doc.getURL();
		this->documents.push_back(url);

		string text = doc.getText();
		string httpResponseCodeMsg;

		stringstream ss(text);
		getline(ss, httpResponseCodeMsg);

		string contentType;
		string tempHeaderLine;
		bool endHeader = false;

		while (!endHeader) {
			getline(ss, tempHeaderLine);

			if (tempHeaderLine.find("Content-Type:") != string::npos) {
				contentType.assign(tempHeaderLine);

			} else if (tempHeaderLine.compare("\r") == 0) {
				endHeader = true;
			}
		}

		if (httpResponseCodeMsg.find("200") != string::npos
				|| httpResponseCodeMsg.find("203") != string::npos
				|| httpResponseCodeMsg.find("206") != string::npos) {
			string htmlText;
			getline(ss, htmlText, (char) 26);

			using namespace htmlcxx::HTML;

			ParserDom parser;
			tree<Node> dom = parser.parseTree(htmlText);

			tree<Node>::iterator it = dom.begin();
			tree<Node>::iterator end = dom.end();

//			tree<Node> n;
//			tree_node tn;
//			n.

			for (; it != end; ++it) {
//				if ((!it->isTag()) && (!it->isComment()) && (it->head->tagName() == "p")) {
//					cout << it->text();
//				}
				if (it->tagName() == "p" || it->tagName()=="P") {
//					cout << "sadasdsd" << endl;

						cout << it->closingText() << endl;
//				  		it->parseAttributes();
//						cout << it->attribute("href").second << endl;
				}

//				cout << it->text() << endl;
//				if ((!it->isTag()) && (!it->isComment())) {
//					cout << it->text() << endl;
//					string tagName = it->tagName();
//
//					if (tagName == "p") {
//						//						it->parseAttributes();
//						string pureText;
//						(*it) >> pureText;
//						cout << pureText;
//						cout << it->text() << endl;
//					}
//				}
			}
		}

		doc.clear();
		++i;

		//		if (i % 10000 == 0)
		//			cout << i << endl;

		if (i == 5)
			break;
	}

	return 0;
}
