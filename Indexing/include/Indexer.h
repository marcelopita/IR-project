/*
 * Indexer.h
 *
 *  Created on: 23/03/2010
 *      Author: pita
 */

#ifndef INDEXER_H_
#define INDEXER_H_

class Indexer {
public:
	Indexer();
	virtual ~Indexer();
private:
	char **vocabulary;
	char **documents;
};

#endif /* INDEXER_H_ */
