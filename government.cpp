#include <iostream>
#include "class.hpp"
#include <vector>
#include <unistd.h>

using namespace std;

extern std::vector<Account*> accounts;
extern std::vector<Character*> characters;

vector<LuaThread> threads;
size_t LuaThread::numbers=1;
bool init=false;
void government_init(){
	size_t trs = getConfigInt("thread");
	cout<<"start "<<trs<<" threads"<<endl;
	threads.resize(trs);
}
void government_func(){
	time_t now;
	now=time(0);
	size_t lk = accounts.size();
	size_t lp = characters.size();
	size_t lt = threads.size();
	//set inthread = true if account works
	for(size_t t = 0;t<lt;t++){
		LuaThread& thread=threads[t];
		if(thread.isFree()){
			for(size_t p = 0;p<lp;p++){
				Character* ch=characters[p];
				Account* acc = ch->getParent();
				if(ch->getNext()<now && acc!=0 && !acc->inthread){
					thread.addChar(ch);
					break;
				}
			}
		}
	}
	sleep(5);
}
