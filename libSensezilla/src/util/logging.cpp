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
