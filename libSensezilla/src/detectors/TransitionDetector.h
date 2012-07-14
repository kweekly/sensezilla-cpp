/*
 * TransitionDetector.h
 *
 *  Created on: Jul 9, 2012
 *      Author: nerd256
 */

#ifndef TRANSITIONDETECTOR_H_
#define TRANSITIONDETECTOR_H_


class TransitionEvent : public Event {
public:
	enum TransitionType {
		FALLING = 0,
		RISING = 1
	};
	TransitionType type;

	TransitionEvent(double tm, TransitionType ty) {
		t = tm;
		type = ty;
	}
	virtual ~TransitionEvent(){	}

};

class TransitionDetector : public AbstractDetector<TransitionEvent> {
public:
	TransitionDetector(double thres = 1e-1, double deci = 200);
	virtual ~TransitionDetector();

	void setParams(double th, double deci);

	virtual EventSeries<TransitionEvent> * detect(TimeSeries * ts);
private:
	double thresh;
	double deci;
};


#endif /* TRANSITIONDETECTOR_H_ */
