#include "PF_IPS.h"
#include "SIRFilter.h"



void PF_IPS::_sirFilter() {
	
	// do the processing
	log_i("\nGenerating initial States");
	vector<State> X0;
	for ( int c = 0; c < nParticles; c++ ) {
		State s;
		s.pos.x = randDouble() * (maxx-minx) + minx;
		s.pos.y = randDouble() * (maxy-miny) + miny;
		X0.push_back(s);
	}

	Params p;
	p.context = this;
	filter = new SIRFilter<State,Observation,Params>(X0,PF_IPS::transition_model,PF_IPS::observation_model,p);
	Observation o;
	time_t last_update, cur_time;
	int last_step_no = 0;
	const int UPDATE_INTERVAL = 1;
	time(&last_update);
	for ( int step_no = 0; step_no < rssi_data[0]->t.size(); step_no++ ) {
		TIME = rssi_data[0]->t[step_no];
		o.rfid_system_rssi_measurements.clear();
		for ( int sid = 0; sid < rssi_data.size(); sid++ ) {
			o.rfid_system_rssi_measurements.push_back(rssi_data[sid]->v[step_no]);
		}
		// display info in the console
		time(&cur_time);
		if ( cur_time - last_update >= UPDATE_INTERVAL ) {
			log_i("Step %d of %d. %.2f steps/s",step_no+1,rssi_data[0]->t.size(),((double)step_no-last_step_no)/((double)cur_time-last_update));
			last_update = cur_time;
			last_step_no = step_no;
		}
		
		// do the step
		current_best_state = filter->step(o);

		// find "maximum probability" particle
		vector<State> states = filter->state();
		vector<double> weights = filter->weights();

		double sumx = 0,sumy = 0,sumw = 0;
		for ( int c = 0; c < states.size(); c++ ) {
			if ( !_isnan(states[c].pos.x + states[c].pos.y + weights[c]) ) {
				
				sumx += states[c].pos.x * weights[c];
				sumy += states[c].pos.y * weights[c];
				sumw += weights[c];
				
				/*
				sumx += states[c].pos.x;
				sumy += states[c].pos.y;
				sumw += 1; 
				*/
			}
		}
		if (sumw > 0) {
			current_best_state.pos.x = sumx / sumw;
			current_best_state.pos.y = sumy / sumw;
		}
		double mindist = 9e99;
		int minidx = 0;
		for ( int c = 0; c < states.size(); c++ ) {
			if ( dist(states[c].pos, current_best_state.pos) < mindist ) {
				minidx = c;
				mindist = dist(states[c].pos, current_best_state.pos);
			}
		}
		current_best_state = states[minidx];
		current_best_state_index = minidx;

		// check if visualizer wants access
		if ( req_data_lock ) {
			data_locked_mutex.unlock();
			data_locked_ack_mutex.lock(); // wait for visualizer to acknowlege that it locked the data mutex
			data_locked_ack_mutex.unlock();
			data_locked_mutex.lock(); // relock the data
		}
	}
	delete filter;
}


double tsSTD(TimeSeries *ts) {
	double ssq = 0;
	double mean = ts->mean();
	for (size_t c = 0; c < ts->v.size(); c++ ) {
		ssq += pow(ts->v[c]-mean,2);
	}
	return ssq / ts->v.size();
}

double dist(xycoords x1, xycoords x2) {
	return sqrt((x1.x-x2.x)*(x1.x-x2.x) + (x1.y-x2.y)*(x1.y-x2.y));
}

void PF_IPS::transition_model(State & state, Params & params) {
	ShortestPathMap * pathmap = params.context->pathmap;
	Grid grid = params.context->grid;
	
	gridcoords hc = grid.nearestCell(state.pos);
	gridcoords hc2 = pathmap->getRandomCellInRange(hc);
	if ( hc2.i < 0 || hc2.j < 0 ) { // must have gone out of range or into an obstacle
		state.pos.x = _Nan._Double;
		state.pos.y = _Nan._Double;
		return;
	}
	
	xycoords xy2 =  grid.getCenter(hc2);

	xy2.x += randDouble()*grid.W - grid.W/2;
	xy2.y += randDouble()*grid.W - grid.W/2;
	
	state.pos = xy2;
}

double PF_IPS::observation_model(const State & in_state, const Observation & measurement, Params & params) {
	double prob = 1.0-randDouble()*0.001;
	if ( _isnan(in_state.pos.x) || _isnan(in_state.pos.y) ) {
		return 0.0;
	}

	for (size_t sensor_idx = 0; sensor_idx < measurement.rfid_system_rssi_measurements.size(); sensor_idx++ ) {
		if ( !_isnan(measurement.rfid_system_rssi_measurements[sensor_idx]) ) {
			RSSISensor * sensor = &(params.context->sensors[sensor_idx]);
			double d = dist(in_state.pos,sensor->pos);
			double mu,sigma;
			
			_get_gaussian_parameters(sensor, d, mu, sigma);

			// for gaussian distribution (normal PDF)
			double mudiff = (measurement.rfid_system_rssi_measurements[sensor_idx]-mu);
			prob *= 1.0/(sigma*sqrt(2*M_PI)) * exp( -mudiff*mudiff / (2*sigma*sigma) );
			
		}
	}
	return prob;
}

void PF_IPS::_get_gaussian_parameters(RSSISensor * sensor, double d, double & mu, double & sigma) {
		int didx = 0;
		mu = sigma = 0;
		while( didx < sensor->gauss_calib_r.size() && d >= sensor->gauss_calib_r[didx] ) {
			didx++;
		}
	
		if ( didx >= sensor->gauss_calib_r.size() - 1 ) {
			mu = sensor->gauss_calib_mu.back();
			sigma = sensor->gauss_calib_sigma.back();
		} else if ( d < sensor->gauss_calib_r[0] ) {
			mu = sensor->gauss_calib_mu[0];
			sigma = sensor->gauss_calib_sigma[0];
		} else {
			double dd = (d - sensor->gauss_calib_r[didx])/(sensor->gauss_calib_r[didx+1] - sensor->gauss_calib_r[didx]);
			mu = dd*sensor->gauss_calib_mu[didx+1] + (1-dd)*sensor->gauss_calib_mu[didx];
			sigma = dd*sensor->gauss_calib_sigma[didx+1] + (1-dd)*sensor->gauss_calib_sigma[didx];
		}
}