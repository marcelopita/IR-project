/*
 * main.cpp
 *
 *  Created on: 23/03/2010
 *      Author: pita
 */

#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "../include/Indexer.h"

using namespace std;

int processQueries() {
	cout << "*** PROCESSAMENTO DE CONSULTA ***\n";

	string query;
	cout << "\nBusca >> ";
	getline(cin, query);

	vector<string> words;
	string *tempStr = new string;

	for (unsigned i = 0; i < query.size(); i++) {
		char currentChar = query.at(i);

		if (currentChar == ' ' || i == (query.size() - 1)) {
			tempStr->append(1, currentChar);
			words.push_back(*tempStr);
			tempStr = new string;

		} else {
			tempStr->append(1, currentChar);
		}
	}

//	for (unsigned i = 0; i < words.size(); i++) {
//		cout << words[i] << endl;
//	}

	cout << "\nResultados:\n";

	return 0;
}

int indexCollection(string *collectionDirectory, string *collectionIndexFileName) {
	cout << "Indexing collection..." << endl;

	Indexer indexer(*collectionDirectory, *collectionIndexFileName);
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
			exit(indexCollection(new string(argv[2]), new string(argv[3])));

		} else {
			cerr << "|ERRO|\tInforme um dos modos como parâmetro do programa:";
			cerr << "\n\t(1) processar consultas;\n\t(2) indexar coleção.\n";
			exit(1);
		}
	}
}
