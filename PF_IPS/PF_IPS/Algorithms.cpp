#include "PF_IPS.h"

double tsSTD(TimeSeries *ts);


void PF_IPS::_calibrate_RSSI_system() {
	log_i("Calibrating RSSI system");

	ofstream fout(rssi_basedir + "scatter.csv");
	for ( size_t sensi = 0; sensi < sensors.size(); sensi++ ) {
		for ( size_t refi = 0; refi < reference_tags.size(); refi++) {
			RSSISensor sensor = sensors[sensi];
			RSSITag tag = reference_tags[refi];
			TimeSeries * ts = rssi_refdata[refi][sensi];
			double dist = sqrt(pow(sensor.pos.x + sense_ref_X - tag.pos.x - tag_ref_X,2) + pow(sensor.pos.y + sense_ref_Y - tag.pos.y - tag_ref_Y,2) + pow(tag_ref_Z - sense_ref_Z,2));
			log_i("Sensor: %s, Tag: %s, Distance: %.2fm Mean: %.2f dBm STD: %.2f",sensor.IDstr.c_str(),tag.IDstr.c_str(),dist,ts->mean(),tsSTD(ts));
			for (size_t ti = 0; ti < ts->v.size(); ti++ ) {
				fout << ts->t[ti] << "," << ts->v[ti] << "," << sensi << "," << refi << "," << dist << endl;
			}
		}
	}
	fout.close();
}

double tsSTD(TimeSeries *ts) {
	double ssq = 0;
	double mean = ts->mean();
	for (size_t c = 0; c < ts->v.size(); c++ ) {
		ssq += pow(ts->v[c]-mean,2);
	}
	return ssq / ts->v.size();
}