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
#include <sys/time.h>
#include <sys/resource.h>

using namespace std;

class Util {

public:

	static void trim(std::string&, const std::string&);

	static void tokenize(const string&, vector<string>&, const string&, bool);

	static void minus(struct rusage&, struct rusage&, struct rusage&);

	static void plus(struct rusage&, struct rusage&, struct rusage&);

	static void saveMemTime(ofstream&, struct rusage&, struct rusage&, int, int);

	static string getTimeStr(struct rusage& usage);

	static int changeCharSet(const string&, const string&, string&);

	static void removeURIUnprintableChars(string& uri);

	const static int DOC_OK = 0;

	const static int DOC_BLOCKED = 1;

};

#endif /* UTIL_H_ */
