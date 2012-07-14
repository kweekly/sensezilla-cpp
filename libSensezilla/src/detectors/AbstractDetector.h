/*
 * AbstractDetector.h
 *
 *  Created on: Jul 9, 2012
 *      Author: nerd256
 */

#ifndef ABSTRACTDETECTOR_H_
#define ABSTRACTDETECTOR_H_

template <class E>
class AbstractDetector {
public:
	AbstractDetector() {}
	~AbstractDetector() {}

	virtual EventSeries<E> * detect(TimeSeries * ts) {return new EventSeries<E>();}
};

#endif /* ABSTRACTDETECTOR_H_ */
