/*
 * Util.cpp
 *
 *  Created on: 14/04/2010
 *      Author: 04610922479
 */

#include "../include/Util.h"

Util::Util() {
	// TODO Auto-generated constructor stub

}

Util::~Util() {
	// TODO Auto-generated destructor stub
}

void Util::trim(std::string &str, const std::string &whitespaces = " ") {
	std::string::size_type pos = str.find_last_not_of(whitespaces);
	if (pos != std::string::npos) {
		str.erase(pos + 1);
		pos = str.find_first_not_of(whitespaces);
		if (pos != std::string::npos)
			str.erase(0, pos);
	} else
		str.erase(str.begin(), str.end());
}

void Util::tokenize(const string& str, vector<string>& tokens,
		const string& delimiters = " ", bool lowerCase = false) {
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos) {
		if (!lowerCase) {
			tokens.push_back(str.substr(lastPos, pos - lastPos));
		} else {
			string subStr = str.substr(lastPos, pos - lastPos);
			transform(subStr.begin(), subStr.end(), subStr.begin(),
					(int(*)(int)) tolower);
			tokens.push_back(subStr);
		}
		lastPos = str.find_first_not_of(delimiters, pos);
		pos = str.find_first_of(delimiters, lastPos);
	}
}
