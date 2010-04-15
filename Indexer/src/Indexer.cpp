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
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "../include/HTML_Parser.h"
#include "../include/Util.h"

struct Triple {
	int termNumber;
	int docNumber;
	int termPosition;
};

struct RunTriple {
	int runId;
	Triple triple;
};

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
	this->numTriplesSaved = 0;
	this->numRuns = 0;
	this->lastDocumentEntry = 0;
	this->lastPositionEntry = 0;
}

Indexer::~Indexer() {
	delete this->reader;
}

/**
 * Extract terms from useful text and save triples.
 */
void Indexer::tokenizeAndSaveTriples(int docNumber, const string& str,
		const string& delimiters = " ", bool lowerCase = false) {
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	string::size_type pos = str.find_first_of(delimiters, lastPos);

	int termPosition = 0;
	while (string::npos != pos || string::npos != lastPos) {
		string subStr = str.substr(lastPos, pos - lastPos);
		if (lowerCase) {
			transform(subStr.begin(), subStr.end(), subStr.begin(),
					(int(*)(int)) tolower);
		}
		this->saveTripleTempFile(subStr, docNumber, termPosition);
		termPosition++;
		lastPos = str.find_first_not_of(delimiters, pos);
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
		return Util::DOC_BLOCKED;
	}

	string tempHeaderLine;
	bool endHeader = false;

	// Extract some HTTP header fields (content type) and detect its end.
	while (!endHeader) {
		getline(ss, tempHeaderLine);
		string headerLineNoSpaces(tempHeaderLine);
		Util::trim(headerLineNoSpaces, " \t\n\r");

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

	return Util::DOC_OK;
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
		return Util::DOC_BLOCKED;
	}

	contentType = "html";
	if (contentTypeHeaderLine.find("/plain") != string::npos
			|| contentTypeHeaderLine.find("/PLAIN") != string::npos) {
		contentType = "plain";
	}

	return Util::DOC_OK;
}

/**
 * Extract text charset from header.
 */
void Indexer::getCharSetHeader(string& contentTypeHeaderLine,
		string& charsetHeader) {
	if (contentTypeHeaderLine.find("charset") == string::npos
			&& contentTypeHeaderLine.find("CHARSET") == string::npos
			&& contentTypeHeaderLine.find("Charset") == string::npos) {
		charsetHeader = "";
		return;
	}

	vector<string> contentTypeTokens;
	Util::tokenize(contentTypeHeaderLine, contentTypeTokens, " ;=", false);

	for (unsigned i = 0; i < contentTypeTokens.size(); i++) {
		string token = contentTypeTokens.at(i);
		if ((token == "charset" || token == "CHARSET" || token == "Charset")
				&& (i < contentTypeTokens.size() - 1)) {
			charsetHeader = contentTypeTokens.at(i + 1);
			break;
		}
	}

	Util::trim(charsetHeader, " \n\r\t,;|()/\\{}");
}

/**
 * Extract useful content from HTML text.
 *
 */
void Indexer::extractUsefulContent(string& htmlText, string& usefulText) {
	HTML_Parser htmlParser;
	htmlParser.setSearchingCharset(false);
	htmlParser.parse(htmlText);
	htmlParser.getText(usefulText);
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
	}
}

/*
 * Index collection terms and documents.
 */
int Indexer::index() {
	ofstream memTimeFile("mem_time.csv", ios_base::app);

	/* STARTIG INDEXING TIMING */
	// Start index saving timing
	struct rusage indexingT0;
	getrusage(RUSAGE_SELF, &indexingT0);
	Util::saveMemTime(memTimeFile, indexingT0, indexingT0);

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

	struct rusage totalParsingTime;
	totalParsingTime.ru_utime.tv_sec = 0;
	totalParsingTime.ru_utime.tv_usec = 0;

	struct rusage totalTriplesTime;
	totalTriplesTime.ru_utime.tv_sec = 0;
	totalTriplesTime.ru_utime.tv_usec = 0;

	while (reader->getNextDocument(doc)) {

		/* --- START PARSING --- */
		// Begin parsing timing
		struct rusage parsingT0;
		getrusage(RUSAGE_SELF, &parsingT0);

		string fullText = doc.getText();

		// SEPARATE HEADER AND CONTENT

		string contentTypeHeaderLine, content;

		if (separateHeaderContent(fullText, contentTypeHeaderLine, content)
				== Util::DOC_BLOCKED) {
			numDocsInvalidHTTPHeader++;
			doc.clear();
			i++;
			continue;
		}

		// GET CONTENT TYPE FROM HEADER

		string contentType;

		if (getContentTypeHeader(contentTypeHeaderLine, contentType)
				== Util::DOC_BLOCKED) {
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
			HTML_Parser htmlParser;
			htmlParser.setSearchingCharset(true);
			htmlParser.parse(iso88591Content);
			htmlParser.getCharset(charsetMetaTag);
			//			getCharSetMetaTag(iso88591Content, charsetMetaTag);

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
			if (Util::changeCharSet(charset, "ISO-8859-1", content)
					== Util::DOC_BLOCKED) {
				numDocsUnknownCharSet++;
				doc.clear();
				i++;
				continue;
			}

			content = htmlcxx::HTML::decode_entities(content);
			charset = "ISO8859-1";
		}

		// CHANGE CONTENT CHAR SET TO ASCII USING TRANSLITERATION

		if (Util::changeCharSet(charset, "ASCII//TRANSLIT", content)
				== Util::DOC_BLOCKED) {
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

		/* --- END PARSING --- */
		// End parsing timing
		struct rusage parsingTime;
		getrusage(RUSAGE_SELF, &parsingTime);
		Util::minus(parsingTime, parsingT0, parsingTime);
		Util::plus(totalParsingTime, parsingTime, totalParsingTime);

		// REGISTER DOCUMENT URL AND GET ITS NUMBER

		this->documents.push_back(doc.getURL());
		int docNumber = lastDocNumber++;

		// GET INDEXABLE TERMS IN LOWER CASE AND SAVE SORTED TRIPLES IN A TEMPORARY FILE

		/* --- START SAVING TRIPLES --- */
		// Begin triples saving timing
		struct rusage triplesT0;
		getrusage(RUSAGE_SELF, &triplesT0);

		tokenizeAndSaveTriples(docNumber, usefulContent,
				" \t\r\n\"\'!@#&*()_+-|=`{[}]^~?/\\:><,.;", true);

		/* --- END SAVING TRIPLES --- */
		// End triples saving timing
		struct rusage triplesTime;
		getrusage(RUSAGE_SELF, &triplesTime);
		Util::minus(triplesTime, triplesT0, triplesTime);
		Util::plus(totalTriplesTime, triplesTime, totalTriplesTime);

		doc.clear();
		i++;

		if (docNumber % 3000 == 0) {
			struct rusage currentTime;
			getrusage(RUSAGE_SELF, &currentTime);
			Util::saveMemTime(memTimeFile, indexingT0, currentTime);
			cout << docNumber << "\t";
		}

		if (docNumber >= 8999) {
			break;
		}
	}

	// SAVE REMAINING TRIPLES IN THE TEMPORARY FILE

	/* --- START SAVING TRIPLES --- */
	// Begin remaninig triples saving timing
	struct rusage triplesT0;
	getrusage(RUSAGE_SELF, &triplesT0);

	if ((int) this->kTriples.size() > 0) {
		sort(this->kTriples.begin(), this->kTriples.end(), lesserTriple);
		saveTriplesVectorTempFile();
		this->kTriples.clear();
	}

	/* --- END SAVING TRIPLES --- */
	// End remaninig triples saving timing
	struct rusage triplesTime;
	getrusage(RUSAGE_SELF, &triplesTime);
	Util::minus(triplesTime, triplesT0, triplesTime);
	Util::plus(totalTriplesTime, triplesTime, totalTriplesTime);

	// SAVE VOCABULARY

	cout << endl << "SAVING VOCABULARY... ";

	/* --- START SAVING VOCABULARY --- */
	// Start saving vocabulary
	struct rusage vocabT0;
	getrusage(RUSAGE_SELF, &vocabT0);

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
	struct rusage vocabTime;
	getrusage(RUSAGE_SELF, &vocabTime);
	Util::minus(vocabTime, vocabT0, vocabTime);

	cout << "OK!" << endl;

	// SAVE URLS

	cout << "SAVING URLS... ";

	/* --- START SAVING URLS --- */
	// Start saving URLs
	struct rusage urlsT0;
	getrusage(RUSAGE_SELF, &urlsT0);

	ofstream docsFile("urls");
	vector<string>::iterator docsIter;
	for (docsIter = documents.begin(); docsIter != documents.end(); docsIter++) {
		docsFile << *docsIter << endl;
	}
	docsFile.close();

	/* --- END SAVING URLS --- */
	// End saving URLs
	struct rusage urlsTime;
	getrusage(RUSAGE_SELF, &urlsTime);
	Util::minus(urlsTime, urlsT0, urlsTime);

	cout << "OK!" << endl;

	// MERGE SORTED RUNS AND SAVE INVERTED FILE

	cout << "SAVING INVERTED FILE... ";

	/* --- START MERGE TRIPLES AND SAVE INDEX --- */
	// Start index saving timing
	struct rusage saveIndexT0;
	getrusage(RUSAGE_SELF, &saveIndexT0);

	mergeSortedRuns();

	/* --- END MERGE TRIPLES AND SAVE INDEX --- */
	// End index saving timing
	struct rusage saveIndexTime;
	getrusage(RUSAGE_SELF, &saveIndexTime);
	Util::minus(saveIndexTime, saveIndexT0, saveIndexTime);

	cout << "OK!" << endl;

	/* END INDEXING TIMING */
	// End indexing timing
	struct rusage indexingTime;
	getrusage(RUSAGE_SELF, &indexingTime);
	Util::minus(indexingTime, indexingT0, indexingTime);
	Util::saveMemTime(memTimeFile, indexingT0, indexingTime);

	// PRINT USEFUL INFORMATION
	cout << "\n#############################" << endl;
	cout << "Invalid HTTP header:\t" << numDocsInvalidHTTPHeader << endl;
	cout << "Invalid content type:\t" << numDocsInvalidContentType << endl;
	cout << "Unknown char set:\t" << numDocsUnknownCharSet << endl;
	cout << "Useful documents:\t" << lastDocNumber << endl;
	cout << "Number of terms:\t" << vocabulary.size() << endl;
	cout << "Number of triples:\t" << numTriplesSaved << endl;
	cout << "Value of k:\t\t" << this->k << endl;
	cout << "Parsing documents time:\t" << Util::getTimeStr(totalParsingTime)
			<< endl;
	cout << "Triples saving time:\t" << Util::getTimeStr(totalTriplesTime)
			<< endl;
	cout << "Vocab. saving time:\t" << Util::getTimeStr(vocabTime) << endl;
	cout << "URLs saving time:\t" << Util::getTimeStr(urlsTime) << endl;
	cout << "Index saving time:\t" << Util::getTimeStr(saveIndexTime) << endl;
	cout << "Total indexing time:\t" << Util::getTimeStr(indexingTime) << endl;

	return 0;
}
