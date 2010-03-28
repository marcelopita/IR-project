/**
 * Document class implementation
 * @author: anisio@dcc.ufmg.br
 */

#include "Document.h"

using namespace std;
using namespace RICPNS;

Document::Document() 
	: url_(), 
		text_(), 
		length_(0) {
}

Document::Document(const std::string & url, 
									 const std::string & text,
									 const size_t & length)
	: url_(url), 
		text_(text), 
		length_(0) {
}

Document::~Document() {}

void Document::setURL(const std::string & url) {
	url_ = url;
}

string Document::getURL() const {
	return url_;
}

void Document::setText(const std::string & text) {
	text_ = text;
}

string Document::getText() const {
	return text_;
}

void Document::setLength(const size_t & length) {
	length_ = length;
}

size_t Document::getLength() const {
	return length_;
}

void Document::clear() {
	setURL("");
	setText("");
	setLength(0);
}

