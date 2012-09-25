/*
 * library_builder.cpp
 *
 *  Created on: Jul 9, 2012
 *      Author: nerd256
 */
#include "all.h"
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/io.hpp>

#define bvector boost::numeric::ublas::vector<double>
#define bmatrix boost::numeric::ublas::matrix<double>
#define scalarmatrix(r,c,v) boost::numeric::ublas::scalar_matrix<double>(r,c,v)
#define scalarvector(r,v) boost::numeric::ublas::scalar_vector<double>(r,v)
using namespace boost::numeric::ublas;

int error = 0;

class HMMObserver : public AbstractProgram {
public:
	HMMObserver();
	virtual ~HMMObserver();

	virtual void printHelp();
	virtual void start();

	bool processCLOption(string opt, string val);

private:
	int outprec;

	string csvin_fname;
	string csvout_fname;
	string statesin_fname;

	bool use_csv;
};

HMMObserver::HMMObserver() {
	csvin_fname.clear();
	csvout_fname.clear();
	statesin_fname.clear();
	use_csv = false;
	outprec = 13;
}
HMMObserver::~HMMObserver() {

}

bool HMMObserver::processCLOption(string opt, string val) {
	if (opt == "csvin") {
		use_csv = true;
		csvin_fname = val;;
		return true;
	}
	else if ( opt == "outprec" ) {
		outprec = atoi(val.c_str());
		return true;
	} else if ( opt == "csvout") {
		csvout_fname = val;
		return true;
	} else if ( opt == "statesin") {
		statesin_fname = val;
		return true;
	}

	return AbstractProgram::processCLOption(opt, val);
}

void HMMObserver::printHelp() {
	log_i(
			"HMM Observer\n"
			"Kevin Weekly\n"
			"(port of BM_HMM_EM_FB.m by Zhaoyi Kang)"
			"\n"
			"\t-statesin : Input state definitions\n"
			"\t-csvin  : Input CSV file\n"
			"\t-csvout : Output CSV file\n"
			"\t-outprec :Output precision\n"
			);
	AbstractProgram::printHelp();
}

TransitionDetector TRANS_DETECTOR;

bool cmp_by_timeseries_length(TimeSeries * ts1, TimeSeries * ts2) {
	return (ts1->t.back() - ts1->t.front()) > (ts2->t.back() - ts2->t.front());
}

void HMMObserver::start() {
	TimeSeries * inputTS = NULL;

	bvector state_emission_means;
	bvector state_emission_vars;

	double sum;

	int numSeq = 5;						// Number of EM steps
	double epsilon = 0.0001;				// Log-liklihood threshold
	int nStates = 0;
	int T = 0;

	if (statesin_fname.size() > 0) {
		std::vector<TimeSeries *> statesin = CSVLoader::loadMultiTSfromCSV(statesin_fname);
		if ( statesin.size() != 3) {
			log_e("States defn. file must have 4 columns. Stop.");
			return;
		}
		state_emission_means.resize(statesin[0]->t.size());
		state_emission_vars.resize(statesin[0]->t.size());
		for (int c = 0; c < statesin[0]->t.size(); c++) {
			state_emission_means[c] = statesin[1]->v[c];
			state_emission_vars[c] = statesin[2]->v[c];
		}
		nStates = statesin.size();

		while(statesin.size() > 0) {
			delete statesin.back();
			statesin.pop_back();
		}
	} else {
		log_e("Input states file not given. Stop.");
		return;
	}

	if (use_csv) {
		inputTS = CSVLoader::loadTSfromCSV(csvin_fname);
		T = inputTS->t.size();
	} else {
		log_e("No input source defined. Stop.");
		return;
	}

#define test(X) (inputTS->v[(X)])
/*
 *  %transition matrix initialization
    CHG = cell(Ndev,1);
    for q = 1:Ndev
        CHG{q} = [0.999 0.001; 0.001 0.999];
    end;
    A = ones(m,m);
    for l = 1:m
        for w = 1:m
            for q = 1:Ndev
                A(l,w) = A(l,w)*CHG{q}(M(w,q)+1,M(l,q)+1);
            end;
        end;
    end;
 */
	double transprob = 0.001;

	bmatrix A(nStates,nStates);
	for (int i = 0; i < nStates; i++)
		for ( int j = 0; j < nStates; j++ )
			A(i,j) = 1.0 / nStates;

   //     L_t = zeros(m, T);
   //     F_t = zeros(m, T);

	bmatrix L_t(nStates, T);
	bmatrix F_t(nStates, T);
	TimeSeries * STATE_level = inputTS->copy();

	double last_likelihood = 0;

	for ( int seq_no = 0; seq_no < numSeq ; seq_no++ ) {
		log_i("Starting EM step");
		std::cout << "\tTransition Matrix:" << A << endl;
		std::cout << "\tEmission Means:" << state_emission_means << endl;
		std::cout << "\tEmission Variance:" << state_emission_vars << endl;

		// TP = 1./sqrt( M*Q+BGN ).*exp(-1/2*( test(1)-M*Beta-Beta_GND ).^2./( M*Q+BGN )).*(1/m*ones(m, 1));
		// L_t(:,1) = TP/sum(TP);
		sum = 0;
		for ( int s = 0; s < nStates; s++) {
			L_t(s,0) = 1.0/sqrt( state_emission_vars[s] )*exp(-0.5*pow( test(1)- state_emission_means[s],2 )/( state_emission_vars[s] ))/nStates;
			sum += L_t(s,0);
		}
		column(L_t,1) /= sum;

		// F_t(:,T) = 1/m*ones(m, 1);
		column(F_t,T-1) = scalarvector(nStates,1.0/nStates);

		// for t=2:T
		for (int t = 1; t < T; t++) {
            // TP = 1./sqrt( M*Q+BGN ).*exp(-1/2*( test(t)-M*Beta-Beta_GND ).^2./( M*Q+BGN )).*(A'*L_t(:,t-1));
            // L_t(:,t) = TP/sum(TP);
			sum = 0;
			for ( int s = 0; s < nStates; s++ ) {
				L_t(s,t) = 1.0/sqrt( state_emission_vars[s] )*exp(-0.5*pow( test(t)- state_emission_means[s],2 )/( state_emission_vars[s] ))/(inner_prod(column(A, s),column(L_t,t-1)));
				sum += L_t(s,t);
			}
			column(L_t,t) /= sum;

			// tb = T - t + 1;
			int tb = T - 1 - t;

            // TP = A*(F_t(:,tb+1).*(1./sqrt( M*Q+BGN ).*exp(-1/2*( test(tb+1)-M*Beta-Beta_GND ).^2./( M*Q+BGN ))));
            // F_t(:,tb) = TP/sum(TP);
			sum = 0;
			for ( int s = 0; s < nStates; s++ ) {
				F_t(s,tb) = (F_t(s,tb+1) * (1.0/sqrt( state_emission_vars[s] )*exp(-0.5*pow( test(tb + 1)- state_emission_means[s],2 )/( state_emission_vars[s] ))));
			}
			column(F_t,tb) = prod(A,column(F_t,tb));
			for ( int s = 0; s < nStates; s++ ) sum += F_t(s,tb);
			column(F_t,tb) /= sum;

		}


		/*
		 B_t = L_t.*F_t;
		 B_normal = sum(B_t);
		 for i = 1:m
			 B_t(i,:) = B_t(i,:)./B_normal;
		 end;
		 */

		bmatrix B_t = element_prod(L_t,F_t);
		for ( int r = 0; r < nStates; r++) {
			double sum = 0;
			for ( int col = 0; col < T; col++ ) {
				sum += B_t(r,col);
			}
			row(B_t,r) /= sum;
		}
		// [junk, STATE_level] = max(B_t);
		for ( int time = 0 ; time < T; time ++ ) {
			int maxi = 0;
			for ( int state = 0; state < nStates; state++) {
				if ( B_t(state,time) > B_t(maxi,time) ) {
					maxi = state;
				}
			}
			STATE_level->v[time] = maxi;
		}


        //% Update Parameters
        //B_p = (M'*B_t*(test-Beta_GND))./sum(M'*B_t, 2);
		// (Update for state_emission_means)
		bvector state_emission_means_update(nStates);
		for ( int s = 0; s < nStates; s++) {
			double denom = 0;
			state_emission_means_update[s] = 0;
			for (int t = 0; t < T; t++) {
				state_emission_means_update[s] += B_t(s,t) * test(t);
				denom += B_t(s,t);
			}
			state_emission_means_update[s] /= denom;
		}
		state_emission_means.swap(state_emission_means_update);

        //TP = zeros(m, T);
        //BTP = M*Beta;
        //QTP = M*Q;
        //for i = 1:m
            //TP(i,:) = test'-BTP(i);
        //end;
		//Q_p = sum(B_t.*(TP.^2), 2)./sum(B_t, 2);
        bvector state_emission_vars_update(nStates);
        for ( int s = 0; s < nStates; s++ ) {
        	double num = 0, denom = 0;
        	for (int t = 0; t < T; t++) {
        		num += B_t(s,t) * pow(test(t)-state_emission_means[s],2);
        		denom += B_t(s,t);
        	}
        	state_emission_vars_update[s] = num / denom;
        }
        state_emission_vars.swap(state_emission_vars_update);

        //A_p = A.*(L_t*(B_t')) + round(0.01/(m-1)*0.1*T*ones(m,m)) + round((0.99-0.01/(m-1))*0.1*T*eye(m,m));
        //for i = 1:m
        //    A_p(i,:) = A_p(i,:)/sum(A_p(i,:));
        //end;
        bmatrix A_update = element_prod(A,prod(L_t,trans(B_t)));
        for ( int s1 = 0; s1 < nStates; s1++ ) {
        	for ( int s2 = 0; s2 < nStates; s2++ ) {
        		A_update(s1,s2) += round(0.01/(nStates-1)*0.1*T);
        	}
        	A_update(s1,s1) += round((0.99-0.01/(nStates-1))*0.1*T);
        }
        for ( int s1 = 0; s1 < nStates; s1++ ) {
        	double sum = 0;
        	for ( int s2 = 0; s2 < nStates; s2++ ) {
        		sum += A_update(s1,s2);
        	}
        	row(A_update,s1) /= sum;
        }
        A.swap(A_update);

        //for i = 1:m
        //    TP(i,:) = 1/2*log( QTP(i)+BGN ) + 1/2*( test'-BTP(i)-Beta_GND ).^2/( QTP(i)+BGN );
        //end;
        //Likelihood = sum(sum(A.*(L_t*(B_t')).*(-log(A)))) + sum(sum(B_t.*TP, 2));

        double likelihood = 0;
        bmatrix LB = prod(L_t,trans(B_t));
        for ( int s = 0; s < nStates; s++ ) {
        	for (int t = 0; t < T; t++ ) {
        		double tp = 0.5*log( state_emission_vars[s])  + 0.5 * pow( test(t) - state_emission_means[s] ,2 )/(state_emission_vars[s]);
        		likelihood += tp * B_t(s,t);
        	}
        	for ( int s2; s2 < nStates; s2++ ) {
        		likelihood += A(s,s2)*LB(s,s2)*-log(A(s,s2));
        	}
        }

        log_i("Likelihood : %.2f",likelihood);

        if ( fabs(likelihood - last_likelihood)/fabs(last_likelihood) < epsilon ) {
        	log_i("Likelihood converged. Stopping.\n");
        	break;
        }

        last_likelihood = likelihood;
	}

	if ( csvout_fname.size() > 0 ) {
		CSVLoader::writeTStoCSV(csvout_fname, STATE_level, outprec);
	}

	cleanup:
	delete inputTS;
	delete STATE_level;

	log_i("Done.");
}



int main(int argc,  char * const * argv) {
	HMMObserver prog;
	prog.parseCL(argc,argv);
	prog.start();
	return error;
}
