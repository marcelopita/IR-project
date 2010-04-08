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
#include <errno.h>

struct TempFileTriple {
	int termNumber;
	int docNumber;
	int termPosition;
};

Indexer::Indexer(string collectionDirectory, string collectionIndexFileName,
		string tempFileName, string indexFileName) {
	this->collectionDirectory = collectionDirectory;
	this->collectionIndexFileName = collectionIndexFileName;
	this->tempFileName = tempFileName;
	this->indexFileName = indexFileName;
	this->whitespaces1 = " \t\n\r";
	this->whitespaces2 = " \t\n\r,;|()/\\{}";
}

Indexer::~Indexer() {
	delete this->reader;
}

void Indexer::trim(std::string &str, const std::string &whitespaces = " ") {
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
		const string& delimiters = " ", bool lowerCase = false) {
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos) {
		// Found a token, add it to the vector.
		if (!lowerCase) {
			tokens.push_back(str.substr(lastPos, pos - lastPos));
		} else {
			string subStr = str.substr(lastPos, pos - lastPos);
			transform(subStr.begin(), subStr.end(), subStr.begin(),
					(int(*)(int)) tolower);
			tokens.push_back(subStr);
		}
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
		int &numDocsInvalidHTTPHeader, int &numDocsInvalidContentType,
		int &numDocsUnknownCharSet) {
	// Full text
	string text = doc.getText();
	//	text.assign(htmlcxx::HTML::decode_entities(text));

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
		charset = "ISO8859-1";

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
	}

	return DOC_OK;
}

//void Indexer::sortTempFile() {
//	ifstream tempFile("tempFile.tmp");
//	ofstream tempFileSorted("temFileSorted.tmp");
//	tempFileSorted.clear();
//
//	while (!tempFile.eof()) {
//		triple aTriple;
//		if (tempFile.read((char*) &aTriple, sizeof(triple)) > 0) {
//			tempFileSorted.write((char*) &aTriple, sizeof(triple));
//		}
//	}
//
//	tempFile.close();
//	tempFileSorted.close();
//}

bool cmp(TempFileTriple t1, TempFileTriple t2) {
	if (t1.termNumber > t2.termNumber) {
		return true;

	} else if (t1.termNumber == t2.termNumber) {
		if (t1.docNumber > t2.docNumber) {
			return true;

		} else if (t1.docNumber == t2.docNumber) {
			if (t1.termPosition > t2.termPosition) {
				return true;
			}
		}
	}

	return false;
}

/*
 * Sort triples in temporary file.
 */
void Indexer::sortTriples() {
	fstream triplesFile("tempIndex.tmp", ios_base::in | ios_base::binary
			| ios_base::trunc);

	// k for 1 MB blocks = 104857600 bytes
	int pageSize = 1048576;
	int bytesPerTriple = sizeof(TempFileTriple);

	int k = pageSize / bytesPerTriple;

	int lastPosition = 0;
	triplesFile.seekg(lastPosition);

	while (!triplesFile.eof()) {
		int bytesRead = 0;
		triplesFile.seekg(lastPosition);

		vector<TempFileTriple> triples;

		for (int i = 0; i < k; i++) {
			if (triplesFile.eof())
				break;

			TempFileTriple aTriple;
			if (triplesFile.read((char*) &aTriple, sizeof(TempFileTriple)) > 0) {
				triples.push_back(aTriple);
			}

			bytesRead += bytesPerTriple;
		}

		// qsort
		sort(triples.begin(), triples.end(), cmp);

		triplesFile.seekp(lastPosition);
		for (int i = 0; i < (int) triples.size(); i++) {
			TempFileTriple aTriple = triples.at(i);
			triplesFile.write((char*) &aTriple, sizeof(TempFileTriple));
		}

		lastPosition += bytesRead;
	}

	triplesFile.close();
}

/*
 * Separate HTTP header and content.
 */
int Indexer::separateHeaderContent(string& fullText,
		string& contentTypeHeaderLine, string& content) {
	stringstream ss(fullText);

	string httpResponseCodeMsg;
	getline(ss, httpResponseCodeMsg);

	// Exclude documents without HTTP header or with returned status code different of 200, 203 or 206
	if (httpResponseCodeMsg.find("HTTP") == string::npos
			|| (httpResponseCodeMsg.find("200") == string::npos
					&& httpResponseCodeMsg.find("203") == string::npos
					&& httpResponseCodeMsg.find("206") == string::npos)) {
		return Indexer::DOC_BLOCKED;
	}

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

	getline(ss, content, (char) 26);

	return DOC_OK;
}

/**
 * Get content type from HTTP header.
 */
int Indexer::getContentTypeHeader(string& contentTypeHeaderLine,
		string& contentType) {
	// Allowed content types: text/html and text/plain
	// Future work: Treat PDF files
	if (contentTypeHeaderLine.find("text/html") == string::npos
			&& contentTypeHeaderLine.find("TEXT/HTML") == string::npos
			&& contentTypeHeaderLine.find("text/HTML") == string::npos
			&& contentTypeHeaderLine.find("text/plain") == string::npos
			&& contentTypeHeaderLine.find("TEXT/PLAIN") == string::npos
			&& contentTypeHeaderLine.find("text/PLAIN") == string::npos) {
		return DOC_BLOCKED;
	}

	contentType = "html";
	if (contentTypeHeaderLine.find("/plain") != string::npos
			|| contentTypeHeaderLine.find("/PLAIN") != string::npos) {
		contentType = "plain";
	}

	return DOC_OK;
}

void Indexer::getCharSetHeader(string& contentTypeHeaderLine,
		string& charsetHeader) {
	if (contentTypeHeaderLine.find("charset") == string::npos
			&& contentTypeHeaderLine.find("CHARSET") == string::npos
			&& contentTypeHeaderLine.find("Charset") == string::npos) {
		charsetHeader = "";
		return;
	}

	vector<string> contentTypeTokens;
	this->tokenize(contentTypeHeaderLine, contentTypeTokens, " ;=");

	for (unsigned i = 0; i < contentTypeTokens.size(); i++) {
		string token = contentTypeTokens.at(i);
		if ((token == "charset" || token == "CHARSET" || token == "Charset")
				&& (i < contentTypeTokens.size() - 1)) {
			charsetHeader = contentTypeTokens.at(i + 1);
			break;
		}
	}

	this->trim(charsetHeader, " \n\r\t,;|()/\\{}");
}

void Indexer::getCharSetMetaTag(string& htmlText, string& charsetMetaTag) {
	using namespace htmlcxx::HTML;

	ParserDom parser;
	parser.parse(htmlText);
	tree<Node> rootNode = parser.getTree();

	tree<Node>::iterator it = rootNode.begin();
	tree<Node>::iterator end = rootNode.end();

	for (; it != end; ++it) {
		if (it->isTag() && (it->tagName() == "meta" || it->tagName() == "META")) {
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

				this->getCharSetHeader(contentAttrEntry.second, charsetMetaTag);
			}
		}
	}
}

/**
 * Change char set of content.
 */
int Indexer::changeCharSet(const string& from, const string& to,
		string& content) {
	char *input = new char[content.size() + 1];
	memset(input, 0, content.size() + 1);
	strncpy(input, content.c_str(), content.size());
	char *inBuf = input;

	char *output = new char[content.size() * 2 + 1];
	memset(output, 0, content.size() * 2 + 1);
	char *outBuf = output;

	size_t inputSize = content.size();
	size_t outputSize = content.size() * 2;

	setlocale(LC_ALL, "");

	iconv_t cd = iconv_open(to.c_str(), from.c_str());

	if (cd == (iconv_t) (-1)) {
		delete[] input;
		delete[] output;
		return DOC_BLOCKED;
	}

	bool error = false;

	do {
		if (iconv(cd, &inBuf, &inputSize, &outBuf, &outputSize)
				== (size_t) (-1)) {
			error = true;
			inputSize--;
			inBuf++;

		} else {
			error = false;
		}

	} while (error);

	content.assign(output);

	iconv_close(cd);
	delete[] input;
	delete[] output;

	return DOC_OK;
}

void Indexer::extractUsefulContent(string& htmlText, string& usefulText) {
	using namespace htmlcxx::HTML;

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
			this->trim(itText, " \t\n\r,;|()/\\{}");
			if (itText.empty())
				continue;

			usefulText.append(it->text());
			usefulText.append(" ");
		}
	}
}

/**
 * Compress a buffer using the XXXX algorithm.
 */
void Indexer::compress(char* inBuffer, char* outBuffer) {
	// TODO: Implement XXXX algorithm
	outBuffer = inBuffer;
}

/**
 * Uncompress a buffer compressed in the XXXX algorithm.
 */
void Indexer::uncompress(char* inBuffer, char* outBuffer) {
	// TODO: Uncompress buffer
	outBuffer = inBuffer;
}

/**
 * Compress triples.
 */
void Indexer::compressTriple(char* triple, char* outBuffer) {
	this->compress(triple, outBuffer);
}

/**
 * Uncompress triples.
 */
void Indexer::uncompressTriple(char* inBuffer, char* triple) {
	this->uncompress(inBuffer, triple);
}

/**
 * Save triples <term,doc,pos> in temporary file.
 *
 */
void Indexer::saveTriplesTempFile(vector<string>& terms, int docNumber) {
	ofstream tempFile(this->tempFileName.c_str());
	tempFile.clear();

	for (int i = 0; i < (int) terms.size(); i++) {
		map<string, int>::iterator refTerm = vocabulary.find(terms[i]);

		int termNumber;
		if (refTerm == vocabulary.end()) {
			termNumber = vocabulary.size();
			vocabulary.insert(make_pair(terms[i], termNumber));

		} else {
			termNumber = refTerm->second;
		}

		TempFileTriple triple;
		triple.termNumber = termNumber;
		triple.docNumber = docNumber;
		triple.termPosition = i;

		tempFile.write((char*)&triple, sizeof(TempFileTriple));
	}

	tempFile.close();
}

/*
 * Index collection terms and documents.
 */
int Indexer::index() {
	this->reader = new CollectionReader(this->collectionDirectory,
			this->collectionIndexFileName);

	Document doc;
	doc.clear();
	int i = 0;
	int lastDocNumber = 0;
	int numDocsInvalidHTTPHeader = 0;
	int numDocsInvalidContentType = 0;
	int numDocsUnknownCharSet = 0;

	while (reader->getNextDocument(doc)) {
		string fullText = doc.getText();

		// SEPARATE HEADER AND CONTENT

		string contentTypeHeaderLine, content;

		if (separateHeaderContent(fullText, contentTypeHeaderLine, content)
				== DOC_BLOCKED) {
			numDocsInvalidHTTPHeader++;
			doc.clear();
			i++;
			continue;
		}

		// GET CONTENT TYPE FROM HEADER

		string contentType;

		if (getContentTypeHeader(contentTypeHeaderLine, contentType)
				== DOC_BLOCKED) {
			numDocsInvalidContentType++;
			doc.clear();
			i++;
			continue;
		}

		// GET CHAR SET FROM HEADER OR HTML META TAG

		string charset, charsetHeader, charsetMetaTag;

		getCharSetHeader(contentTypeHeaderLine, charsetHeader);

		if (contentType == "html") {
			string iso88591Content = htmlcxx::HTML::decode_entities(content);
			getCharSetMetaTag(iso88591Content, charsetMetaTag);

		} else {
			charsetMetaTag = "";
		}

		if (charsetMetaTag != "") {
			charset = charsetMetaTag;

		} else if (charsetHeader != "") {
			charset = charsetHeader;

		} else {
			charset = "ISO-8859-1";
		}

		// CHANGE HTML CONTENT CHAR SET TO ISO8859-1 AND DECODE HTML ENTITIES

		if (contentType == "html") {
			if (changeCharSet(charset, "ISO-8859-1", content) == DOC_BLOCKED) {
				numDocsUnknownCharSet++;
				doc.clear();
				i++;
				continue;
			}

			content = htmlcxx::HTML::decode_entities(content);
			charset = "ISO8859-1";
		}

		// CHANGE CONTENT CHAR SET TO ASCII USING TRANSLITERATION

		if (changeCharSet(charset, "ASCII//TRANSLIT", content) == DOC_BLOCKED) {
			numDocsUnknownCharSet++;
			doc.clear();
			i++;
			continue;
		}

		// EXTRACT USEFUL CONTENT

		string usefulContent;

		if (contentType == "plain") {
			usefulContent = content;

		} else {
			extractUsefulContent(content, usefulContent);
		}

		// REGISTER DOCUMENT URL AND GET ITS NUMBER

		this->documents.push_back(doc.getURL());
		int docNumber = lastDocNumber++;

		// GET INDEXABLE TERMS IN LOWER CASE

		vector<string> indexableTerms;
		tokenize(usefulContent, indexableTerms,
				" \t\r\n\"\'!@#&*()_+=`{[}]^~ç?/\\:><,.;ªº", true);

//				/*********** TEMP ***********/
//				cout << "\n=== Documento " << i << " =================" << endl;
//				for (int j = 0; j < (int) indexableTerms.size(); j++)
//					cout << j << ":\t" << indexableTerms.at(j) << endl;
//
//				doc.clear();
//				i++;
//				if (i == 50) {
//					break;
//				}
//				continue;
//				/****************************/

		// SAVE TRIPLES IN A COMPRESSED TEMPORARY FILE

		saveTriplesTempFile(indexableTerms, docNumber);

		// SORT THE TEMPORARY FILE

		// SAVE COMPRESSED INDEX

		doc.clear();
		i++;

		if (i >= 50) {
			break;
		}
	}

	cout << "\n#############################" << endl;
	cout << "Invalid HTTP header:\t" << numDocsInvalidHTTPHeader << endl;
	cout << "Invalid content type:\t" << numDocsInvalidContentType << endl;
	cout << "Unknown char set:\t" << numDocsUnknownCharSet << endl;
	cout << "Useful documents:\t" << lastDocNumber << endl;

	return 0;
}

//int Indexer::index() {
//	ofstream tempFile("tempFile.tmp");
//	tempFile.clear();
//
//	this->reader = new CollectionReader(this->collectionDirectory,
//			this->collectionIndexFileName);
//
//	Document doc;
//	doc.clear();
//	int i = 0;
//	int numDocsInvalidHTTPHeader = 0;
//	int numDocsInvalidContentType = 0;
//	int numDocsUnknownCharSet = 0;
//
//	while (reader->getNextDocument(doc)) {
//		// Index document URL
//		string url = doc.getURL();
//		this->documents.push_back(url);
//
//		string usefulText;
//
//		if (this->preprocessDocument(doc, usefulText, numDocsInvalidHTTPHeader,
//				numDocsInvalidContentType, numDocsUnknownCharSet)
//				== DOC_BLOCKED) {
//			continue;
//		}
//
//		cout << "========================" << endl;
//		cout << usefulText << endl;
//		cout << "========================" << endl;
//
//		vector<string> indexableTerms;
//		tokenize(usefulText, indexableTerms,
//				" \t\r\n\"\'!@#&*()_+=`{[}]^~ç?/\\:><,.;ªº");
//
//		for (int j = 0; j < (int) indexableTerms.size(); j++) {
////			if (indexableTerms[j] == "-") {
////				continue;
////			}
//
//			transform(indexableTerms[j].begin(), indexableTerms[j].end(),
//					indexableTerms[j].begin(), (int(*)(int)) tolower);
//
//			int termNumber;
//
//			map<string, int>::iterator refTerm = vocabulary.find(
//					indexableTerms[j]);
//			if (refTerm == vocabulary.end()) {
//				termNumber = vocabulary.size();
//				vocabulary.insert(make_pair(indexableTerms[j], termNumber));
//
//				cout << termNumber << ": " << indexableTerms[j] << endl;
//
//			} else {
//				termNumber = (*refTerm).second;
//			}
//
//			triple aTriple;
//			aTriple.termNumber = termNumber;
//			aTriple.docNumber = i;
//			aTriple.termPosition = j;
//
//			// TODO: Compress entry in temporary file
//			tempFile.write((char*) &aTriple, sizeof(triple));
//		}
//
//		if (i == 0) {
//			break;
//		}
//
//		//		if (i % 5000 == 0) {
//		//			cout << "===============" << endl;
//		//			cout << i << endl;
//		//			cout << "===============" << endl;
//		//		}
//
//		doc.clear();
//		++i;
//	}
//
//	tempFile.close();
//
//	// Sorting (quicksort)
//	//	sortTempFile();
//	//	sortTriples();
//
//	cout << "\n-----------------------------------------------------------"
//			<< endl;
//	cout << "Invalid HTTP headers:\t" << numDocsInvalidHTTPHeader << endl;
//	cout << "Invalid content types:\t" << numDocsInvalidContentType << endl;
//	cout << "Unknown char set:\t" << numDocsUnknownCharSet << endl;
//	cout << "Number of terms:\t" << vocabulary.size() << endl;
//
//	return 0;
//}
