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
#include <algorithm>

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

void resultsUnion(set<string>& s1, set<string>& s2, set<string>& result) {
	set<string> s;
	set_union(s1.begin(), s1.end(), s2.begin(), s2.end(),
			inserter(s, s.begin()));
	result = s;
}

void resultsIntersection(set<string>& s1, set<string>& s2, set<string>& result) {
	set<string> s;
	set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(s,
			s.begin()));
	result = s;
}

int processQueries() {
	cout << "\n*** PROCESSAMENTO DE CONSULTA ***\n";

	string vocabularyFileName = "vocabulary";
	string urlsFileName = "urls";
	string invertedFileName = "inverted_file";

	cout << endl;
	QueryProcessor queryProcessor(vocabularyFileName, urlsFileName,
			invertedFileName);

	while (true) {
		vector<string> input;
		set<string> words;
		set<string> docs;

		string query;
		cout << "\n-----------------------------------------";
		cout << "\nBuscar >> ";
		getline(cin, query);

		tokenize(query, input, " ");

		if (input.size() == 1 && input[0] == "quit")
			break;

		vector<string>::iterator inputIter;
		for (inputIter = input.begin(); inputIter != input.end(); inputIter++) {
			if (*inputIter == "and") {
				if ((++inputIter) != input.end()) {
					set<string> singleTermResult;
					queryProcessor.querySingleTerm(*inputIter, singleTermResult);
					resultsIntersection(docs, singleTermResult, docs);
				}

			} else if (*inputIter == "or") {
				if ((++inputIter) != input.end()) {
					set<string> singleTermResult;
					queryProcessor.querySingleTerm(*inputIter, singleTermResult);
					resultsUnion(docs, singleTermResult, docs);
				}

			} else {
				queryProcessor.querySingleTerm(*inputIter, docs);
			}
		}

		cout << "\nResultados:\n\n";

		set<string>::iterator docsIter;
		for (docsIter = docs.begin(); docsIter != docs.end(); docsIter++) {
			cout << "\t" << *docsIter << endl;
		}
	}

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
