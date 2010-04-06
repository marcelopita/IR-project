/*
 * Indexer.cpp
 *
 *  Created on: 23/03/2010
 *      Author: pita
 */

#include "../include/Indexer.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <iconv.h>
#include <locale.h>
#include <htmlcxx/html/ParserDom.h>
#include <htmlcxx/html/CharsetConverter.h>
#include <htmlcxx/html/utils.h>
#include <algorithm>

Indexer::Indexer(string collectionDirectory, string collectionIndexFileName) {
	this->collectionDirectory = collectionDirectory;
	this->collectionIndexFileName = collectionIndexFileName;
	this->whitespaces1 = " \t\n\r";
	this->whitespaces2 = " \t\n\r,;|()/\\{}";
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

void Indexer::getTerms(const string &text, vector<string> &terms) {
	//	string tempTerm;
	//
	//	vector<char> allowedChars;
	//	allowedChars.push_back()
	//
	//	for (int i = 0; i < text.size(); i++) {
	//
	//	}
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
	text.assign(htmlcxx::HTML::decode_entities(text));

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
		charset = "utf-8";

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

	// Transform all contents to ascii, if possible
	if (charset != "ascii" && charset != "ASCII" && charset != "") {
		char input[usefulText.size() + 1];
		memset(input, 0, usefulText.size() + 1);
		strncpy(input, usefulText.c_str(), usefulText.size());
		char *inBuf = input;

		char output[usefulText.size() * 2 + 1];
		char *outBuf = output;
		memset(output, 0, usefulText.size() * 2 + 1);

		size_t inputSize = usefulText.size();
		size_t outputSize = usefulText.size() * 2;

		setlocale(LC_ALL, "");

		iconv_t cd = iconv_open("ascii//translit", charset.c_str());
		if (cd != (iconv_t) (-1)) {
			iconv(cd, &inBuf, &inputSize, &outBuf, &outputSize);
			usefulText.assign(output);
		}

		iconv_close(cd);

		//		delete inBuf;
		//		delete outBuf;
	}

	return DOC_OK;
}

void Indexer::sortTriples() {
	ifstream triplesFile("tempIndex.tmp", ios::in | ios::binary);
	ofstream triplesFileSorted("tempIndexSorted.tmp", ios::out | ios::binary);

	// k for 100 MB blocks = 104857600 bytes
	long k = (long) 104857600 / (long) (2 * sizeof(long) + sizeof(unsigned));

	long lastPosition = 0;
	triplesFile.seekg(lastPosition);

	while (!triplesFile.eof()) {
		long bytesRead = 0;
		triplesFile.seekg(lastPosition);

		void ***triples;
		triples = new void**[k];

		for (long i = 0; i < (long)k; i++) {
			triples[i] = new void*[3];
			triples[i][0] = new long;
			triples[i][1] = new long;
			triples[i][2] = new unsigned;

			if (!triplesFile.eof()) {
//				triplesFile.read(triples[i][0], sizeof(long));
//				triplesFile.read(triples[i][1], sizeof(long));
//				triplesFile.read(triples[i][2], sizeof(unsigned));
				triplesFile >> triples[i][0] >> triples[i][1] >> triples[i][2];
				cout << triples[i][0] << ", " << triples[i][1] << ", " << triples[i][2];
				bytesRead += (2 * sizeof(long) + sizeof(unsigned));

			} else {
				break;
			}
		}

		// qsort

		triplesFileSorted.seekp(lastPosition);
		for (long i = 0; i < (long)(bytesRead / (2 * sizeof(long) + sizeof(unsigned))); i++) {
			triplesFileSorted << triples[i][0] << triples[i][1] << triples[i][2];
		}

		lastPosition += bytesRead;
	}
}

int Indexer::index() {
	ofstream tempFile("tempIndex.tmp");

	this->reader = new CollectionReader(this->collectionDirectory,
			this->collectionIndexFileName);

	Document doc;
	doc.clear();
	long i = 0;
	long numDocsInvalidHTTPHeader = 0;
	long numDocsInvalidContentType = 0;
	long numDocsUnknownCharSet = 0;

	while (reader->getNextDocument(doc)) {
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
		tokenize(usefulText, indexableTerms,
				" \t\r\n\"\'!@#&*()_+=`{[}]^~ç?/\\:><,.;ªº");

		for (unsigned j = 0; j < indexableTerms.size(); j++) {
			if (indexableTerms[j] == "-") {
				continue;
			}

			transform(indexableTerms[j].begin(), indexableTerms[j].end(),
					indexableTerms[j].begin(), (int(*)(int)) tolower);

			long termNumber;

			map<string, int>::iterator refTerm = vocabulary.find(
					indexableTerms[j]);
			if (refTerm == vocabulary.end()) {
				termNumber = vocabulary.size();
				vocabulary.insert(make_pair(indexableTerms[j], termNumber));

				//				cout << termNumber << ": " << indexableTerms[j] << endl;

			} else {
				termNumber = (*refTerm).second;
			}

			// TODO: Compress entry in temporary file
			tempFile << termNumber << i << j;
		}

		if (i == 1000) {
			break;
		}

		//		if (i % 5000 == 0) {
		//			cout << "===============" << endl;
		//			cout << i << endl;
		//			cout << "===============" << endl;
		//		}

		doc.clear();
		++i;
	}

	// Sorting (quicksort)
	sortTriples();

	cout << "\n-----------------------------------------------------------"
			<< endl;
	cout << "Invalid HTTP headers:\t" << numDocsInvalidHTTPHeader << endl;
	cout << "Invalid content types:\t" << numDocsInvalidContentType << endl;
	cout << "Unknown char set:\t" << numDocsUnknownCharSet << endl;
	cout << "Number of terms:\t" << vocabulary.size() << endl;

	return 0;
}
