/*
 * logging.c
 *
 *  Created on: Jul 3, 2012
 *      Author: nerd256
 */

#include "all.h"

void log( const char* format,...) {
    va_list args;
    va_start( args, format );
    vfprintf( stdout, format, args );
    va_end( args );
    fprintf( stdout, "\n" );
}

void log_i( const char* format,...) {
    va_list args;
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );
    fprintf( stderr, "\n" );
}

void log_e( const char* format,...) {
    va_list args;
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );
    fprintf( stderr, "\n" );
}

void log_prog(int steps_done, int steps_complete, char * step_name, const char * step_prog, ...) {
	va_list args;
	fprintf(stdout,"PROGRESS STEP %d OF %d \"%s\" ",steps_done,steps_complete,step_name);
	va_start( args, step_prog );
	vfprintf(stdout,step_prog,args);
	va_end(args);
	fprintf(stdout," DONE\n");
}

