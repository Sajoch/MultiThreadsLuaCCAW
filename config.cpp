#include "file.hpp"
#include <iostream>
#include <map>

using namespace std;

static map<string,string> config;

string getConfigString(string name){
	return config[name];
}
size_t getConfigInt(string name){
	return stol(config[name]);
}
bool getConfigBool(string name){
	return config[name]=="true"?true:false;
}
int read_config(){
	fline linia;
	file plik_konfig("config.txt","rb","=");
	if(!plik_konfig.isGood()){
		cout<<"Cannot found config file, please create config.txt"<<endl;
		return 1;
	}
	while(plik_konfig.isEOF()){
		linia=plik_konfig.getLine();
		if(linia.isCommented()){
			continue;
		}
		if(linia.countVals()<2 || linia.getVal(0).empty()){
			continue;
		}
		config[linia.getVal(0)]=linia.getVal(1);
	}
	return 0;
}
