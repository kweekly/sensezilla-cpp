/*
 * EventSeries.h
 *
 *  Created on: Jul 9, 2012
 *      Author: nerd256
 */

#ifndef EVENTSERIES_H_
#define EVENTSERIES_H_

template <class E>
class EventSeries {
public:
	EventSeries() {
		events.clear();
	}

	virtual ~EventSeries() {
		events.clear();
	}

	void addEventAtEnd(E event) {
		events.push_back(event);
	}

	int findEvent(double time){
		// do a binary search
		int iA = 0, iB = events.size();
		while ( iA < iB ) {
			int iC = (iA+iB) / 2;
			if ( events[iC].t < time ) {
				iA = iC + 1;
			} else {
				iB = iC;
			}
		}
		return iA;
	}

	E getEvent(double time){
		return events[findEvent(time)];
	}

	EventSeries<E> * selectTime(double t1, double t2){
		EventSeries<E> * esnew = new EventSeries<E>();
		int iA = findEvent(t1);
		for (int c = iA; c < events.size() && events[c].t <= t2; c++) {
			esnew->addEventAtEnd(events[c]);
		}
		return esnew;
	}

	vector<E> events;
};

class Event {
public:
	Event()  {
		t = 0;
	}
	virtual ~Event() {}

	double t;
};

#endif /* EVENTSERIES_H_ */
