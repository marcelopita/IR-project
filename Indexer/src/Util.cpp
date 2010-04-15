/*
 * Util.cpp
 *
 *  Created on: 14/04/2010
 *      Author: 04610922479
 */

#include "../include/Util.h"
#include <sstream>
#include <malloc.h>
#include <fstream>
#include <errno.h>



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

void Util::minus(struct rusage &u1, struct rusage &u2, struct rusage &result) {
	result.ru_utime.tv_sec = u1.ru_utime.tv_sec - u2.ru_utime.tv_sec;

	if (u2.ru_utime.tv_usec > u1.ru_utime.tv_usec) {
		--result.ru_utime.tv_sec;
		result.ru_utime.tv_usec = 1000000 + u1.ru_utime.tv_usec
				- u2.ru_utime.tv_usec;
	} else {
		result.ru_utime.tv_usec = u1.ru_utime.tv_usec - u2.ru_utime.tv_usec;
	}
}

void Util::plus(struct rusage &u1, struct rusage &u2, struct rusage &result) {
	result.ru_utime.tv_sec = u1.ru_utime.tv_sec + u2.ru_utime.tv_sec;
	result.ru_utime.tv_usec = u1.ru_utime.tv_usec + u2.ru_utime.tv_usec;

	if (result.ru_utime.tv_usec >= 1000000) {
		++result.ru_utime.tv_sec;
		result.ru_utime.tv_usec -= 1000000;
	}
}

string Util::getTimeStr(struct rusage& usage) {
	ostringstream timeStr;
	timeStr << ((double) usage.ru_utime.tv_sec
			+ ((double) usage.ru_utime.tv_usec / 1000000.0)) << " s";
	return timeStr.str();
}

/**
 * Register current time and memory allocation.
 */
void Util::saveMemTime(ofstream& memTimeFile, struct rusage& initialTime,
		struct rusage& currentTime) {
	struct rusage elapsedTime;
	Util::minus(currentTime, initialTime, elapsedTime);
	struct mallinfo info = mallinfo();

	ostringstream memTimeStream;
	memTimeStream << Util::getTimeStr(elapsedTime) << ", "
			<< ((double) (info.usmblks + info.uordblks) / 1048576.0) << endl;

	memTimeFile << memTimeStream.str();
}

/**
 * Change content's char set.
 */
int Util::changeCharSet(const string& from, const string& to,
		string& content) {
	char *input = new char[content.size() + 1];
	memset(input, 0, content.size() + 1);
	strncpy(input, content.c_str(), content.size());
	char *inBuf = input;

	char *output = new char[content.size() * 2 + 1];
	memset(output, 0, content.size() * 2 + 1);
	char *outBuf = output;

	size_t inputSize = content.size();
	size_t outputSize = content.size() * 2;

	setlocale(LC_ALL, "");

	iconv_t cd = iconv_open(to.c_str(), from.c_str());

	if (cd == (iconv_t) (-1)) {
		delete[] input;
		delete[] output;
		return DOC_BLOCKED;
	}

	bool error = false;

	do {
		if (iconv(cd, &inBuf, &inputSize, &outBuf, &outputSize)
				== (size_t) (-1)) {
			if (errno == EILSEQ) {
				error = true;
				inputSize--;
				inBuf++;
			} else {
				error = false;
			}

		} else {
			error = false;
		}

	} while (error);

	content.assign(output);

	iconv_close(cd);
	delete[] input;
	delete[] output;

	return DOC_OK;
}
