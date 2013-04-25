
#include "all.h"
#include <regex>

ConfigurationValue::ConfigurationValue() {
	isArray = false;
}

ConfigurationValue::ConfigurationValue(vector<ConfigurationValue> vect) {
	isArray = true;
	arr.swap(vect);
}

ConfigurationValue::ConfigurationValue(string str) {
	isArray = false;
	val = str;
}

ConfigurationValue::~ConfigurationValue() {
}

int ConfigurationValue::asInt() {
	if (!isArray) {
		return std::stoi( val );
	} else {
		log_i("Warning: Array config value treated as non-array");
		return (*this)[0].asInt();
	}
}
string ConfigurationValue::asString() {
	if (!isArray) {
		return val;
	} else {
		log_i("Warning: Array config value treated as non-array");
		return (*this)[0].asString();
	}
}
double ConfigurationValue::asDouble() {
	if (!isArray) {
		return std::stof( val );
	} else {
		log_i("Warning: Array config value treated as non-array");
		return (*this)[0].asDouble();
	}
}

ConfigurationValue & ConfigurationValue::operator[](int i) {
	if ( isArray ) {
		return arr[i];
	} else {
		log_i("Warning: Scalar config value treated as array");
		return (*this);
	}
}


vector<ConfigurationValue>::iterator ConfigurationValue::begin() {
	return arr.begin();
}

vector<ConfigurationValue>::iterator ConfigurationValue::end() {
	return arr.end();
}

const std::regex wsexpr("^\\s*(.*?)\\s*$"),
    comexp("^(.*?)#.*$"),
    setexpr("^(\\S+)\\s+(\\S+?)\\s+(.+?)$"),
	incexpr("^include\\s+\\\"(.*?)\\\"$"),
	sectexpr("^\\[(.+?)\\]$");

map<string,map<string, ConfigurationValue>> ConfigurationLoader::readConfiguration(string fname) {
	map<string,map<string,ConfigurationValue>> retval;
	_readConfiguration(fname, retval);
	return retval;
}

void ConfigurationLoader::_readConfiguration(string fname, map<string,map<string, ConfigurationValue>> &retval ) {
	ifstream fin(fname);

	match_results<string::const_iterator> result;

	string cursect("global");

	int line_no = 0;
	while(fin.peek() != EOF) {
		line_no++;
		string line;
		getline(fin,line);

		//cout << line << endl;
		//remove comments
		if ( regex_match(line, result, comexp) ) {
			line = result[1];
			//cout << "RMCOMM: "<<line << endl;
		}

		// skip whitespace
		if ( regex_match(line, result, wsexpr) ) {
			line = result[1];
			//cout << "RMWS: "<<line << endl;
		}
		if ( line.size() == 0 ) {
			continue;
		}

		// section header
		if ( regex_match(line,result,sectexpr) ) {
			cursect = result[1];
			continue;
		}

		// include another file
		if ( regex_match(line,result,incexpr) ) {
			string f2 = result[1];
			if ( f2.find("/") == string::npos && f2.find("\\") == string::npos ) {
				size_t pos1 = fname.rfind("\\");
				size_t pos2 = fname.rfind("/");
				if ( pos1 < pos2 && pos2 != string::npos) {
					f2 = fname.substr( 0, pos2 + 1 ) + f2;
				} else if ( pos2 < pos1 && pos1 != string::npos ) {
					f2 = fname.substr( 0, pos1 + 1 ) + f2;
				} 
				
				
			}
			//std::cout << "Include "<<f2<<endl;
			_readConfiguration(f2, retval);		
			continue;
		}

		// actual assignment
		if ( regex_match(line,result,setexpr) ) {
			string key = result[1];
			string oper = result[2];
			string val = result[3];
			
			if ( oper == "=" ) {
				retval[cursect][key] = ConfigurationValue(val);
				//std::cout << "[" << cursect << "] "<<key<<" = "<<val << endl;
			} else if ( oper == "@=") {
				retval[cursect][key] = ConfigurationValue(vector<ConfigurationValue>(1, ConfigurationValue(val)));
				//std::cout << "[" << cursect << "] "<<key<<" @= "<<val << endl;
			} else if ( oper == "@+") {
				if ( retval[cursect].find(key) == retval[cursect].end() ) {
					retval[cursect][key] = ConfigurationValue(vector<ConfigurationValue>(1, ConfigurationValue(val)));
				}
				else {
					retval[cursect][key].arr.push_back(ConfigurationValue(val));
				}
				//std::cout << "[" << cursect << "] "<<key<<" @+ "<<val << endl;
			}
		} else {
			log_e("Configuration file %s:%d : Syntax Error %s",fname.c_str(),line_no,line.c_str());
		}
	}
	fin.close();
}
