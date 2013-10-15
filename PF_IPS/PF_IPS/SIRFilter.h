#pragma once
#include "all.h"
#include "PFTypes.h"
#include "ShortestPathMap.h"
#include <thread>
#include <mutex>


//#define num_threads 12
#define SIRFN(X) template <class X_type,class Y_type,class P_type> X SIRFilter<X_type,Y_type,P_type>

template <class X_type,class Y_type,class P_type>
class SIRFilter {
public:
	SIRFilter(const std::vector<X_type> X0, 
				void (*tm)(X_type & state, P_type & params),
				double (*om)(const X_type & in_state, const Y_type & measurement, P_type & params),
				const P_type & parms,
				size_t trajtime = 100, size_t nthreads=12	);
	~SIRFilter();

	void set_reposition_ratio(double ratio, void (*repositioner)(X_type & state, P_type &params));


	X_type step(const Y_type & measurement);
	std::vector<X_type> stepall(const std::vector<Y_type> measurements);


	std::vector<X_type> state();
	std::vector<double> weights();

private:
	friend class Visualization;

	class Particle {
	public:
		double prob;
		X_type state;
		#define NO_PARENT 0xFFFFFFFF
		size_t parent;
	};
	static int particle_comp_fn(const void * p1, const void * p2);
	
	size_t num_threads;

	size_t pcache_time;
	size_t nParticles;
	double reposition_ratio;
	size_t step_no;
	Particle ** pcache;
	Particle * maxp_cache;
	Particle * pscratch;

	P_type params;

	// User-supplied functions
	void (*transition_model)(X_type & state, P_type & params);
	double (*observation_model)(const X_type & in_state, const Y_type & measurement, P_type & params);
	void (*repositioner)(X_type & state, P_type & params);

	// excecution threads
	class ThreadReturn {
	public:
		double psum;
		int n;
		int maxp;
	};
	ThreadReturn * thread_retvals;

	enum WorkStep {
		QSORT, PREDUPDATE
	} work_step;

	bool program_alive;

	thread * thread_pool;
	void _worker_thread(int thread_no);
	void _run_workers(WorkStep ws);
	
	mutex * thread_waiting_mutex; // master unlocks this to signal that step should start
	mutex * thread_waiting_ack_mutex; // child locks this until it acknowleges the step start signal
	mutex * thread_working_mutex; // child locks this to indicate that it is working
	
	
	const Y_type * current_measurement;
};

#include "SIRFilter.h"

SIRFN( )::SIRFilter(const std::vector<X_type> X0, 
					void (*tm)(X_type & state, P_type & params),
					double (*om)(const X_type & in_state, const Y_type & measurement, P_type & params),
					const P_type & parms, 
					size_t trajtime,size_t nThreads) {
	params = parms;
	transition_model = tm;
	observation_model = om;
	pcache_time = trajtime;
	num_threads = nThreads;

	thread_pool = new thread[num_threads];
	thread_retvals = new ThreadReturn[num_threads];
	thread_waiting_mutex = new mutex[num_threads];
	thread_waiting_ack_mutex = new mutex[num_threads];
	thread_working_mutex = new mutex[num_threads];

	nParticles = X0.size();
	pcache = new Particle*[pcache_time];
	for (size_t c = 0; c < pcache_time; c++ ) {
		pcache[c] = new Particle[nParticles];
	}
	maxp_cache = new Particle[pcache_time];

	pscratch = new Particle[nParticles];

	step_no = 0;
	for ( size_t c = 0; c < nParticles; c++ ) {
		pcache[1][c].state = X0[c];
		pcache[1][c].prob = 1.0/nParticles;
		pcache[1][c].parent = NO_PARENT;
	}

	program_alive = true;
	for (size_t c = 0; c < num_threads; c++ ) {
		thread_waiting_mutex[c].lock();
		thread_pool[c] = thread(&SIRFilter::_worker_thread, this, c);
		std::this_thread::yield();
		thread_working_mutex[c].lock();
		thread_working_mutex[c].unlock();
	}

	this->repositioner = NULL;
}

SIRFN()::~SIRFilter() {
	program_alive = false;
	for ( size_t t = 0; t < num_threads; t++ ) {
		thread_waiting_mutex[t].unlock();
	}

	for ( size_t c = 0; c < pcache_time; c++ ) {
		delete [] pcache[c];
	}
	delete [] pcache;
	
	delete [] pscratch;

	for ( size_t t = 0; t < num_threads; t++ ) {
		thread_pool[t].join();
	}

	delete [] thread_pool;
	delete [] thread_retvals;
	delete [] thread_waiting_mutex;
	delete [] thread_waiting_ack_mutex;
	delete [] thread_working_mutex;
}

SIRFN(void)::set_reposition_ratio(double ratio, void (*repositioner)(X_type & state, P_type &params)) {
	reposition_ratio = ratio;
	this->repositioner = repositioner;
}

SIRFN(int)::particle_comp_fn(const void * p1, const void * p2) {
	double v = ((Particle *)p1)->prob - ((Particle*)p2)->prob;
	if ( v == 0 ) return 0;
	if ( v > 0 ) return 1;
	return -1;
}

SIRFN(void)::_worker_thread(int thread_no) {
	size_t step0, step1;
	size_t start, end;
	thread_working_mutex[thread_no].lock();

	start = (int)(thread_no * ((double)nParticles / num_threads));
	if (thread_no == num_threads-1)
		end = nParticles;
	else
		end = (int)((thread_no+1) * ((double)nParticles / num_threads));

	log_i("Worker Thread %d started. Responsible for %d to %d (%d elements)", thread_no,start,end,(end-start));
	
	thread_retvals[thread_no].n = start-end;

	while (true) {
		thread_waiting_ack_mutex[thread_no].lock(); // should get this for free
		thread_working_mutex[thread_no].unlock(); // done working, ready for next step
		thread_waiting_mutex[thread_no].lock(); // wait for step to start
		if ( !program_alive ) break;
		//log_i("\tWT %d : Step %d",thread_no,step_no);

		thread_working_mutex[thread_no].lock(); // lock our working mutex
		thread_waiting_ack_mutex[thread_no].unlock(); // acknowleged that the waiting mutex was acknowleged
		thread_waiting_mutex[thread_no].unlock(); // we have successfully locked the working mutex

		step0 = step_no % pcache_time;
		step1 = (step_no + 1) % pcache_time;		
		
		// do work ...
		switch(work_step){
		case QSORT:
			qsort(&pcache[step1][start],end-start,sizeof(Particle),particle_comp_fn); // sort chunk in order
			for ( size_t p = start; p < end; p++ ) { // put into temporary array
				pscratch[p] = pcache[step1][p];
			}
			break;
		case PREDUPDATE:
			thread_retvals[thread_no].psum = 0;
			thread_retvals[thread_no].maxp = start;
			for (size_t p = start; p < end; p++) {
				// prediction
				if ( repositioner != NULL && randDouble() < reposition_ratio ) {
					State s = pcache[step1][p].state;
					repositioner(s,params);
					/*randDouble();
					randDouble();
					randDouble();*/
					//transition_model(pcache[step1][p].state,params);
				} else {
					transition_model(pcache[step1][p].state,params);
				}

				// update importance weights
				pcache[step1][p].prob = observation_model(pcache[step1][p].state, *current_measurement, params);

				// update sum of probabilities
				thread_retvals[thread_no].psum += pcache[step1][p].prob;

				// update current maximum
				if ( pcache[step1][p].prob > pcache[step1][thread_retvals[thread_no].maxp].prob )
						thread_retvals[thread_no].maxp = p;
			}
			
			break;
		default:
			break;
		}
	}
	thread_waiting_ack_mutex[thread_no].unlock();
	thread_waiting_mutex[thread_no].unlock();
	log_i("Worker Thread %d ended",thread_no);
}

SIRFN(void)::_run_workers(WorkStep ws){
	work_step = ws;
	for ( size_t t = 0; t < num_threads; t++ ) {
		thread_waiting_mutex[t].unlock();  // signal that child can continue
		thread_waiting_ack_mutex[t].lock(); // child has acknowleged that it can continue
		thread_waiting_mutex[t].lock(); // lock to prevent child from continuing next step
		thread_waiting_ack_mutex[t].unlock(); // prepare for next step
	}
	
	// wait for threads to finish
	for ( size_t t = 0; t < num_threads; t++ ) {
		thread_working_mutex[t].lock();
		thread_working_mutex[t].unlock();
	}
}

SIRFN(X_type)::step(const Y_type & measurement) {
	size_t step0, step1;

	current_measurement = &measurement;
	step_no++;

	step0 = step_no % pcache_time;
	step1 = (step_no + 1) % pcache_time;


	// Discrete re-sampling of particles, however we reserve some to be repositioned
	
	for (size_t p = 0; p < nParticles; p++ ) {
		pcache[step1][p].prob = randDouble(); // create new list of random numbers from 0 to 1.0
	}
	
	// sort step1 in order using worker threads
	_run_workers(QSORT);
	// merge the subarrays
	// array has been copied into temporary array by the worker thread
	size_t * ptrs = new size_t[num_threads];
	for ( size_t t = 0; t < num_threads; t++ ) {
		ptrs[t] = (int)(t * ((double)nParticles / num_threads));
	}
	for ( size_t p = 0; p < nParticles; p++ ) {
		double minv = 9e99;
		size_t mint = 0;
		for ( size_t t = 0; t < num_threads; t++ ) {
			size_t end = (int)((t+1) * ((double)nParticles / num_threads));
			if ( ptrs[t] < end && pscratch[ptrs[t]].prob < minv ) {
				mint = t;
				minv = pscratch[ptrs[t]].prob;
			}
		}
		pcache[step1][p] = pscratch[ptrs[mint]];
		ptrs[mint]++;
	}
	delete [] ptrs;

	// array of pcache[step1][p] now contains random numbers sorted ascending
	size_t p1 = 0;
	// skip 0 probs
	double cumsum = 0;
	for ( size_t p2 = 0; p2 < nParticles; p2++ ) {
		while ( pcache[step1][p2].prob > cumsum && p1 < nParticles ) {
			cumsum += pcache[step0][p1].prob;
			p1++;
		}
		if ( p1 >= nParticles ) p1 = nParticles-1;
		pcache[step1][p2].parent = p1-1;
		pcache[step1][p2].state = pcache[step0][p1-1].state;
		pcache[step1][p2].prob  = pcache[step0][p1-1].prob;
	}

	// Start worker threads on prediction and update steps, they will also calculate psums and maximum prob. particles
	_run_workers(PREDUPDATE);

	// Normalize the importance weights
	double psum = 0.0;
	for ( size_t t = 0; t < num_threads; t++ ) {
		psum += thread_retvals[t].psum;
	}
	if (psum != 0.0) {
		for ( size_t p = 0; p < nParticles; p++ ) {
			pcache[step1][p].prob /= psum;
		}
	} else {
		log_e("Particles died, renormalize");
		for ( size_t p = 0; p < nParticles; p++ ) {
			pcache[step1][p].prob = 1.0/nParticles;
		}
	}

	
	// find maximum prob particle
	size_t maxidx = 0;
	for ( size_t t = 0; t < num_threads; t++ ) {
		if ( pcache[step1][thread_retvals[t].maxp].prob > pcache[step1][maxidx].prob ) {
			maxidx = thread_retvals[t].maxp;
		}
	}
	if ( maxidx == NO_PARENT ) {
		log_i("Warning: No maximum probability particle");
	}

	maxp_cache[step1] = pcache[step1][maxidx];

	return pcache[step1][maxidx].state;
}

SIRFN(vector<X_type>)::stepall(const std::vector<Y_type> measurements) {
	vector<X_type> retval;
	retval.reserve(measurements.size());
	for (vector<Y_type>::iterator iter = measurements.begin(); iter != measurements.end(); iter++) {
		retval.push_back(step(*iter));
	}
	return retval;
}

SIRFN(vector<X_type>)::state() {
	std::vector<X_type> retval;
	retval.reserve(nParticles);
	for (size_t c = 0; c < nParticles; c++ )
		retval.push_back(pcache[step_no % pcache_time][c].state);
	return retval;
}

SIRFN(vector<double>)::weights() {
	std::vector<double> retval;
	retval.reserve(nParticles);
	for (size_t c = 0; c < nParticles; c++ )
		retval.push_back(pcache[step_no % pcache_time][c].prob);
	return retval;
}