/**
 * Collection object
 * @author: anisio@dcc.ufmg.br
 */

#ifndef __COLLECTION_READER_H__
#define __COLLECTION_READER_H__

#include "envDefault.h"
#include "Document.h"

#include <string>
#include <stdio.h>

namespace RICPNS {

	class CollectionReader {
		public:

			explicit CollectionReader(const std::string & inputDirectory,
									  const std::string & indexFileName);
			virtual ~CollectionReader();

			bool getNextDocument(Document & doc);

			// TODO:
			// std::string getDocument(const std::string & url);

		private:

			void initialize();
			int uncompressDocument(unsigned char * from,
								   size_t & compressedDocSize,
								   size_t & uncompressedDocSize,
								   std::string & to);

			std::string inputDirectory_;
			std::string inputIndexFileName_;
			std::string inputContentFileName_;

			FILE* inputIndexFilePtr_;
			FILE* inputContentFilePtr_;

			size_t inputCurrentOffset_;
	};
}

#endif

