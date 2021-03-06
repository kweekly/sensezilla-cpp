/*
 * logging.h
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#ifndef LOGGING_H_
#define LOGGING_H_

void log_i(const char* format,...); // for informational messages
void log_e(const char* format,...); // for errors

void log_prog(int steps_done, int steps_complete, char * step_name, const char * step_prog, ...);

#endif /* LOGGING_H_ */
