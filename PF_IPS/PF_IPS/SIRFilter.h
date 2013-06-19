#pragma once
#include "all.h"
#include "PFTypes.h"
#include "ShortestPathMap.h"


template <class X_type,class Y_type,class P_type>
class SIRFilter {
public:
	SIRFilter(const std::vector<X_type> X0, 
				void (*tm)(X_type & state, P_type & params),
				double (*om)(const X_type & in_state, const Y_type & measurement, P_type & params),
				const P_type & parms,
				size_t trajtime = 100	);
	~SIRFilter();

	void step(const Y_type & measurement);
	void stepall(const std::vector<Y_type> measurements);

	std::vector<X_type> state();
	std::vector<double> weights();

private:

	class Particle {
	public:
		X_type state;
		double prob;
		#define NO_PARENT 0xFFFFFFFF
		size_t parent;
	};
	static int particle_comp_fn(const void * p1, const void * p2);

	size_t pcache_time;
	size_t nParticles;
	size_t step_no;
	Particle ** pcache;

	P_type params;

	// User-supplied functions
	void (*transition_model)(X_type & state, P_type & params);
	double (*observation_model)(const X_type & in_state, const Y_type & measurement, P_type & params);
};

#include "SIRFilter.h"

template <class X_type,class Y_type,class P_type> SIRFilter<X_type,Y_type,P_type>::SIRFilter(const std::vector<X_type> X0, 
																							 void (*tm)(X_type & state, P_type & params),
																							 double (*om)(const X_type & in_state, const Y_type & measurement, P_type & params),
																							 const P_type & parms, 
																							 size_t trajtime) {
	params = parms;
	transition_model = tm;
	observation_model = om;
	pcache_time = trajtime;

	nParticles = X0.size();
	pcache = new Particle*[pcache_time];
	for (size_t c = 0; c < pcache_time; c++ ) {
		pcache[c] = new Particle[nParticles];
	}

	step_no = 0;
	for ( size_t c = 0; c < nParticles; c++ ) {
		pcache[0][c].state = X0[c];
		pcache[0][c].prob = 1.0/nParticles;
		pcache[0][c].parent = NO_PARENT;
	}
}

template <class X_type,class Y_type,class P_type> SIRFilter<X_type,Y_type,P_type>::~SIRFilter() {
	for ( size_t c = 0; c < pcache_time; c++ ) {
		delete [] pcache[c];
	}
	delete [] pcache;
}

template <class X_type,class Y_type,class P_type> int SIRFilter<X_type,Y_type,P_type>::particle_comp_fn(const void * p1, const void * p2) {
	double v = ((Particle *)p1)->prob - ((Particle*)p2)->prob;
	if ( v == 0 ) return 0;
	if ( v > 0 ) return 1;
	return -1;
}

template <class X_type,class Y_type,class P_type> void SIRFilter<X_type,Y_type,P_type>::step(const Y_type & measurement) {
	size_t step0 = step_no % pcache_time, step1 = (step_no + 1) % pcache_time;
	step_no++;

	// Discrete re-sampling of particles
	for ( size_t p = 0; p < nParticles; p++ ) {
		pcache[step1][p].prob = rand() / (double)(RAND_MAX); // create new list of random numbers from 0 to 1.0
	}
	qsort(pcache[step0],nParticles,sizeof(Particle),particle_comp_fn); // sort those into order
	size_t p1 = 0;
	double cumsum = 0;
	for ( size_t p2 = 0; p2 < nParticles; p2++ ) {
		while ( pcache[step1][p2].prob > cumsum ) {
			cumsum += pcache[step0][p1].prob;
			p1++;
		}
		pcache[step1][p2].parent = p1;
		pcache[step1][p2].state = pcache[step0][p1].state;
		pcache[step1][p2].prob  = pcache[step0][p1].prob;
	}

	// Prediction
	for ( size_t p = 0; p < nParticles; p++ ) {
		transition_model(pcache[step1][p].state,params);
	}

	// Update importance weights
	for ( size_t p = 0; p < nParticles; p++ ) {
		pcache[step1][p].prob = observation_model(pcache[step1][p].state, measurement, params);
	}

	// Normalize the importance weights
	double psum = 0.0;
	for ( size_t p = 0; p < nParticles; p++ ) {
		psum += pcache[step1][p].prob;
	}
	if (psum == 0.0) {
		for ( size_t p = 0; p < nParticles; p++ ) {
			pcache[step1][p].prob /= psum;
		}
	} else {
		for ( size_t p = 0; p < nParticles; p++ ) {
			pcache[step1][p].prob = 1.0/nParticles;
		}
	}

	// find maximum
	double maxp = 0.0;
	size_t maxidx = NO_PARENT;
	for ( size_t p = 0; p < nParticles; p++ ) {
		if ( pcache[step1][p].prob > maxp ) {
			maxp = pcache[step1][p].prob;
			maxidx = p;
		}
	}
	if ( maxidx == NO_PARENT ) {
		log_i("Warning: No maximum probability particle");
	}
}

template <class X_type,class Y_type,class P_type> void SIRFilter<X_type,Y_type,P_type>::stepall(const std::vector<Y_type> measurements) {
	for (vector<Y_type>::iterator iter = measurements.begin(); iter != measurements.end(); iter++) {
		step(*iter);
	}
}

template <class X_type,class Y_type,class P_type> std::vector<X_type> SIRFilter<X_type,Y_type,P_type>::state() {
	std::vector<X_type> retval;
	retval.reserve(nParticles);
	for (size_t c = 0; c < nParticles; c++ )
		retval.push_back(pcache[step_no % pcache_time][c].state);
	return retval;
}

template <class X_type,class Y_type,class P_type> std::vector<double>SIRFilter<X_type,Y_type,P_type>::weights() {
	std::vector<double> retval;
	retval.reserve(nParticles);
	for (size_t c = 0; c < nParticles; c++ )
		retval.push_back(pcache[step_no % pcache_time][c].weight);
	return retval;
}