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
#include <sys/stat.h>
#include <sys/types.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/resource.h>

struct Triple {
	int termNumber;
	int docNumber;
	int termPosition;
};

struct RunTriple {
	int runId;
	Triple triple;
};

Indexer::Indexer(string collectionDirectory, string collectionIndexFileName,
		string tempFileName, string indexFileName, int runSize) {
	this->collectionDirectory = collectionDirectory;
	this->collectionIndexFileName = collectionIndexFileName;
	this->tempFilePrefix = tempFileName;
	this->tempDir = "tmp/";
	this->finalTempFileName = "final";
	this->finalTempFileName.append(tempFileName);
	this->indexFileNamePrefix = indexFileName;
	this->runSize = runSize;
	this->k = runSize / sizeof(Triple);
	//	this->kTriples.reserve(this->k);
	this->numTriplesSaved = 0;
	this->numRuns = 0;
	this->lastDocumentEntry = 0;
	this->lastPositionEntry = 0;
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

void Indexer::tokenize2(const string&str, vector<string>& tokens,
		bool lowerCase = false) {
	string token;
	for (int i = 0; i < (int) str.size(); i++) {
		if (isalnum(str[i])) {
			if (lowerCase) {
				token.append(1, tolower(str[i]));
			} else {
				token.append(1, str[i]);
			}
		} else {
			tokens.push_back(token);
			token.clear();
		}
	}
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
		trim(headerLineNoSpaces, " \t\n\r");

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
			trim(httpEquivAttrEntry.second, " \t\n\r");
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
 * Change content's char set.
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
			if (errno == EILSEQ) {
				error = true;
				inputSize--;
				inBuf++;
			} else {
				error = false;
			}

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

/**
 * Extract useful content from HTML text.
 *
 */
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
 * Inform if t1 is lesser than t2.
 */
bool lesserTriple(Triple t1, Triple t2) {
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
bool greaterTriple(Triple t1, Triple t2) {
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
	return greaterTriple(t1.triple, t2.triple);
}

/**
 * Print triple.
 */
void printTriple(Triple& t) {
	cout << "<" << t.termNumber << ", " << t.docNumber << ", "
			<< t.termPosition << ">" << endl;
}

/**
 * Save triples' vector in temporary file.
 */
void Indexer::saveTriplesVectorTempFile() {
	ostringstream tempFileNameStream;
	tempFileNameStream << this->tempDir << this->tempFilePrefix << numRuns
			<< ".tmp";
	ofstream tempFile(tempFileNameStream.str().c_str(), ios_base::app);

	for (int j = 0; j < (int) this->kTriples.size(); j++) {
		// TODO: Compress temporary file
		Triple triple = this->kTriples[j];

		tempFile.write((char*) &(triple.termNumber), sizeof(int));
		tempFile.write((char*) &(triple.docNumber), sizeof(int));
		tempFile.write((char*) &(triple.termPosition), sizeof(int));
	}
	tempFile.close();

	this->numRuns++;
	this->numTriplesSaved += this->kTriples.size();
}

void Indexer::saveTripleTempFile(string&term, int&docNumber, int&termPosition) {
	map<string, int>::iterator refTerm = vocabulary.find(term);

	int termNumber;
	if (refTerm == vocabulary.end()) {
		termNumber = vocabulary.size();
		vocabulary.insert(make_pair(term, termNumber));

	} else {
		termNumber = refTerm->second;
	}

	Triple triple;
	triple.termNumber = termNumber;
	triple.docNumber = docNumber;
	triple.termPosition = termPosition;

	this->kTriples.push_back(triple);

	if ((int) this->kTriples.size() == this->k) {
		sort(this->kTriples.begin(), this->kTriples.end(), lesserTriple);
		saveTriplesVectorTempFile();
		this->kTriples.clear();
	}
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

		Triple triple;
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
 * Write term entry in the inverted file.
 */
void Indexer::writeInvertedFile() {
	vector<vector<Triple> > triplesPerDocs;
	vector<Triple> tpd;
	vector<Triple>::iterator triplesPerTermIter;
	for (triplesPerTermIter = triplesPerTerm.begin(); triplesPerTermIter
			!= triplesPerTerm.end(); triplesPerTermIter++) {
		if (!tpd.empty() && (triplesPerTermIter->docNumber != tpd[0].docNumber)) {
			triplesPerDocs.push_back(tpd);
			tpd.clear();
		}

		tpd.push_back(*triplesPerTermIter);
	}
	if (!tpd.empty()) {
		triplesPerDocs.push_back(tpd);
		tpd.clear();
	}

	ostringstream termsFileNameStream;
	termsFileNameStream << this->indexFileNamePrefix << "_terms";
	ofstream termsFile(termsFileNameStream.str().c_str(), ios_base::app);

	ostringstream docsFileNameStream;
	docsFileNameStream << this->indexFileNamePrefix << "_docs";
	ofstream docsFile(docsFileNameStream.str().c_str(), ios_base::app);

	ostringstream posFileNameStream;
	posFileNameStream << this->indexFileNamePrefix << "_pos";
	ofstream posFile(posFileNameStream.str().c_str(), ios_base::app);

	int termNumber = this->triplesPerTerm[0].termNumber;
	int termFrequency = triplesPerDocs.size();

	termsFile.write((char*) &termNumber, sizeof(int));
	termsFile.write((char*) &termFrequency, sizeof(int));
	termsFile.write((char*) &lastDocumentEntry, sizeof(int));

	vector<vector<Triple> >::iterator triplesPerDocsIter;
	for (triplesPerDocsIter = triplesPerDocs.begin(); triplesPerDocsIter
			!= triplesPerDocs.end(); triplesPerDocsIter++) {
		lastDocumentEntry++;

		docsFile.write((char*) &(triplesPerDocsIter->at(0).docNumber),
				sizeof(int));
		int termFreqDoc = triplesPerDocsIter->size();
		docsFile.write((char*) &(termFreqDoc), sizeof(int));
		docsFile.write((char*) &lastPositionEntry, sizeof(int));

		vector<Triple>::iterator tpdIter;
		for (tpdIter = triplesPerDocsIter->begin(); tpdIter
				!= triplesPerDocsIter->end(); tpdIter++) {
			lastPositionEntry++;

			posFile.write((char*) &(tpdIter->termPosition), sizeof(int));
		}
	}

	termsFile.close();
	docsFile.close();
	posFile.close();
}

/**
 * Merge sorted runs.
 */
void Indexer::mergeSortedRuns() {
	vector<RunTriple> heap;
	vector<ifstream*> tempFiles;

	for (int i = 0; i < this->numRuns; i++) {
		ostringstream tempFileNameStream;
		tempFileNameStream << this->tempDir << this->tempFilePrefix << i
				<< ".tmp";
		tempFiles.push_back(new ifstream(tempFileNameStream.str().c_str()));
	}

	// INITIAL HEAP POPULATION
	for (int i = 0; i < this->numRuns; i++) {
		RunTriple runTriple;

		if (tempFiles[i]->eof())
			continue;

		Triple triple;

		// READ TERM NUMBER
		if (tempFiles[i]->read((char*) &(triple.termNumber), sizeof(int)) <= 0) {
			continue;
		}

		// READ DOC NUMBER
		if (tempFiles[i]->read((char*) &(triple.docNumber), sizeof(int)) <= 0) {
			continue;
		}

		// READ TERM POSITION
		if (tempFiles[i]->read((char*) &(triple.termPosition), sizeof(int))
				<= 0) {
			continue;
		}

		runTriple.triple = triple;

		runTriple.runId = i;
		heap.push_back(runTriple);
		push_heap(heap.begin(), heap.end(), greaterRunTriple);
	}

	// POP AND PUSH HEAP
	while (!heap.empty()) {
		// POP
		RunTriple popRunTriple = heap.front();
		pop_heap(heap.begin(), heap.end(), greaterRunTriple);
		heap.pop_back();

		if (!this->triplesPerTerm.empty()
				&& (this->triplesPerTerm[0].termNumber
						!= popRunTriple.triple.termNumber)) {
			writeInvertedFile();
			this->triplesPerTerm.clear();
		}

		this->triplesPerTerm.push_back(popRunTriple.triple);

		// PUSH
		RunTriple runTriple;

		if (tempFiles[popRunTriple.runId]->eof())
			continue;

		Triple triple;

		// READ TERM NUMBER
		if (tempFiles[popRunTriple.runId]->read((char*) &(triple.termNumber),
				sizeof(int)) <= 0) {
			continue;
		}

		// READ DOC NUMBER
		if (tempFiles[popRunTriple.runId]->read((char*) &(triple.docNumber),
				sizeof(int)) <= 0) {
			continue;
		}

		// READ TERM POSITION
		if (tempFiles[popRunTriple.runId]->read((char*) &(triple.termPosition),
				sizeof(int)) <= 0) {
			continue;
		}

		runTriple.triple = triple;

		runTriple.runId = popRunTriple.runId;
		heap.push_back(runTriple);
		push_heap(heap.begin(), heap.end(), greaterRunTriple);
	}

	// WRITE REMAINING TRIPLES IN INVERTED FILE
	if (!this->triplesPerTerm.empty()) {
		writeInvertedFile();
		this->triplesPerTerm.clear();
	}

	for (int i = 0; i < (int) this->numRuns; i++) {
		tempFiles[i]->close();
		delete tempFiles[i];
		//		ostringstream tempFileNameStream;
		//		tempFileNameStream << this->tempDir << this->tempFilePrefix << i
		//				<< ".tmp";
		//		remove(tempFileNameStream.str().c_str());
	}

	//	rmdir("tmp");
}

/**
 * Register current memory allocation.
 */
void saveMemTime(ofstream& memTimeFile, int initialTime) {
	return;
	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
	double timeElapsed = (double) (1000000 * usage.ru_utime.tv_sec
			+ usage.ru_utime.tv_usec - initialTime) / 1000000.0;
	struct mallinfo info = mallinfo();
	ostringstream memTimeStream;
	memTimeStream << timeElapsed << ", " << ((double) (info.usmblks
			+ info.uordblks) / 1048576.0) << endl;
	memTimeFile << memTimeStream.str();
}

/*
 * Index collection terms and documents.
 */
int Indexer::index() {
	ofstream memTimeFile("mem_time.csv", ios_base::app);

	struct rusage usage;

	/* STARTIG INDEXING TIMING */
	// Start index saving timing
	long indexingTimeSec;
	getrusage(RUSAGE_SELF, &usage);
	long indexingT0 = 1000000 * usage.ru_utime.tv_sec + usage.ru_utime.tv_usec;
	saveMemTime(memTimeFile, indexingT0);

	this->reader = new CollectionReader(this->collectionDirectory,
			this->collectionIndexFileName);

	mkdir(this->tempDir.c_str(), S_IRWXU);

	Document doc;
	doc.clear();
	int i = 0;
	int lastDocNumber = 0;
	int numDocsInvalidHTTPHeader = 0;
	int numDocsInvalidContentType = 0;
	int numDocsUnknownCharSet = 0;

	long totalParsingTime = 0;
	long totalTriplesTime = 0;

	while (reader->getNextDocument(doc)) {

		/* --- START PARSING --- */
		// Begin parsing timing
		//		clock_t parsingT0 = clock();
		long parsingT0 = 1000000 * usage.ru_utime.tv_sec
				+ usage.ru_utime.tv_usec;
		// Verify memory allocation before parsing
		saveMemTime(memTimeFile, indexingT0);

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

		/* --- END PARSING --- */
		// End parsing timing
		getrusage(RUSAGE_SELF, &usage);
		totalParsingTime += (1000000 * usage.ru_utime.tv_sec
				+ usage.ru_utime.tv_usec - parsingT0);
		// Verify memory allocation after parsing
		saveMemTime(memTimeFile, indexingT0);

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
			cout << endl;
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

		// ELIMINATE NOT PRINTABLE CHARACTERS

		for (int k = 0; k < (int) usefulContent.size(); k++) {
			int asciiCode = (int) usefulContent[k];

			if (asciiCode < 32 || asciiCode > 126) {
				usefulContent[k] = ' ';
			}
		}

		// REGISTER DOCUMENT URL AND GET ITS NUMBER

		this->documents.push_back(doc.getURL());
		int docNumber = lastDocNumber++;

		// GET INDEXABLE TERMS IN LOWER CASE

		vector<string> indexableTerms;
		//		tokenize2(usefulContent, indexableTerms, true);
		tokenize(usefulContent, indexableTerms,
				" \t\r\n\"\'!@#&*()_+-|=`{[}]^~?/\\:><,.;", true);

		// SAVE SORTED TRIPLES IN A TEMPORARY FILE

		/* --- START SAVING TRIPLES --- */
		// Begin triples saving timing
		getrusage(RUSAGE_SELF, &usage);
		long triplesT0 = 1000000 * usage.ru_utime.tv_sec
				+ usage.ru_utime.tv_usec;
		// Verify memory allocation before triples saving
		saveMemTime(memTimeFile, indexingT0);

		saveTriplesTempFile(indexableTerms, docNumber);

		/* --- END SAVING TRIPLES --- */
		// End triples saving timing
		getrusage(RUSAGE_SELF, &usage);
		totalTriplesTime += (1000000 * usage.ru_utime.tv_sec
				+ usage.ru_utime.tv_usec - triplesT0);
		// Verify memory allocation after triples saving
		saveMemTime(memTimeFile, indexingT0);

		doc.clear();
		i++;

		if (docNumber % 500 == 0) {
			cout << docNumber << "\t";
			flush(cout);
		}

		if (docNumber >= 999) {
			break;
		}
	}

	// SAVE REMAINING TRIPLES IN THE TEMPORARY FILE

	/* --- START SAVING TRIPLES --- */
	// Begin remaninig triples saving timing
	getrusage(RUSAGE_SELF, &usage);
	long triplesT0 = 1000000 * usage.ru_utime.tv_sec + usage.ru_utime.tv_usec;
	// Verify memory allocation before remaining triples saving
	saveMemTime(memTimeFile, indexingT0);

	if ((int) this->kTriples.size() > 0) {
		sort(this->kTriples.begin(), this->kTriples.end(), lesserTriple);
		saveTriplesVectorTempFile();
		this->kTriples.clear();
	}

	/* --- END SAVING TRIPLES --- */
	// End remaninig triples saving timing
	getrusage(RUSAGE_SELF, &usage);
	totalTriplesTime += (1000000 * usage.ru_utime.tv_sec
			+ usage.ru_utime.tv_usec - triplesT0);
	// Verify memory allocation after remaining triples saving
	saveMemTime(memTimeFile, indexingT0);

	// SAVE VOCABULARY

	cout << endl << "SAVING VOCABULARY... ";

	/* --- START SAVING VOCABULARY --- */
	// Start saving vocabulary
	getrusage(RUSAGE_SELF, &usage);
	long vocabT0 = 1000000 * usage.ru_utime.tv_sec + usage.ru_utime.tv_usec;
	// Verify memory allocation before vocabulary saving
	saveMemTime(memTimeFile, indexingT0);

	ofstream vocabFile("vocabulary");
	map<string, int>::iterator vocabIter;
	for (vocabIter = vocabulary.begin(); vocabIter != vocabulary.end(); vocabIter++) {
		int termSize = vocabIter->first.size();
		char *termStr = (char*) vocabIter->first.c_str();

		vocabFile.write((char*) &(vocabIter->second), sizeof(int));
		vocabFile.write((char*) &termSize, sizeof(int));
		vocabFile.write(termStr, vocabIter->first.size());
	}
	vocabFile.close();

	/* --- END SAVING VOCABULARY --- */
	// End saving vocabulary
	getrusage(RUSAGE_SELF, &usage);
	long vocabTime = (1000000 * usage.ru_utime.tv_sec + usage.ru_utime.tv_usec
			- vocabT0);
	// Verify memory allocation after vocabulary saving
	saveMemTime(memTimeFile, indexingT0);

	cout << "OK!" << endl;

	// SAVE URLS

	cout << "SAVING URLS... ";

	/* --- START SAVING URLS --- */
	// Start saving URLs
	getrusage(RUSAGE_SELF, &usage);
	long urlsT0 = 1000000 * usage.ru_utime.tv_sec + usage.ru_utime.tv_usec;
	// Verify memory allocation before URLs saving
	saveMemTime(memTimeFile, indexingT0);

	ofstream docsFile("urls");
	vector<string>::iterator docsIter;
	for (docsIter = documents.begin(); docsIter != documents.end(); docsIter++) {
		docsFile << *docsIter << endl;
	}
	docsFile.close();

	/* --- END SAVING URLS --- */
	// End saving URLs
	getrusage(RUSAGE_SELF, &usage);
	long urlsTime = (1000000 * usage.ru_utime.tv_sec + usage.ru_utime.tv_usec
			- urlsT0);
	// Verify memory allocation after vocabulary saving
	saveMemTime(memTimeFile, indexingT0);

	cout << "OK!" << endl;

	// MERGE SORTED RUNS AND SAVE INVERTED FILE

	cout << "SAVING INVERTED FILE... ";

	/* --- START MERGE TRIPLES AND SAVE INDEX --- */
	// Start index saving timing
	getrusage(RUSAGE_SELF, &usage);
	long indexSavingTime;
	long indexSavingT0 = 1000000 * usage.ru_utime.tv_sec
			+ usage.ru_utime.tv_usec;
	// Verify memory allocation before index saving
	saveMemTime(memTimeFile, indexingT0);

	mergeSortedRuns();

	/* --- END MERGE TRIPLES AND SAVE INDEX --- */
	// End index saving timing
	getrusage(RUSAGE_SELF, &usage);
	indexSavingTime = 1000000 * usage.ru_utime.tv_sec + usage.ru_utime.tv_usec
			- indexSavingT0;
	// Verify memory allocation after index saving
	saveMemTime(memTimeFile, indexingT0);

	cout << "OK!" << endl;

	/* END INDEXING TIMING */
	// End indexing timing
	getrusage(RUSAGE_SELF, &usage);
	indexingTimeSec = 1000000 * usage.ru_utime.tv_sec + usage.ru_utime.tv_usec
			- indexingT0;
	saveMemTime(memTimeFile, indexingT0);

	// PRINT USEFUL INFORMATION
	cout << "\n#############################" << endl;
	cout << "Invalid HTTP header:\t" << numDocsInvalidHTTPHeader << endl;
	cout << "Invalid content type:\t" << numDocsInvalidContentType << endl;
	cout << "Unknown char set:\t" << numDocsUnknownCharSet << endl;
	cout << "Useful documents:\t" << lastDocNumber << endl;
	cout << "Number of terms:\t" << vocabulary.size() << endl;
	cout << "Number of triples:\t" << numTriplesSaved << endl;
	cout << "Value of k:\t\t" << this->k << endl;
	cout << "Parsing documents time:\t" << (double) totalParsingTime
			/ 1000000.0 << " s" << endl;
	cout << "Triples saving time:\t" << (double) totalTriplesTime / 1000000.0
			<< " s" << endl;
	cout << "Vocab. saving time:\t" << (double) vocabTime / 1000000.0 << " s"
			<< endl;
	cout << "URLs saving time:\t" << (double) urlsTime / 1000000.0 << " s"
			<< endl;
	cout << "Index saving time:\t" << (double) indexSavingTime / 1000000.0
			<< " s" << endl;
	cout << "Total indexing time:\t" << (double) indexingTimeSec / 1000000.0
			<< " s" << endl;

	return 0;
}
