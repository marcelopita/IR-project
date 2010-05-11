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
#include "../include/BooleanProcessor.h"
#include "../include/VectorSpaceProcessor.h"
#include "../include/Util.h"

using namespace std;

int processQueries(int model) {
	cout << "\n*** PROCESSAMENTO DE CONSULTA ***\n\n";

	string vocabularyFileName = "vocabulary";
	string urlsFileName = "urls";
	string invertedFileName = "inverted_file";

	QueryProcessor *queryProcessor = NULL;

	switch (model) {
	case QueryProcessor::BOOLEAN:
		cout << "Processador: Booleano" << endl;
		queryProcessor = new BooleanProcessor(vocabularyFileName, urlsFileName,
				invertedFileName);
		break;

	case QueryProcessor::VECTOR_SPACE:
		cout << "Processador: Espaço Vetorial" << endl;
		queryProcessor = new VectorSpaceProcessor(vocabularyFileName,
				urlsFileName, invertedFileName, -1, -1);
		break;

		//	case QueryProcessor::BM25:
		//		break;
		//
		//	case QueryProcessor::HYBRID:
		//		break;
		//
		//	case QueryProcessor::MY_MODEL:
		//		break;

	default:
		cerr << "|ERRO|\tInforme um modelo válido:";
		cerr
				<< "\n\t(1) Boolean;\n\t(2) Vetorial;\n\t(3) BM-25;\n\t(4) Híbrido Vetorial/BM-25;\n\t(5) Novo modelo.\n\n";
		return 1;
	}

	while (true) {
		vector<string> input;
		vector<string> words;
		vector<string> docs;

		docs.clear();

		string query;
		cout << "\n-----------------------------------------";
		cout << "\nBuscar >> ";
		getline(cin, query);

		Util::changeCharSet("UTF-8", "ASCII//TRANSLIT", query);
		Util::tokenize(query, input, " ", true);

		if (input.size() == 0)
			break;

		queryProcessor->query(input, docs);

		cout << "\n" << docs.size() << " Resultados:\n\n";

		vector<string>::iterator docsIter;
		for (docsIter = docs.begin(); docsIter != docs.end(); docsIter++) {
			cout << "\t" << *docsIter << endl;
		}
	}

	if (queryProcessor != NULL) {
		delete queryProcessor;
	}

	return 0;
}

int indexCollection(string *collectionDirectory,
		string *collectionIndexFileName, string *tempFileNamePrefix,
		string *indexFileName, int runSize, int maxNumTriples,
		string *executionDir) {
	cout << "\n*** INDEXAÇÃO DE COLEÇÃO ***\n";

	Indexer indexer(*collectionDirectory, *collectionIndexFileName,
			*tempFileNamePrefix, *indexFileName, runSize, maxNumTriples,
			*executionDir);
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
			exit(processQueries(atoi(argv[2])));

		} else if (mode == 2) {
			exit(indexCollection(new string(argv[2]), new string(argv[3]),
					new string(argv[4]), new string(argv[5]), atoi(argv[6]),
					atoi(argv[7]), new string(argv[8])));

		} else {
			cerr << "|ERRO|\tInforme um dos modos como parâmetro do programa:";
			cerr << "\n\t(1) processar consultas;\n\t(2) indexar coleção.\n";
			exit(1);
		}
	}
}
