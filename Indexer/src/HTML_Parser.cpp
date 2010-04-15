/*
 * HTML_Parser.cpp
 *
 *  Created on: 13/04/2010
 *      Author: pita
 */

#include "../include/HTML_Parser.h"
#include "../include/Util.h"

void HTML_Parser::beginParsing() {
	this->searchingCharset = false;
	this->isInsideInvalidTag = false;
	this->charset = "";
	this->text = "";
}

void HTML_Parser::setCharset(string&contentTypeText) {
	charset.clear();

	if (contentTypeText.find("charset") == string::npos
			&& contentTypeText.find("CHARSET") == string::npos
			&& contentTypeText.find("Charset") == string::npos) {
		return;
	}

	vector<string> contentTypeTokens;
	Util::tokenize(contentTypeText, contentTypeTokens, " ;=", false);

	for (unsigned i = 0; i < contentTypeTokens.size(); i++) {
		string token = contentTypeTokens.at(i);
		if ((token == "charset" || token == "CHARSET" || token == "Charset")
				&& (i < contentTypeTokens.size() - 1)) {
			charset = contentTypeTokens.at(i + 1);
			break;
		}
	}

	Util::trim(charset, " \n\r\t,;|()/\\{}");
}

void HTML_Parser::foundTag(Node node, bool isEnd) {
	string tagName = node.tagName();

	if (tagName == "script" || tagName == "SCRIPT" || tagName == "style"
			|| tagName == "STYLE") {
		if (isEnd)
			this->isInsideInvalidTag = false;
		else
			this->isInsideInvalidTag = true;

	} else if (this->searchingCharset && (tagName == "meta" || tagName
			== "META")) {
		node.parseAttributes();

		pair<bool, string> httpEquivAttrEntry;

		// If META tag has HTTP-EQUIV attribute
		if ((node.attribute("HTTP-EQUIV")).first) {
			httpEquivAttrEntry = node.attribute("HTTP-EQUIV");

		} else if ((node.attribute("http-equiv")).first) {
			httpEquivAttrEntry = node.attribute("http-equiv");

		} else {
			return;
		}

		Util::trim(httpEquivAttrEntry.second, " \t\n\r");

		// If HTTP-EQUIV attribute has value 'Content-Type'
		if (httpEquivAttrEntry.second == "Content-Type"
				|| httpEquivAttrEntry.second == "Content-type") {
			pair<bool, string> contentAttrEntry;

			// If META tag has CONTENT attribute
			if ((node.attribute("CONTENT")).first) {
				contentAttrEntry = node.attribute("CONTENT");

			} else if ((node.attribute("content")).first) {
				contentAttrEntry = node.attribute("content");

			} else if ((node.attribute("Content")).first) {
				contentAttrEntry = node.attribute("Content");

			} else {
				return;
			}

			this->setCharset(contentAttrEntry.second);
		}
	}
}

void HTML_Parser::foundText(Node node) {
	if (this->searchingCharset || this->isInsideInvalidTag)
		return;

	this->text.append(node.text());
	Util::trim(this->text, " \t\n\r,;|()/\\{}");
	this->text.append(" ");
}

void HTML_Parser::setSearchingCharset(bool searchingCharset) {
	this->searchingCharset = searchingCharset;
}

void HTML_Parser::getText(string&text) {
	text.clear();
	text.assign(this->text);
}

void HTML_Parser::getCharset(string&charset) {
	charset.clear();
	charset.assign(this->charset);
}
