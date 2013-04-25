#ifndef CONFIGURATION_LOADER_H
#define CONFIGURATION_LODAER_H

class ConfigurationValue {
	friend class ConfigurationLoader;

public:
	ConfigurationValue();

	int asInt() ;
	string asString() ;
	double asDouble() ;

	ConfigurationValue & operator[](int i);
	vector<ConfigurationValue>::iterator begin() ;
	vector<ConfigurationValue>::iterator end() ;

	~ConfigurationValue();
private:
	ConfigurationValue(vector<ConfigurationValue> vect);
	ConfigurationValue(string str);	

	vector<ConfigurationValue> arr;
	bool isArray;
	string val;
};

class ConfigurationLoader {
public:
	
	static map<string,map<string, ConfigurationValue>> readConfiguration(string fname);
	
	static void writeConfiguration(string fname, map<string,map<string,ConfigurationValue>> conf);

private:

	static void _readConfiguration(string fname, map<string,map<string, ConfigurationValue>> &retval);

};


#endif