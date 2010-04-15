/*
 * HTML_Parser.h
 *
 *  Created on: 13/04/2010
 *      Author: pita
 */

#ifndef HTML_PARSER_H_
#define HTML_PARSER_H_

#include <htmlcxx/html/ParserSax.h>
#include <string>

using namespace htmlcxx::HTML;
using namespace std;

class HTML_Parser : public ParserSax {

public:

	void beginParsing();

	void foundTag(Node node, bool isEnd);

	void foundText(Node node);

	void setSearchingCharset(bool searchingCharset);

	void getText(string&text);

	void getCharset(string&charset);

private:

	bool searchingCharset;

	bool isInsideInvalidTag;

	string text;

	string charset;

	void setCharset(string&contentTypeAttrValue);

};

#endif /* HTML_PARSER_H_ */
