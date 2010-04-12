/*
 * main.cpp
 *
 *  Created on: 23/03/2010
 *      Author: pita
 */

#include <iostream>
#include <stdlib.h>
#include <string>
#include <set>
#include <vector>

#include "../include/Indexer.h"
#include "../include/QueryProcessor.h"

using namespace std;

void tokenize(const string& str, vector<string>& tokens,
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

int processQueries() {
	cout << "*** PROCESSAMENTO DE CONSULTA ***\n";

	string vocabularyFileName = "vocabulary";
	string urlsFileName = "urls";
	string invertedFileName = "inverted_file";

	set<string> booleanOperatorsWords;
	booleanOperatorsWords.insert("&&");
	booleanOperatorsWords.insert("||");

	QueryProcessor queryProcessor(vocabularyFileName, urlsFileName,
			invertedFileName);

	bool endSearch = false;
	while (!endSearch) {
		vector<string> input;
		set<string> words;
		set<string> docs;

		string query;
		cout << "\nBusca >> ";
		getline(cin, query);

		tokenize(query, input, " ");

		bool expectOperator = false;

		vector<string>::iterator inputIter;
		for (inputIter = input.begin(); inputIter != input.end(); inputIter++) {
			if (expectOperator && (booleanOperatorsWords.find(*inputIter)
					== booleanOperatorsWords.end())) {
				cerr << "Entrada inválida!" << endl;
				endSearch = true;
				break;
			}
		}
	}

	//	string *tempStr = new string;
	//
	//	for (unsigned i = 0; i < query.size(); i++) {
	//		char currentChar = query.at(i);
	//
	//		if (currentChar == ' ' || i == (query.size() - 1)) {
	//			tempStr->append(1, currentChar);
	//			words.push_back(*tempStr);
	//			tempStr = new string;
	//
	//		} else {
	//			tempStr->append(1, currentChar);
	//		}
	//	}
	//
	//	vector<string> docs;
	//	QueryProcessor queryProcessor(words, docs);
	//
	//	cout << "\nResultados:\n\n";

	//	queryProcessor.query(words, QueryProcessor::OR, docs);

	return 0;
}

int indexCollection(string *collectionDirectory,
		string *collectionIndexFileName, string *tempFileNamePrefix,
		string *indexFileName, int runSize) {
	cout << "Indexing collection..." << endl;

	Indexer indexer(*collectionDirectory, *collectionIndexFileName,
			*tempFileNamePrefix, *indexFileName, runSize);
	int ret = indexer.index();

	delete collectionDirectory;
	delete collectionIndexFileName;

	return ret;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		cerr << "|ERRO|\tInforme um dos modos como parâmetro do programa:";
		cerr << "\n\t(1) processar consultas;\n\t(2) indexar coleção." << endl;
		exit(1);

	} else {
		int mode = atoi(argv[1]);

		if (mode == 1) {
			exit(processQueries());

		} else if (mode == 2) {
			exit(indexCollection(new string(argv[2]), new string(argv[3]),
					new string(argv[4]), new string(argv[5]), atoi(argv[6])));

		} else {
			cerr << "|ERRO|\tInforme um dos modos como parâmetro do programa:";
			cerr << "\n\t(1) processar consultas;\n\t(2) indexar coleção.\n";
			exit(1);
		}
	}
}
