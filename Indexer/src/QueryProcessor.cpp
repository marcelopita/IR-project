/*
 * QueryProcessor.cpp
 *
 *  Created on: 11/04/2010
 *      Author: pita
 */

#include "../include/QueryProcessor.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>
#include <iconv.h>
#include <locale.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>

QueryProcessor::QueryProcessor(string& vocabularyFileName,
		string& urlsFileName, string& invertedFileName) {
	ifstream vocabularyFile(vocabularyFileName.c_str());
	while (!vocabularyFile.eof()) {
		int termNumber;
		if (vocabularyFile.read((char*) &termNumber, sizeof(int)) <= 0)
			continue;

		int termSize;
		if (vocabularyFile.read((char*) &termSize, sizeof(int)) <= 0)
			continue;

		char term[termSize + 1];
		if (vocabularyFile.read(term, termSize) <= 0)
			continue;
		term[termSize] = '\0';

		string termStr(term);
		vocabulary.insert(make_pair(termStr, termNumber));
	}
	vocabularyFile.close();

	ifstream urlsFile(urlsFileName.c_str());
	while (!urlsFile.eof()) {
		string url;
		getline(urlsFile, url);
		urls.push_back(url);
	}
	urlsFile.close();

	this->indexFileNamePrefix = invertedFileName;
}

QueryProcessor::~QueryProcessor() {
}

/**
 * Change char set of content.
 */
int QueryProcessor::changeCharSet(const string& from, const string& to,
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
		return 0;
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

	return 1;
}

void QueryProcessor::unionOp(set<set<int> >& sets, set<int>& unionSet) {
	set<int> s;
	set<set<int> >::iterator setsIter;
	for (setsIter = sets.begin(); setsIter != sets.end(); setsIter++) {
		//		unionSet.insert(setsIter->begin(), setsIter->end());
		set_union(setsIter->begin(), setsIter->end(), unionSet.begin(),
				unionSet.end(), inserter(s, s.begin()));
		unionSet = s;
	}
}

void QueryProcessor::intersectionOp(set<set<int> >& sets,
		set<int>& intersectionSet) {
	set<int> s;
	set<set<int> >::iterator setsIter;
	for (setsIter = sets.begin(); setsIter != sets.end(); setsIter++) {
		set_intersection(setsIter->begin(), setsIter->end(),
				intersectionSet.begin(), intersectionSet.end(), inserter(s,
						s.begin()));
		intersectionSet = s;
	}
}

void QueryProcessor::getDocsAllWords(set<int>& words, set<int>& docs) {
	ostringstream termsFileNameStream;
	termsFileNameStream << this->indexFileNamePrefix << "_terms";
	ifstream termsFile(termsFileNameStream.str().c_str());

	ostringstream docsFileNameStream;
	docsFileNameStream << this->indexFileNamePrefix << "_docs";
	ifstream docsFile(docsFileNameStream.str().c_str());

	set<set<int> > docsPerWord;

	set<int>::iterator wordsIter;
	for (wordsIter = words.begin(); wordsIter != words.end(); wordsIter++) {
		set<int> wordDocs;

		termsFile.seekg(sizeof(int) * ((*wordsIter) * 3 + 2));

		int docsListRef;
		if (termsFile.read((char*) &docsListRef, sizeof(int)) <= 0)
			continue;

		docsFile.seekg(sizeof(int) * (1 + (*wordsIter) * 3));

		int numDocs;
		if (docsFile.read((char*) &numDocs, sizeof(int)) <= 0)
			continue;

		for (int i = 0; i < numDocs; i++) {
			int docNumber;
			if (docsFile.read((char*) &docNumber, sizeof(int)) <= 0)
				continue;
			wordDocs.insert(docNumber);

			int temp;
			if (docsFile.read((char*) &temp, sizeof(int)) <= 0)
				continue;
			if (docsFile.read((char*) &temp, sizeof(int)) <= 0)
				continue;
		}

		docsPerWord.insert(wordDocs);
	}

	intersectionOp(docsPerWord, docs);
}

void QueryProcessor::getDocsSomeWord(set<int>& words, set<int>& docs) {
	ostringstream termsFileNameStream;
	termsFileNameStream << this->indexFileNamePrefix << "_terms";
	ifstream termsFile(termsFileNameStream.str().c_str());

	ostringstream docsFileNameStream;
	docsFileNameStream << this->indexFileNamePrefix << "_docs";
	ifstream docsFile(docsFileNameStream.str().c_str());

	//	ostringstream posFileNameStream;
	//	posFileNameStream << this->indexFileNamePrefix << "_pos";
	//	ifstream posFile(posFileNameStream.str().c_str());

	set<set<int> > docsPerWord;

	set<int>::iterator wordsIter;
	for (wordsIter = words.begin(); wordsIter != words.end(); wordsIter++) {
		set<int> wordDocs;

		termsFile.seekg((*wordsIter) * 3 * sizeof(int));

		int termNumber;
		if (termsFile.read((char*) &termNumber, sizeof(int)) <= 0)
			continue;

		int tf;
		if (termsFile.read((char*) &tf, sizeof(int)) <= 0)
			continue;

		int docsRef;
		if (termsFile.read((char*) &docsRef, sizeof(int)) <= 0) {
			continue;
		}

		docsFile.seekg(docsRef * 3 * sizeof(int));
		for (int i = 0; i < tf; i++) {
			int docNumber;
			if (docsFile.read((char*) &docNumber, sizeof(int)) <= 0) {
				continue;
			}

			int temp;
			if (docsFile.read((char*) &temp, sizeof(int)) <= 0) {
				continue;
			}
			if (docsFile.read((char*) &temp, sizeof(int)) <= 0) {
				continue;
			}

			cout << docNumber << ", ";

			wordDocs.insert(docNumber);
		}

		docsPerWord.insert(wordDocs);
	}

	unionOp(docsPerWord, docs);
}

void QueryProcessor::query(set<string>& words, int mode, set<string>& docs) {
	set<int> wordsNumbers;

	set<string>::iterator wordsIter;
	for (wordsIter = words.begin(); wordsIter != words.end(); wordsIter++) {
		string word = *wordsIter;
		transform(word.begin(), word.end(), word.begin(), (int(*)(int)) tolower);
		changeCharSet("UTF-8", "ASCII//TRANSLIT", word);

		map<string, int>::iterator wordRef = this->vocabulary.find(word);
		if (wordRef != this->vocabulary.end()) {
			cout << wordRef->first << ": " << wordRef->second << endl;
			wordsNumbers.insert(wordRef->second);
		}
	}

	docs.clear();
	set<int> docsNumbers;

	if (mode == AND) {
		getDocsAllWords(wordsNumbers, docsNumbers);

	} else if (mode == OR) {
		getDocsSomeWord(wordsNumbers, docsNumbers);
	}

	set<int>::iterator docsNumbersIter;
	for (docsNumbersIter = docsNumbers.begin(); docsNumbersIter
			!= docsNumbers.end(); docsNumbersIter++) {
		docs.insert(urls[*docsNumbersIter]);
	}
}
