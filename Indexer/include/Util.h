/*
 * Util.h
 *
 *  Created on: 14/04/2010
 *      Author: 04610922479
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <string>
#include <vector>

using namespace std;

class Util {

public:

	Util();

	~Util();

	static void trim(std::string&, const std::string&);

	static void tokenize(const string&, vector<string>&, const string&, bool);

};

#endif /* UTIL_H_ */
