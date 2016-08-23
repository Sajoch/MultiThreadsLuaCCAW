#include <fstream>
#include <string>
#include <vector>

class fline{
	std::string linia;
	std::vector<std::string> vals;
public:
	fline();
	void parse(std::string splitter);
	bool isCommented();
	std::string& getLine();
	std::string getVal(size_t id);
	size_t countVals();
};

class file{
	std::string path;
	std::fstream plik;
	std::string splitter;
public:
	file(std::string _path, std::ios_base::openmode mode, std::string _splitter);
	fline getLine();
	void writeLine(std::string a);
	bool isGood();
	bool isEOF();
};
