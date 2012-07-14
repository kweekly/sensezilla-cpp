/*
 * logging.h
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#ifndef LOGGING_H_
#define LOGGING_H_

void log(const char* format,...); // for data
void log_i(const char* format,...); // for informational messages
void log_e(const char* format,...); // for errors


#endif /* LOGGING_H_ */
