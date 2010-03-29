/**
 * Document representation
 * author: anisio@dcc.ufmg.br
 */

#ifndef __DOCUMENT_H__
#define __DOCUMENT_H__

#include <string>

namespace RICPNS {

  class Document {
    public:

			Document();
      Document(const std::string & url, 
							 const std::string & text,
							 const size_t & length);
      virtual ~Document();

			void setURL(const std::string & url);
      std::string getURL() const;
			void setText(const std::string & text);
      std::string getText() const;
			void setLength(const size_t & length);
			size_t getLength() const;

			void clear();

    private:

    	std::string url_;
			std::string text_;
			size_t length_;
  };
}

#endif

