/*
 * AbstractProgram.h
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#ifndef ABSTRACTPROGRAM_H_
#define ABSTRACTPROGRAM_H_

class AbstractProgram {
public:
	AbstractProgram();
	virtual ~AbstractProgram();

	void parseCL(int argc, char * const * argv);

	virtual bool processCLOption( string opt, string val );
	virtual void start();
	virtual void printHelp();


};


#endif /* ABSTRACTPROGRAM_H_ */
