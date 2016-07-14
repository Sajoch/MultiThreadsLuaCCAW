#include <vector>
#include <map>
#include <iostream>
#include <unistd.h>
#include "file.hpp"
#include "class.hpp"
#include <mutex>
#include <thread>

using namespace std;

std::vector<Account*> accounts;
std::vector<Character*> characters;
extern vector<LuaThread> threads;
Account::Account(std::string _login, std::string _password):login(_login),
	password(_password)
{

}
Account::~Account(){

}
std::string Account::getLogin(){
	return login;
}
std::string Account::getPassword(){
	return password;
}
void Account::setPassword(std::string p){
	password=p;
}

Character::Character(std::string _login, std::string _world, std::string _nick):
	login(_login),world(_world),nick(_nick),next_use(0){

}
Character::Character(const Character& a): login(a.login),
	world(a.world),nick(a.nick),next_use(a.next_use){

}
Account* Character::getParent(){
	size_t l = accounts.size();
	for(size_t i=0;i<l;i++){
		if(accounts[i]->getLogin()==login){
			return accounts[i];
		}
	}
	return 0;
}
bool Character::isParent(Account* _par){
	return login==_par->getLogin();
}
std::string Character::getWorld(){
	return world;
}
std::string Character::getNick(){
	return nick;
}
time_t Character::getNext(){
	return next_use;
}
void Character::setNext(time_t t){
	next_use=t;
}
std::string Character::getLogin(){
	return login;
}
class Mini_acc{
public:
	std::string login;
	std::string password;
	int from;
	Mini_acc(std::string _login, std::string _password, int _from);
	Mini_acc* get(std::vector<Mini_acc>& vec, bool password_check);
};
class Mini_char{
public:
	std::string login;
	std::string world;
	std::string nickname;
	int from;
	Mini_char(std::string _login, std::string _world, std::string _nickname, int _from);
	Mini_char* get(std::vector<Mini_char>& vec);
	bool hasParent();
};
Mini_acc::Mini_acc(std::string _login, std::string _password, int _from):
	login(_login), password(_password), from(_from)
{

}
Mini_acc* Mini_acc::get(std::vector<Mini_acc>& vec, bool password_check){
	size_t l = vec.size();
	for(size_t i = 0; i < l; i++){
		if(vec[i].login==login && (!password_check || vec[i].password==password)){
			return &vec[i];
		}
	}
	return (Mini_acc*)0;
}
Mini_char::Mini_char(std::string _login, std::string _world, std::string _nickname, int _from):
	login(_login), world(_world), nickname(_nickname), from(_from)
{

}
Mini_char* Mini_char::get(std::vector<Mini_char>& vec){
	size_t l = vec.size();
	for(size_t i = 0; i < l; i++){
		if(vec[i].login==login && vec[i].world==world && vec[i].nickname==nickname){
			return &vec[i];
		}
	}
	return (Mini_char*)0;
}
bool Mini_char::hasParent(){
	size_t l = accounts.size();
	for(size_t i=0;i<l;i++){
		if(accounts[i]->getLogin()==login){
			return true;
		}
	}
	return false;
}

static std::mutex mutex_work;
static int work_lock_state;
static std::vector<int> zadania;
//extern vector<LuaThread> threads;
void government_init();
void government_func();

void work_lock(){
	work_lock_state = 1;
	mutex_work.lock();
}
void work_unlock(){
	mutex_work.unlock();
	work_lock_state = 0;
}
bool work_locked(){
	return work_lock_state==1?true:false;
}
int read_config();

int read_accounts(){
	fline line;
	file file_accounts("accounts.txt","rb",":");
	vector<Mini_acc> active_accounts;
	vector<Mini_char> active_chars;
	if(!file_accounts.isGood()){
		return 1;
	}
	/*
		from: 1 - file, 2- memory, 3 - both, 4 - need change
	*/
	while(file_accounts.isEOF()){
		line=file_accounts.getLine();
		if(line.isCommented()){
			continue;
		}
		if(line.countVals()!=4 || line.getVal(0).empty()){
			continue;
		}
		if(!line.getVal(1).empty()){
			Mini_acc nacc(line.getVal(0),line.getVal(1),1);
			if(nacc.get(active_accounts,false)==0){
				active_accounts.push_back(nacc);
			}
		}
		if(!line.getVal(2).empty() && !line.getVal(3).empty()){
			Mini_char nchar(line.getVal(0),line.getVal(2),line.getVal(3),1);
			if(nchar.get(active_chars)==0){
				active_chars.push_back(nchar);
			}
		}
	}
	size_t lrk = accounts.size();
	size_t lrp = characters.size();
	for(size_t i=0;i<lrk;i++){
		Mini_acc nacc(accounts[i]->getLogin(),accounts[i]->getPassword(),2);
		Mini_acc* poa = nacc.get(active_accounts,true);
		Mini_acc* poa2 = nacc.get(active_accounts,false);
		if(poa==0 && poa2!=0){
			poa2->from = 4;
		}else if(poa==0 && poa2==0){
			active_accounts.push_back(nacc);
		}else{
			poa->from = 3;
		}
	}
	for(size_t i=0;i<lrp;i++){
		Account* acc = characters[i]->getParent();
		Mini_char nchar(characters[i]->getLogin(),characters[i]->getWorld(),
			characters[i]->getNick(),2);
		Mini_char* poa = nchar.get(active_chars);
		if(poa==0 && acc!=0){
			active_chars.push_back(nchar);
		}else if(poa!=0 && acc==0){
			poa->from = 2;
		}else if(poa!=0){
			poa->from = 3;
		}
	}
	//analiza
	work_lock();
	size_t lk = active_accounts.size();
	for(size_t i=0;i<lk;i++){
		std::string klogin = active_accounts[i].login;
		std::string kpassword = active_accounts[i].password;
		switch(active_accounts[i].from){
			case 1:
				accounts.push_back(new Account(klogin,kpassword));
			break;
			case 2:{
				//delete from threads
				size_t st = threads.size();
				//delete accounts
				size_t lrk = accounts.size();
				for(size_t ri=0;ri<lrk;ri++){
					if(accounts[ri]->getLogin()==klogin){
						for(size_t t = 0;t<st;t++){
							LuaThread& thread=threads[t];
							if(thread.isAcc(accounts[ri])){
								thread.delChar();
							}
						}
						delete accounts[ri];
						accounts.erase(accounts.begin()+ri);
						break;
					}
				}
			}break;
			case 4:
				size_t lrk = accounts.size();
				for(size_t ri=0;ri<lrk;ri++){
					if(accounts[ri]->getLogin()==klogin){
						accounts[ri]->setPassword(kpassword);
						break;
					}
				}
			break;
		}
	}
	size_t lp = active_chars.size();
	for(size_t i=0;i<lp;i++){
		std::string klogin = active_chars[i].login;
		std::string pworld = active_chars[i].world;
		std::string pnick = active_chars[i].nickname;
		if(!active_chars[i].hasParent()){
			active_chars[i].from=2;
		}
		switch(active_chars[i].from){
			case 1:
				characters.push_back(new Character(klogin,pworld,pnick));
			break;
			case 2:{
				//delete from threads
				size_t st = threads.size();
				//delete characters
				size_t lrp = characters.size();
				for(size_t ri=0;ri<lrp;ri++){
					if(characters[ri]->getLogin()==klogin &&
						characters[ri]->getWorld()==pworld &&
						characters[ri]->getNick()==pnick){
							for(size_t t = 0;t<st;t++){
								LuaThread& thread=threads[t];
								if(thread.isChar(characters[ri])){
									thread.delChar();
								}
							}
							delete characters[ri];
							characters.erase(characters.begin()+ri);
							break;
					}
				}
			}break;
		}
	}
	work_unlock();
	return 0;
}

int main(){
	srand(time(0));
	fline line;
	int ret=0;
	//wczytywanie konfigu i start watkow
	ret=read_config();
	if(ret!=0){
		return ret;
	}
	government_init();
	//aktualizacja z pliku
	while(1){
		ret=read_accounts();
		if(ret!=0){
			return ret;
		}
		government_func();
	}
}
