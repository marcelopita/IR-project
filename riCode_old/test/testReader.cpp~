/**
 * Test the CollectionReader class
 * @author: anisio@dcc.ufmg.br
 */

#include "CollectionReader.h"

#include <cstdlib>
#include <iostream>

using namespace std;
using namespace RICPNS;

int main(int argc, char** argv) {

	cout << "Testing CollectionReader class..." << endl;

	string inputDirectory("/mnt/hd1/anisio/doutorado/estagio/RI/data/outCompressed");
	string indexFileName("indexToCompressedColection.txt");

	CollectionReader * reader = new CollectionReader(inputDirectory,
													 indexFileName);
	Document doc;
	doc.clear();
	int i = 0;
	while(reader->getNextDocument(doc))	{
		//if(doc.getURL() == "http://www.metodoinvestimentos.com.br/parceiros.asp") {
		//	cout << "[" << doc.getURL() << "] [" << doc.getLength() << "]" << endl;
		//	cout << "BEGINHTML[" << doc.getText() << "]ENDHTML" << endl;
		//}
		if((i%1000) == 0) {
			cerr << "[" << doc.getURL() << "]" << endl;
			cerr << i << " processed" << endl;
		}
		doc.clear();
		++i;
	}
	cerr << "Total [" << i << "]" << endl;

	delete reader;

	cout << "Test finished." << endl;

	return EXIT_SUCCESS;
}

