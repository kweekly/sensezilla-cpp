/*
 * all.h
 *
 *  Created on: Jun 29, 2012
 *      Author: kweekly
 */

#ifndef ALL_H_
#define ALL_H_

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
// C libraries
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

// C++ classes
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <set>
#include <map>
using namespace std;

// my files
#include "util/logging.h"
#include "util/TimeSeries.h"
#include "util/CSVLoader.h"
#include "util/EventSeries.h"
#include "util/PNGLoader.h"
#include "util/ConfigurationLoader.h"

#include "programs/AbstractProgram.h"
#include "programs/AbstractFilterProgram.h"


#include "filters/AbstractFilter.h"
#include "filters/WindowedFilter.h"
#include "filters/MinFilter.h"
#include "filters/SpikeFilter.h"
#include "filters/DerivativeFilter.h"

#include "detectors/AbstractDetector.h"
#include "detectors/TransitionDetector.h"

// RAND_MAX = 0x7fff, i.e. 15-bits, so we can try to get another 30 bits
#define randDouble() ((((long)rand()<<15)|(rand()))/((double)0x3fffffffL))

#endif /* ALL_H_ */
