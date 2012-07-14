/*
 * all.h
 *
 *  Created on: Jun 29, 2012
 *      Author: kweekly
 */

#ifndef ALL_H_
#define ALL_H_

// C libraries
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// C++ classes
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace std;

// my files
#include "util/logging.h"
#include "util/TimeSeries.h"
#include "util/CSVLoader.h"
#include "util/EventSeries.h"

#include "programs/AbstractProgram.h"
#include "programs/AbstractFilterProgram.h"


#include "filters/AbstractFilter.h"
#include "filters/WindowedFilter.h"
#include "filters/MinFilter.h"
#include "filters/SpikeFilter.h"
#include "filters/DerivativeFilter.h"

#include "detectors/AbstractDetector.h"
#include "detectors/TransitionDetector.h"
#endif /* ALL_H_ */
