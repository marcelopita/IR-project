/**
 * CollectionReader class implementation
 * @author: anisio@dcc.ufmg.br
 */

#include "CollectionReader.h"

#include <iostream>
#include <cassert>

using namespace std;
using namespace RICPNS;

CollectionReader::CollectionReader(const string & inputDirectory,
																	 const string & indexFileName)
	: inputDirectory_(inputDirectory),
	  inputIndexFileName_(inputDirectory + '/' + indexFileName),
		inputContentFileName_(""),
	  inputIndexFilePtr_(NULL),
	  inputContentFilePtr_(NULL),
	  inputCurrentOffset_(0) {

	initialize();
}

CollectionReader::~CollectionReader() {
	if(inputIndexFilePtr_ != NULL) {
		fclose(inputIndexFilePtr_);
	}
	if(inputContentFilePtr_ != NULL) {
		fclose(inputContentFilePtr_);
	}
}

void CollectionReader::initialize() {
	inputIndexFilePtr_ = fopen(inputIndexFileName_.c_str(), "r");
	assert(inputIndexFilePtr_ != NULL);

	if(DEBUG) {
		cerr << "Index File Name [" << inputIndexFileName_ << "] openned." << endl;
		cerr << "Initialization OK!" << endl;
	}
}

bool CollectionReader::getNextDocument(Document & doc) {

	char url[MAX_STRING_SIZE], inputContentFileName[MAX_STRING_SIZE];
	size_t beginOffset = 0, endOffset = 0, uncompressedPageSize = 0;
	
	fscanf(inputIndexFilePtr_, "%s %s %ud %ud %ud",
				 url,
				 inputContentFileName,
				 &beginOffset,
				 &endOffset,
				 &uncompressedPageSize);

	string tmpContentFileName;
	if(inputContentFileName_ == "") { // Reading first line in index
		// Openning content file
		tmpContentFileName = inputDirectory_ + '/' + string(inputContentFileName);
		inputContentFileName_ = tmpContentFileName;

		inputContentFilePtr_ = fopen(inputContentFileName_.c_str(), "r");
		assert(inputContentFilePtr_ != NULL);

		if(DEBUG) { cerr << "File [" << inputContentFileName_ << "] openned." << endl;  }
	}

	tmpContentFileName = inputDirectory_ + '/' + string(inputContentFileName);
	if(inputContentFileName_ != tmpContentFileName) { // It's time to move to next file
		fclose(inputContentFilePtr_);
		inputContentFilePtr_ = NULL;

		if(DEBUG) { cerr << "File [" << inputContentFileName_ << "] closed." << endl;  }

		inputContentFileName_ = tmpContentFileName;
		inputContentFilePtr_  = fopen(inputContentFileName_.c_str(), "r");
		assert(inputContentFilePtr_ != NULL);

		if(DEBUG) { cerr << "File [" << inputContentFileName_ << "] openned." << endl;  }

		cerr << "begin [" << beginOffset << "] end[" << endOffset << "]" << endl;
	}

	unsigned char * compressedDoc = NULL;
	size_t compressedDocSize = endOffset - beginOffset;
	compressedDoc = new unsigned char[compressedDocSize];
	size_t nchars = fread(compressedDoc, sizeof(char), compressedDocSize, inputContentFilePtr_);
	assert(nchars == compressedDocSize);

	if(feof(inputIndexFilePtr_)) {
		return false;
	}

	string docStr;
	int uRet = uncompressDocument(compressedDoc,
																compressedDocSize,
																uncompressedPageSize,
																docStr);

	assert(uRet == Z_OK);

	doc.setURL(url);
	doc.setText(docStr);
	doc.setLength(uncompressedPageSize);

	delete[] compressedDoc;

	return true;
}

int CollectionReader::uncompressDocument(unsigned char * docCompressed,
																				 size_t & compressedDocSize,
																				 size_t & uncompressedDocSize, 
																				 string & to) {

	unsigned char * out = new unsigned char[uncompressedDocSize + 1];

	int ret = uncompress(out, (uLongf*) &uncompressedDocSize, docCompressed, compressedDocSize);
	out[uncompressedDocSize] = '\0';

	if(DEBUG) {
		switch(ret) {
			case Z_OK:
				//cerr << "Z_OK" << endl;
				break;
			case Z_MEM_ERROR:
				cerr << "Z_MEM_ERROR" << endl;
				break;
			case Z_BUF_ERROR:
				cerr << "Z_BUF_ERROR" << endl;
				break;
			case Z_DATA_ERROR:
				cerr << "Z_DATA_ERROR" << endl;
				break;
			default:
				cerr << "Hein?" << endl;
				break;
		}
	}

	string tmpOut((const char *)out);
	delete[] out;

	to = tmpOut;

	return ret;	
}

