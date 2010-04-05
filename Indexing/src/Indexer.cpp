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

	ifstream charSetsFile("charsets.txt");
	for (int i = 0; i < 1153; i++) {
		getline(charSetsFile, charSets[i]);
	}
}

Indexer::~Indexer() {
	delete this->reader;
}

void Indexer::trim(std::string &str, std::string &whitespaces) {
	std::string::size_type pos = str.find_last_not_of(whitespaces);
	if (pos != std::string::npos) {
		str.erase(pos + 1);
		pos = str.find_first_not_of(whitespaces);
		if (pos != std::string::npos)
			str.erase(0, pos);
	} else
		str.erase(str.begin(), str.end());
}

int Indexer::filterInvalidHTTPHeader(string &docText) {
	// Get HTTP return status code
	stringstream ss(docText);
	string httpResponseCodeMsg;
	getline(ss, httpResponseCodeMsg);

	// Exclude documents without HTTP header or with returned status code different of 200, 203 or 206
	if (httpResponseCodeMsg.find("HTTP") == string::npos
			|| (httpResponseCodeMsg.find("200") == string::npos
					&& httpResponseCodeMsg.find("203") == string::npos
					&& httpResponseCodeMsg.find("206") == string::npos)) {
		return Indexer::DOC_BLOCKED;
	}

	return Indexer::DOC_OK;
}

int Indexer::index() {
	cout << "index" << endl;

	fstream tempFile("/tmp/tempIndex", ios_base::app);

	this->reader = new CollectionReader(this->collectionDirectory,
			this->collectionIndexFileName);

	Document doc;
	doc.clear();
	int i = 0;
	int numDocsInvalidHTTPHeader = 0;
	int numDocsInvalidContentType = 0;
	int numDocsUnknownContentType = 0;

	while (reader->getNextDocument(doc)) {
		// Index document URL
		string url = doc.getURL();
		this->documents.push_back(url);

		// Full text
		string text = doc.getText();

		cout << text << endl;

//		// Filter 1: Remove docs with invalid HTTP header
//		if (this->filterInvalidHTTPHeader(text) == Indexer::DOC_BLOCKED) {
//			++numDocsInvalidHTTPHeader;
//
//			doc.clear();
//			++i;
//			continue;
//		}

//		stringstream ss(text);
//		string tempHeaderLine;
//		bool endHeader = false;
//
//		if (i == 13850) {
//			cout << "--------------" << endl;
//			cout << text;
//			cout << "--------------" << endl;
//			break;
//		}

//		while (!endHeader) {
//			getline(ss, tempHeaderLine);
//
//			endHeader = tempHeaderLine.compare("\r") == 0;
//		}

		//		stringstream ss(text);
		//		string headerText;
		//		getline(ss, headerText, '\r');
		//
		//		cout << "--------------------------" << endl;
		//		cout << headerText;

		//		string contentType;
		//		string tempHeaderLine;
		//		bool endHeader = false;
		//
		//		while (!endHeader) {
		//			getline(ss, tempHeaderLine);
		//
		//			if (tempHeaderLine.find("Content-Type:") != string::npos) {
		//				contentType.assign(tempHeaderLine);
		//
		//			} else if (tempHeaderLine.compare("\r") == 0) {
		//				endHeader = true;
		//			}
		//		}
		//
		//		string htmlText;
		//		getline(ss, htmlText, (char) 26);
		//
		//		string usefulText;
		//
		//		using namespace htmlcxx::HTML;
		//
		//		ParserDom parser;
		//		parser.parse(htmlText);
		//		tree<Node> rootNode = parser.getTree();
		//
		//		tree<Node>::iterator it = rootNode.begin();
		//		tree<Node>::iterator end = rootNode.end();
		//
		//		for (; it != end; ++it) {
		//			if (!it->isTag() && !it->isComment()) {
		//				tree<Node>::iterator parent = rootNode.parent(it);
		//				if (parent->isTag() && (parent->tagName() == "script"
		//						|| parent->tagName() == "SCRIPT")) {
		//					continue;
		//				}
		//
		//				string itText(it->text());
		//				string whitespaces(" \t\n\r,;|()/\{}");
		//				this->trim(itText, whitespaces);
		//
		//				if (itText.empty()) {
		//					continue;
		//
		//				} else {
		//					itText = string(it->text());
		//					trim(itText, whitespaces);
		//					cout << it->text() << endl;
		//				}
		//			}
		//		}

		doc.clear();
		++i;

//		if (i == 13851)
//			break;

		//		cout << i << endl;
	}

	return 0;
}
