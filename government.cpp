#include <iostream>
#include "class.hpp"
#include <vector>
#include <unistd.h>

using namespace std;

extern std::vector<Konta*> konta;
extern std::vector<Postac*> postacie;

vector<LuaThread> threads;
size_t LuaThread::numbers=1;
bool init=false;
void government_init(){
	threads.resize(2);
	/*for(size_t i=0;i<2;i++){
		threads.push_back(LuaThread());
	}*/
}
void government_func(){
	time_t now;
	now=time(0);
	size_t lk = konta.size();
	size_t lp = postacie.size();
	size_t lt = threads.size();
	//ustawia inthread = true jeszeli konto pracuje
	for(size_t t = 0;t<lt;t++){
		LuaThread& thread=threads[t];
		if(thread.isFree()){
			for(size_t p = 0;p<lp;p++){
				Postac* post=postacie[p];
				Konta* konto = post->getParent();
				if(post->getNext()<now && konto!=0 && !konto->inthread){
					thread.addPostac(post);
					break;
				}
			}
		}
	}
	sleep(5);
}
