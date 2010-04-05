/**
 * Test the CollectionWriter class
 * @author: anisio@dcc.ufmg.br
 */

#include "CollectionWriter.h"

#include <cstdlib>
#include <iostream>

using namespace std;
using namespace RICPNS;

int main(int argc, char** argv) {

	cout << "Testing CollectionWriter class..." << endl;

	string inputDirectory("/mnt/hd5/coletaRI");
	string indexFileName("indexFixed.txt");
	string outputDirectory("/mnt/hd5/coletaRI/compressed");
	string outputIndexFileName("indexToCompressedColection.txt");
	string prefixFileName("pagesRICompressed");
	
	CollectionWriter * writer = new CollectionWriter(inputDirectory,
													 indexFileName,
													 outputDirectory,
													 outputIndexFileName,
													 prefixFileName);

	writer->dump();

	delete writer;

	cout << "Test finished." << endl;

	return EXIT_SUCCESS;
}

