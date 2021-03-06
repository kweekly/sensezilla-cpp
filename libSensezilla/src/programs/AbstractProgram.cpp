#include "all.h"

#if !defined(_WIN32)
#include <sys/time.h>
#include <sys/resource.h>
#endif

AbstractProgram::AbstractProgram() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#if __x86_64__
	rlimit64 lim;
	if ( getrlimit64(RLIMIT_AS, &lim) ){
		log_e("Error getting rlimit %d",errno);
	}
	log_i("Decreasing memory limit from %dB to 2GB",lim.rlim_cur);
	lim.rlim_cur = 2000000000L;
	if ( setrlimit64(RLIMIT_AS, &lim) ) {
		log_e("Error setting rlimit %d",errno);
		exit(1);
	}
#elif !defined(_WIN32)
	rlimit lim;
	if ( getrlimit(RLIMIT_AS, &lim) ){
		log_e("Error getting rlimit %d",errno);
	}
	log_i("Decreasing memory limit from %dB to 2GB",lim.rlim_cur);
	lim.rlim_cur = 2000000000L;
	if ( setrlimit(RLIMIT_AS, &lim) ) {
		log_e("Error setting rlimit %d",errno);
		exit(1);
	}
#endif
#endif
}

AbstractProgram::~AbstractProgram() {}

void AbstractProgram::parseCL(int argc, char * const * argv){
	string option, value;
	bool got_opt = false;
	bool error = false;
	for ( int c = 1; c < argc; c++ ) {
		if (argv[c][0] == '-' && !isdigit(argv[c][1])) {
			if ( got_opt ) {
				if ( !processCLOption(option, string("")) ) {
					log_e("Unknown option -%s",option.c_str());
					error = true;
				}
			}
			option.assign(argv[c] + 1);
			got_opt = true;
		} else {
			if ( got_opt ) {
				value.assign(argv[c]);
				if ( !processCLOption(option,value) ) {
					log_e("Unknown option -%s",option.c_str());
				    error = true;
				}
				got_opt = false;
			} else {
				option.assign(argv[c]);
				log_e("Unknown option \"%s\"",option.c_str());
				error = true;
			}
		}
	}
	if ( got_opt ) {
		if ( !processCLOption(option, string("")) ) {
			log_e("Unknown option -%s",option.c_str());
			error = true;
		}
	}
	if ( error ){
		printHelp();
		exit(1);
	}
}

bool AbstractProgram::processCLOption( string opt, string val ) {
	return false;
}

void AbstractProgram::printHelp() {
	log_i(
"Global Options\n"
"\t-cfile : Configuration file to use\n"
"\t-help  : Display help\n"
	);
}

void AbstractProgram::start() {
}


