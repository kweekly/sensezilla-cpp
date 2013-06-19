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
		X_type state;
		double prob;
		#define NO_PARENT 0xFFFFFFFF
		size_t parent;
	};
	int particle_comp_fn(const void * p1, const void * p2);

	size_t pcache_time;
	size_t nParticles;
	size_t step_no;
	Particle ** pcache;

	P_type params;

	// User-supplied functions
	void (*transition_model)(X_type & state, P_type & params);
	double (*observation_model)(const X_type & in_state, const Y_type & measurement, P_type & params);
};