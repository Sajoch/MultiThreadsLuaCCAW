#include <iostream>
#include <fstream>
#include <string>
#include "class.hpp"

using namespace std;

size_t mb_limit = 1024*1024;

void check_limit_file(){
	string log_path = getConfigString("log_file");
	std::fstream fs;
	fs.open(log_path, std::fstream::out | std::fstream::app);
	if(fs.tellg()>mb_limit){
		fs.close();
		fs.open(log_path, std::fstream::out | std::fstream::trunc);
	}
	fs.close();
}
void writeLog(string a){
	check_limit_file();
	string log_path = getConfigString("log_file");
	std::ofstream ofs;
	ofs.open(log_path, std::ofstream::out | std::ofstream::app);
	ofs<<a;
	ofs.close();
}
void writeLog(string a, bool nl){
	check_limit_file();
	string log_path = getConfigString("log_file");
	std::ofstream ofs;
	ofs.open(log_path, std::ofstream::out | std::ofstream::app);
	ofs<<a<<endl;
	ofs.close();
}
void writeLog(size_t a){
	check_limit_file();
	string log_path = getConfigString("log_file");
	std::ofstream ofs;
	ofs.open(log_path, std::ofstream::out | std::ofstream::app);
	ofs<<a;
	ofs.close();
}
void writeLog(size_t a, bool nl){
	check_limit_file();
	string log_path = getConfigString("log_file");
	std::ofstream ofs;
	ofs.open(log_path, std::ofstream::out | std::ofstream::app);
	ofs<<a<<endl;
	ofs.close();
}
void writeLog(double a){
	check_limit_file();
	string log_path = getConfigString("log_file");
	std::ofstream ofs;
	ofs.open(log_path, std::ofstream::out | std::ofstream::app);
	ofs<<a;
	ofs.close();
}
void writeLog(double a, bool nl){
	check_limit_file();
	string log_path = getConfigString("log_file");
	std::ofstream ofs;
	ofs.open(log_path, std::ofstream::out | std::ofstream::app);
	ofs<<a<<endl;
	ofs.close();
}
