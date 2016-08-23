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


file::file(std::string _path, std::ios_base::openmode mode, std::string _splitter):path(_path), splitter(_splitter){
  plik.open(path.c_str(), mode);
	if(plik.bad()){
		std::cerr << "Error: " << strerror(errno)<<std::endl;
	}

}
fline file::getLine(){
	fline ret;
	std::getline(plik,ret.getLine());
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
