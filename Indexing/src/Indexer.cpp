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
	if(pos != std::string::npos) {
		str.erase(pos + 1);
		pos = str.find_first_not_of(whitespaces);
		if(pos != std::string::npos) str.erase(0, pos);
	}
	else str.erase(str.begin(), str.end());
}

int Indexer::index() {
	fstream tempFile("/tmp/tempIndex", ios_base::app);

	this->reader = new CollectionReader(this->collectionDirectory,
			this->collectionIndexFileName);

	Document doc;
	doc.clear();
	int i = 0;
	int numExcludedDocs = 0;

	while (reader->getNextDocument(doc)) {
		// Index document URL
		string url = doc.getURL();
		this->documents.push_back(url);

		// Full text
		string text = doc.getText();

		// Get HTTP return status code
		stringstream ss(text);
		string httpResponseCodeMsg;
		getline(ss, httpResponseCodeMsg);

		// Exclude documents without HTTP header or with returned status code different of 200, 203 or 206
		if (httpResponseCodeMsg.find("200") == string::npos
				&& httpResponseCodeMsg.find("203") == string::npos
				&& httpResponseCodeMsg.find("206") == string::npos) {
			doc.clear();
			++i;
			++numExcludedDocs;
			continue;
		}

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

		string htmlText;
		getline(ss, htmlText, (char) 26);

		string usefulText;

		using namespace htmlcxx::HTML;

		ParserDom parser;
		parser.parse(htmlText);
		tree<Node> rootNode = parser.getTree();

		tree<Node>::iterator it = rootNode.begin();
		tree<Node>::iterator end = rootNode.end();

		for (; it != end; ++it) {
			if (!it->isTag() && !it->isComment()) {
				tree<Node>::iterator parent = rootNode.parent(it);
				if (parent->isTag() && (parent->tagName() == "script" || parent->tagName() == "SCRIPT")) {
					continue;
				}

				string itText(it->text());
				string whitespaces(" \t\n\r,;|()/\{}");
				this->trim(itText, whitespaces);

				if (itText.empty()) {
					continue;

				} else {
					itText = string(it->text());
					trim(itText, whitespaces);
					cout << it->text() << endl;
				}
			}

//			rootNode.parent(it);

//			remove(itText.begin(), itText.end(), ' ');
//			remove(itText.begin(), itText.end(), '\t');
//
//			if (itText != "\n")
//				continue;
//
//			cout << "Texto: " << it->text() << endl;
//
//			if (!it->isComment()) {
//				if (!it->isTag()) {
//					usefulText.append(" ");
//					usefulText.append(it->text());
//
//				} else if (it->tagName() == "meta" || it->tagName() == "META") {
//					cout << it->text() << endl;
//				}
//			}
		}

		//		if (httpResponseCodeMsg.find("200") != string::npos
		//				|| httpResponseCodeMsg.find("203") != string::npos
		//				|| httpResponseCodeMsg.find("206") != string::npos) {
		//			string htmlText;
		//			getline(ss, htmlText, (char) 26);
		//
		//			using namespace htmlcxx::HTML;
		//
		//			ParserDom parser;
		//			tree<Node> dom = parser.parseTree(htmlText);
		//
		//			tree<Node>::iterator it = dom.begin();
		//			tree<Node>::iterator end = dom.end();
		//
		//			Node n;
		////			n.
		//			tree<Node> tn;
		////			tn.
		//
		//			for (; it != end; ++it) {
		//				if (it->tagName() == "p" || it->tagName() == "P") {
		//					tree<Node> pNode(*it);
		//
		//					tree<Node>::iterator pIterBegin = pNode.begin();
		//					tree<Node>::iterator pIterEnd = pNode.end();
		//					for (; pIterBegin != pIterEnd; ++pIterBegin) {
		////						if (!(pIterBegin->isTag()) && !(pIterBegin->isComment())) {
		//							cout << pIterBegin->text() << endl;
		////						}
		//					}
		//
		////					cout << pNode.depth(it) << endl;
		//				}
		////				if ((!it->isTag()) && (!it->isComment()) && (it->parent(0)->tagName() == "p")) {
		////					cout << it->text();
		////				}
		//
		////				if (it->tagName() == "p" || it->tagName()=="P") {
		////						cout << it->closingText() << endl;
		////				  		it->parseAttributes();
		////						cout << it->attribute("href").second << endl;
		////				}
		//
		////				cout << it->text() << endl;
		////				if ((!it->isTag()) && (!it->isComment())) {
		////					cout << it->text() << endl;
		////					string tagName = it->tagName();
		////
		////					if (tagName == "p") {
		////						//						it->parseAttributes();
		////						string pureText;
		////						(*it) >> pureText;
		////						cout << pureText;
		////						cout << it->text() << endl;
		////					}
		////				}
		//			}
		//		} else {
		//			numExcludedDocs++;
		//			cout << numExcludedDocs << endl;
		//		}

		doc.clear();
		++i;

		//		if (i % 20000 == 0)
		//			cout << '\t' << i << endl;

		if (i == 1)
			break;
	}

	return 0;
}
