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
#include <cstdio>

struct TempFileTriple {
	int termNumber;
	int docNumber;
	int termPosition;
};

struct SortedRun {
	int id;
	int begin;
	int end;
};

struct RunTriple {
	int runId;
	TempFileTriple triple;
};

Indexer::Indexer(string collectionDirectory, string collectionIndexFileName,
		string tempFileName, string indexFileName, int runSize) {
	this->collectionDirectory = collectionDirectory;
	this->collectionIndexFileName = collectionIndexFileName;
	this->tempFileName = tempFileName;
	this->finalTempFileName = "final";
	this->finalTempFileName.append(tempFileName);
	this->indexFileName = indexFileName;
	this->runSize = runSize;
	this->k = runSize / sizeof(TempFileTriple);
	this->whitespaces1 = " \t\n\r";
	this->whitespaces2 = " \t\n\r,;|()/\\{}";
	this->numTriplesSaved = 0;
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
 * Inform if t1 is lesser than t2.
 */
bool lesserTriple(TempFileTriple t1, TempFileTriple t2) {
	if (t1.termNumber < t2.termNumber) {
		return true;

	} else if (t1.termNumber == t2.termNumber) {
		if (t1.docNumber < t2.docNumber) {
			return true;

		} else if (t1.docNumber == t2.docNumber) {
			if (t1.termPosition < t2.termPosition) {
				return true;
			}
		}
	}

	return false;
}

/**
 * Inform if t1 is greater than t2.
 */
bool greaterTriple(TempFileTriple t1, TempFileTriple t2) {
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

/**
 * Inform if t1 is greater than t2.
 */
bool greaterRunTriple(RunTriple t1, RunTriple t2) {
	return greaterTriple(t1, t2);
}

/**
 * Print triple.
 */
void printTriple(TempFileTriple& t) {
	cout << "<" << t.termNumber << ", " << t.docNumber << ", "
			<< t.termPosition << ">" << endl;
}

void printSortedRun(SortedRun& r) {
	cout << r.id << ":\t" << r.begin << " -> " << r.end << endl;
}

/**
 * Save triples' vector in temporary file.
 */
void Indexer::saveTriplesVectorTempFile() {
	vector<TempFileTriple>::iterator it = this->kTriples.begin();
	vector<TempFileTriple>::iterator end = this->kTriples.end();
	//	for (; it != end; it++) {
	//		printTriple(*it);
	//	}

	ofstream tempFile(this->tempFileName.c_str(), ios_base::app);
	for (int j = 0; j < (int) this->kTriples.size(); j++) {
		// TODO: Compress temporary file
		tempFile.write((char*) &(this->kTriples.at(j)), sizeof(TempFileTriple));
	}
	tempFile.close();

	SortedRun run;
	run.id = this->sortedRuns.size();
	run.begin = this->numTriplesSaved;
	run.end = this->numTriplesSaved + this->kTriples.size() - 1;
	this->numTriplesSaved += this->kTriples.size();
	this->sortedRuns.push_back(run);
	printSortedRun(run);
}

/**
 * Save triples <term,doc,pos> in temporary file.
 *
 */
void Indexer::saveTriplesTempFile(vector<string>& terms, int docNumber) {
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

		this->kTriples.push_back(triple);

		if ((int) this->kTriples.size() == this->k) {
			sort(this->kTriples.begin(), this->kTriples.end(), lesserTriple);
			saveTriplesVectorTempFile();
			this->kTriples.clear();
		}
	}
}

/**
 * Merge sorted runs.
 */
void Indexer::mergeSortedRuns() {
	int heapSize = this->sortedRuns.size();
	int tripleSize = sizeof(TempFileTriple);
	vector<RunTriple> heap;

	ifstream tempFile(this->tempFileName.c_str());
	ofstream tempFileFinal(this->finalTempFileName.c_str(), ios_base::app);

	// Preencher heap incialmente

	for (int i = 0; i < k; i++) {
		for (int j = 0; j < heapSize; j++) {
			SortedRun run = this->sortedRuns[j];

			int readPosition = tripleSize * run.begin + tripleSize * i;
			if (readPosition > tripleSize * run.end)
				continue;

			TempFileTriple triple;

			tempFile.seekg(readPosition);
			if (tempFile.read((char*)&triple, sizeof(TempFileTriple) <= 0))
				continue;

			RunTriple runTriple;
			runTriple.runId = run.id;
			runTriple.triple = triple;

			heap.push_back(runTriple);
			push_heap(heap.begin(), heap.end(), greaterRunTriple);
		}
	}

//	for (int i = 0; i < heapSize; i++) {
//
//		// Pegar 1 tripla da run i e dar push na heap
////		tempFile.seek
//	}

	tempFile.close();
	tempFileFinal.close();
}

/*
 * Index collection terms and documents.
 */
int Indexer::index() {
	this->reader = new CollectionReader(this->collectionDirectory,
			this->collectionIndexFileName);

	remove(this->tempFileName.c_str());

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
				" \t\r\n\"\'!@#&*()_+-|=`{[}]^~ç?/\\:><,.;ªº", true);

		// SAVE SORTED TRIPLES IN A COMPRESSED TEMPORARY FILE

		saveTriplesTempFile(indexableTerms, docNumber);

		doc.clear();
		i++;

		//		if ((docNumber + 1) % 2000 == 0)
		//			cout << i << endl;

		//		if (docNumber >= 10) {
		//			break;
		//		}
	}

	// SAVE REMAINING TRIPLES IN THE COMPRESSED TEMPORARY FILE

	if ((int) this->kTriples.size() > 0) {
		sort(this->kTriples.begin(), this->kTriples.end(), lesserTriple);
		saveTriplesVectorTempFile();
		this->kTriples.clear();
	}

	// MERGE SORTED RUNS

	mergeSortedRuns();

	// SAVE COMPRESSED INVERTED FILE

	// PRINT USEFUL INFORMATION

	cout << "\n#############################" << endl;
	cout << "Invalid HTTP header:\t" << numDocsInvalidHTTPHeader << endl;
	cout << "Invalid content type:\t" << numDocsInvalidContentType << endl;
	cout << "Unknown char set:\t" << numDocsUnknownCharSet << endl;
	cout << "Useful documents:\t" << lastDocNumber << endl;
	cout << "Number of terms:\t" << vocabulary.size() << endl;

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
