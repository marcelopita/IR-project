/*
 * QueryProcessor.cpp
 *
 *  Created on: 11/04/2010
 *      Author: pita
 */

#include "../include/QueryProcessor.h"
#include "../include/Util.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>

QueryProcessor::QueryProcessor(string& vocabularyFileName,
		string& urlsFileName, string& invertedFileName) {
	cout << "Carregando vocabulário... ";
	flush(cout);
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
	cout << "OK!" << endl;
	flush(cout);

	cout << "Carregando URLs... ";
	flush(cout);
	ifstream urlsFile(urlsFileName.c_str());
	int docNumber = 0;
	while (!urlsFile.eof()) {
		int docSize, urlSize;
		const char *urlChr = new char[urlSize];

		if (urlsFile.read((char*) &docSize, sizeof(int)) <= 0)
			continue;

		if (urlsFile.read((char*) &urlSize, sizeof(int)) <= 0)
			continue;

		if (urlsFile.read((char*) urlChr, urlSize * sizeof(char)) <= 0)
			continue;

		string url(urlChr);

		//		getline(urlsFile, url);

		docsSizes[docNumber] = docSize;
		urls.push_back(urlChr);

		docNumber++;
	}
	urlsFile.close();
	cout << "OK!" << endl;
	flush(cout);

	this->indexFileNamePrefix = invertedFileName;
}

//void QueryProcessor::querySingleTerm(string& word, set<string>& docs) {
//	docs.clear();
//
//	transform(word.begin(), word.end(), word.begin(), (int(*)(int)) tolower);
//	Util::changeCharSet("UTF-8", "ASCII//TRANSLIT", word);
//
//	int wordNumber;
//
//	map<string, int>::iterator wordRef = this->vocabulary.find(word);
//	if (wordRef != this->vocabulary.end()) {
//		wordNumber = wordRef->second;
//	} else {
//		return;
//	}
//
//	ostringstream termsFileNameStream;
//	termsFileNameStream << this->indexFileNamePrefix << "_terms";
//	ifstream termsFile(termsFileNameStream.str().c_str());
//
//	ostringstream docsFileNameStream;
//	docsFileNameStream << this->indexFileNamePrefix << "_docs";
//	ifstream docsFile(docsFileNameStream.str().c_str());
//
//	set<int> docsNumbers;
//	docsNumbers.clear();
//
//	termsFile.seekg(wordNumber * 3 * sizeof(int));
//
//	int termNumber;
//	if (termsFile.read((char*) &termNumber, sizeof(int)) <= 0)
//		return;
//
//	int tf;
//	if (termsFile.read((char*) &tf, sizeof(int)) <= 0)
//		return;
//
//	int docsRef;
//	if (termsFile.read((char*) &docsRef, sizeof(int)) <= 0) {
//		return;
//	}
//
//	docsFile.seekg(docsRef * 3 * sizeof(int));
//	for (int i = 0; i < tf; i++) {
//		int docNumber;
//		if (docsFile.read((char*) &docNumber, sizeof(int)) <= 0) {
//			continue;
//		}
//
//		int temp;
//		if (docsFile.read((char*) &temp, sizeof(int)) <= 0) {
//			continue;
//		}
//		if (docsFile.read((char*) &temp, sizeof(int)) <= 0) {
//			continue;
//		}
//
//		docsNumbers.insert(docNumber);
//	}
//
//	set<int>::iterator docsNumbersIter;
//	for (docsNumbersIter = docsNumbers.begin(); docsNumbersIter
//			!= docsNumbers.end(); docsNumbersIter++) {
//		docs.insert(urls[*docsNumbersIter]);
//	}
//}
