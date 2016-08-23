#include "file.hpp"
#include <iostream>
#include <errno.h>
#include <string.h>

fline::fline(){

}
void fline::parse(std::string splitter){
	size_t br;
	size_t br_start=0;
	while(linia.size()>0){
		char koniec=linia[linia.size()-1];
		if(koniec=='\r' || koniec=='\n' || koniec==' ' || koniec=='\t'){
			linia.pop_back();
		}else{
			break;
		}
	}
	while(linia.size()>1){
		br=linia.find(splitter,br_start);
		if(br==0 || (br>0 && linia[br-1]!='\\')){
			if(br==std::string::npos){
				vals.push_back(linia.substr(br_start));
				break;
			}else{
				vals.push_back(linia.substr(br_start,(br-br_start)));
			}
		}
		br_start=br+1;
	}
}
bool fline::isCommented(){
	if(linia.size()==0 || linia.at(0)=='#' || linia.at(0)=='\n'
		|| linia.at(0)=='\r' || linia.at(0)==' ')
		return true;
	return false;
}
std::string& fline::getLine(){
	return linia;
}
std::string fline::getVal(size_t id){
	if(id>vals.size())
		return "";
	return vals[id];
}
size_t fline::countVals(){
	return vals.size();
}


file::file(std::string _path, std::string mode, std::string _splitter):path(_path), splitter(_splitter){
	std::ios_base::openmode io_mode;
	if(mode.find("r")!=std::string::npos){
		io_mode|=std::fstream::in;
		std::cout<<"open in"<<std::endl;
	}
	if(mode.find("w")!=std::string::npos){
		io_mode|=std::fstream::out;
		std::cout<<"open out"<<std::endl;
	}
	if(mode.find("t")!=std::string::npos){
		io_mode|=std::fstream::trunc;
		std::cout<<"open trucn"<<std::endl;
	}
	if(mode.find("a")!=std::string::npos){
		io_mode|=std::fstream::app;
		std::cout<<"open app"<<std::endl;
	}
	if(mode.find("e")!=std::string::npos){
		io_mode|=std::fstream::ate;
		std::cout<<"open ate"<<std::endl;
	}
	if(mode.find("b")!=std::string::npos){
		io_mode|=std::fstream::binary;
		std::cout<<"open binary"<<std::endl;
	}
  plik.open(path.c_str(),io_mode);
	if(plik.bad()){
		std::cerr << "Error: " << strerror(errno)<<std::endl;
	}

}
fline file::getLine(){
	fline ret;
	std::getline(plik,ret.getLine());
	//std::cout<<"line "<<ret.getLine()<<std::endl;
	ret.parse(splitter);
	return ret;
}
void file::writeLine(std::string a){
	plik<<a<<std::endl;
}
bool file::isGood(){
	return !plik.bad();
}
bool file::isEOF(){
	return !plik.eof();
}
