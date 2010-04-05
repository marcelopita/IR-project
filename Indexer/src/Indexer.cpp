/*
 * Indexer.cpp
 *
 *  Created on: 23/03/2010
 *      Author: pita
 */

#include "../include/Indexer.h"
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <htmlcxx/html/ParserDom.h>
#include <htmlcxx/html/CharsetConverter.h>

Indexer::Indexer(string collectionDirectory, string collectionIndexFileName) {
	this->collectionDirectory = collectionDirectory;
	this->collectionIndexFileName = collectionIndexFileName;
	this->whitespaces1 = " \t\n\r";
	this->whitespaces2 = " \t\n\r,;|()/\{}";
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

void Indexer::tokenize(const string& str, vector<string>& tokens,
		const string& delimiters = " ") {
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos) {
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
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

int Indexer::preprocessDocument(Document &doc, string &usefulText,
		long &numDocsInvalidHTTPHeader, long &numDocsInvalidContentType,
		long &numDocsUnknownCharSet) {
	// Full text
	string text = doc.getText();

	// Filter 1: Remove docs with invalid HTTP header
	if (this->filterInvalidHTTPHeader(text) == Indexer::DOC_BLOCKED) {
		++numDocsInvalidHTTPHeader;

		return DOC_BLOCKED;
	}

	// Content type field
	string contentTypeHeaderLine;

	stringstream ss(text);
	string tempHeaderLine;
	bool endHeader = false;

	// Extract some HTTP header fields (content type) and detect its end.
	while (!endHeader) {
		getline(ss, tempHeaderLine);
		string headerLineNoSpaces(tempHeaderLine);
		trim(headerLineNoSpaces, whitespaces1);

		if (headerLineNoSpaces != "") {
			//				headerText.append(tempHeaderLine);
			if (tempHeaderLine.find("Content-Type:") != string::npos
					|| tempHeaderLine.find("Content-type:") != string::npos) {
				contentTypeHeaderLine.assign(tempHeaderLine);
			}

		} else {
			endHeader = true;
		}
	}

	// Allowed content types: text/html and text/plain
	// Future work: Treat PDF files
	if (contentTypeHeaderLine.find("text/html") == string::npos
			&& contentTypeHeaderLine.find("TEXT/HTML") == string::npos
			&& contentTypeHeaderLine.find("text/HTML") == string::npos
			&& contentTypeHeaderLine.find("text/plain") == string::npos
			&& contentTypeHeaderLine.find("TEXT/PLAIN") == string::npos
			&& contentTypeHeaderLine.find("text/PLAIN") == string::npos) {
		++numDocsInvalidContentType;

		return DOC_BLOCKED;
	}

	string contentType = "html";
	if (contentTypeHeaderLine.find("/plain") != string::npos
			|| contentTypeHeaderLine.find("/PLAIN") != string::npos) {
		contentType = "plain";
	}

	if (contentType == "html") {
		using namespace htmlcxx::HTML;

		string htmlText;
		getline(ss, htmlText, (char) 26);

		ParserDom parser;
		parser.parse(htmlText);
		tree<Node> rootNode = parser.getTree();

		tree<Node>::iterator it = rootNode.begin();
		tree<Node>::iterator end = rootNode.end();

		for (; it != end; ++it) {
			if (!it->isTag() && !it->isComment()) {
				tree<Node>::iterator parent = rootNode.parent(it);
				if (parent->isTag() && (parent->tagName() == "script"
						|| parent->tagName() == "SCRIPT" || parent->tagName()
						== "STYLE" || parent->tagName() == "style")) {
					continue;
				}

				string itText(it->text());
				this->trim(itText, whitespaces2);

				if (itText.empty()) {
					continue;

				} else {
					//						itext = string(it->text());
					//						trim(itText, whitespaces);
					//						cout << it->text() << endl;
					usefulText.append(it->text());
					usefulText.append(" ");
				}

			} else if (it->tagName() == "meta" || it->tagName() == "META") {
				it->parseAttributes();

				pair<bool, string> httpEquivAttrEntry;

				// If META tag has HTTP-EQUIV attribute
				if ((it->attribute("HTTP-EQUIV")).first) {
					httpEquivAttrEntry = it->attribute("HTTP-EQUIV");

				} else if ((it->attribute("http-equiv")).first) {
					httpEquivAttrEntry = it->attribute("http-equiv");

				} else {
					continue;
				}

				// If HTTP-EQUIV attribute has value 'Content-Type'
				trim(httpEquivAttrEntry.second, whitespaces1);
				if (httpEquivAttrEntry.second == "Content-Type"
						|| httpEquivAttrEntry.second == "Content-type") {
					pair<bool, string> contentAttrEntry;

					// If META tag has CONTENT attribute
					if ((it->attribute("CONTENT")).first) {
						contentAttrEntry = it->attribute("CONTENT");

					} else if ((it->attribute("content")).first) {
						contentAttrEntry = it->attribute("content");

					} else if ((it->attribute("Content")).first) {
						contentAttrEntry = it->attribute("Content");

					} else {
						continue;
					}

					contentTypeHeaderLine = contentAttrEntry.second;
				}
			}
		}

	} else {
		// Read till EOF
		getline(ss, usefulText, (char) 26);
	}

	string charset;

	if (contentTypeHeaderLine.find("charset") == string::npos
			&& contentTypeHeaderLine.find("CHARSET") == string::npos
			&& contentTypeHeaderLine.find("Charset") == string::npos) {
		++numDocsUnknownCharSet;
		charset = "";

	} else {
		vector<string> contentTypeTokens;
		this->tokenize(contentTypeHeaderLine, contentTypeTokens, " ;=<>");


		unsigned j;
		for (j = 0; j < contentTypeTokens.size(); j++) {
			string token = contentTypeTokens.at(j);
			if (token == "charset" || token == "CHARSET" || token == "Charset") {
				if (j < contentTypeTokens.size() - 1) {
					charset = contentTypeTokens.at(j + 1);
					break;
				}
			}
		}
	}

	this->trim(charset, whitespaces2);

	using namespace htmlcxx;

	// Transform all contents to utf-8 if possible
	if (charset != "utf-8" && charset != "UTF-8" && charset != "") {
		try {
			CharsetConverter converter(charset, "UTF-8");
			usefulText.assign(converter.convert(usefulText));

		} catch (CharsetConverter::Exception) {
			numDocsUnknownCharSet++;
		}
	}

	// Remove tabs and carriage returns
	for (unsigned k = 0; k < usefulText.size(); k++) {
		if (usefulText[k] == '\r' || usefulText[k] == '\n' || usefulText[k]
				== '\t') {
			usefulText[k] = ' ';
		}
	}

	return DOC_OK;
}

int Indexer::index() {
	fstream tempFile("/tmp/tempIndex", ios_base::app);

	this->reader = new CollectionReader(this->collectionDirectory,
			this->collectionIndexFileName);

	Document doc;
	doc.clear();
	long i = 0;
	long numDocsInvalidHTTPHeader = 0;
	long numDocsInvalidContentType = 0;
	long numDocsUnknownCharSet = 0;

	while (reader->getNextDocument(doc)) {
		if (i % 10000 == 0)
			cout << i << endl;

		// Index document URL
		string url = doc.getURL();
		this->documents.push_back(url);

		string usefulText;

		if (this->preprocessDocument(doc, usefulText, numDocsInvalidHTTPHeader,
				numDocsInvalidContentType, numDocsUnknownCharSet)
				== DOC_BLOCKED) {
			continue;
		}

		vector<string> indexableTerms;
		tokenize(usefulText, indexableTerms, " \t,;:!?()[]{}|");

		for (unsigned j = 0; j < indexableTerms.size(); j++) {
			int termNumber;

			map<string, int>::iterator refTerm = vocabulary.find(
					indexableTerms[j]);
			if (refTerm == vocabulary.end()) {
				termNumber = vocabulary.size();
				vocabulary.insert(make_pair(indexableTerms[j], termNumber));

			} else {
				termNumber = (*refTerm).second;

				//				cout << termNumber << endl;
			}

			// TODO: Compress entry in temporary file
			tempFile << termNumber << i << j;
		}

		//		vector<string>::iterator termsIter = indexableTerms.begin();
		//		for (; termsIter != indexableTerms.end(); termsIter++) {
		//			for (unsigned j = 0; j < indexableTerms.size(); j++) {
		//				if (*termsIter == inde)
		//			}
		//		}

		doc.clear();
		++i;

		//		if (i % 1000 == 0) {
		//			cout << i << endl;
		//		}

//		if (i == 7266)
//			break;
	}

	// Sorting (quicksort)
	// k for 100 MB blocks
	int k = (int) 104857600 / (int) (16 * sizeof(int) + 8 * sizeof(unsigned));

	cout << "\n-----------------------------------------------------------"
			<< endl;
	cout << "Invalid HTTP headers:\t" << numDocsInvalidHTTPHeader << endl;
	cout << "Invalid content types:\t" << numDocsInvalidContentType << endl;
	cout << "Unknown char set:\t" << numDocsUnknownCharSet << endl;
	cout << "Number of terms:\t" << vocabulary.size() << endl;

	return 0;
}
