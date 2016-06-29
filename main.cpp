#include <vector>
#include <map>
#include <iostream>
#include <unistd.h>
#include "file.hpp"
#include "class.hpp"
#include <mutex>
#include <thread>

using namespace std;

std::vector<Konta*> konta;
std::vector<Postac*> postacie;

Konta::Konta(std::string _login, std::string _password):login(_login),
	password(_password),curl(new CURL_myobj())
{

}
Konta::~Konta(){
	delete curl;
}
std::string Konta::getLogin(){
	return login;
}
std::string Konta::getPassword(){
	return password;
}
void Konta::setPassword(std::string p){
	password=p;
}

Postac::Postac(std::string _login, std::string _world, std::string _nick):
	login(_login),world(_world),nick(_nick),next_use(0){

}
Postac::Postac(const Postac& a): login(a.login),
	world(a.world),nick(a.nick),next_use(a.next_use){

}
Konta* Postac::getParent(){
	size_t l = konta.size();
	for(size_t i=0;i<l;i++){
		if(konta[i]->getLogin()==login){
			return konta[i];
		}
	}
	return 0;
}
bool Postac::isParent(Konta* _par){
	return login==_par->getLogin();
}
std::string Postac::getWorld(){
	return world;
}
std::string Postac::getNick(){
	return nick;
}
time_t Postac::getNext(){
	return next_use;
}
void Postac::setNext(time_t t){
	next_use=t;
}
std::string Postac::getLogin(){
	return login;
}
class Mini_konto{
public:
	std::string login;
	std::string password;
	int from;
	Mini_konto(std::string _login, std::string _password, int _from);
	Mini_konto* get(std::vector<Mini_konto>& vec, bool password_check);
};
class Mini_postac{
public:
	std::string login;
	std::string world;
	std::string nickname;
	int from;
	Mini_postac(std::string _login, std::string _world, std::string _nickname, int _from);
	Mini_postac* get(std::vector<Mini_postac>& vec);
	bool hasParent();
};
Mini_konto::Mini_konto(std::string _login, std::string _password, int _from):
	login(_login), password(_password), from(_from)
{

}
Mini_konto* Mini_konto::get(std::vector<Mini_konto>& vec, bool password_check){
	size_t l = vec.size();
	for(size_t i = 0; i < l; i++){
		if(vec[i].login==login && (!password_check || vec[i].password==password)){
			return &vec[i];
		}
	}
	return (Mini_konto*)0;
}
Mini_postac::Mini_postac(std::string _login, std::string _world, std::string _nickname, int _from):
	login(_login), world(_world), nickname(_nickname), from(_from)
{

}
Mini_postac* Mini_postac::get(std::vector<Mini_postac>& vec){
	size_t l = vec.size();
	for(size_t i = 0; i < l; i++){
		if(vec[i].login==login && vec[i].world==world && vec[i].nickname==nickname){
			return &vec[i];
		}
	}
	return (Mini_postac*)0;
}
bool Mini_postac::hasParent(){
	size_t l = konta.size();
	for(size_t i=0;i<l;i++){
		if(konta[i]->getLogin()==login){
			return true;
		}
	}
	return false;
}

static std::mutex mutex_praca;
static int praca_lock_state;
static std::vector<int> zadania;
//extern vector<LuaThread> threads;
void government_init();
void government_func();

void praca_lock(){
	praca_lock_state = 1;
	mutex_praca.lock();
}
void praca_unlock(){
	mutex_praca.unlock();
	praca_lock_state = 0;
}
bool praca_locked(){
	return praca_lock_state==1?true:false;
}
int read_config(){
	fline linia;
	//wczytywanie konfigu i start watkow
	file plik_konfig("config.txt","rb","=");
	if(!plik_konfig.isGood()){
		cout<<"Nie znaleziono pliku konfiguracyjnego, utwórz plik config.txt"<<endl;
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
		//cout<<linia.getVal(0)<<endl;
	}
	return 0;
}

int read_konta(){
	fline linia;
	file plik_konta("accounts.txt","rb",":");
	vector<Mini_konto> aktywne_konta;
	vector<Mini_postac> aktywne_postacie;
	if(!plik_konta.isGood()){
		return 1;
	}
	/*
		from: 1 - file, 2- memory, 3 - both, 4 - need change
	*/
	while(plik_konta.isEOF()){
		linia=plik_konta.getLine();
		if(linia.isCommented()){
			continue;
		}
		if(linia.countVals()!=4 || linia.getVal(0).empty()){
			continue;
		}
		if(!linia.getVal(1).empty()){
			Mini_konto nkonto(linia.getVal(0),linia.getVal(1),1);
			if(nkonto.get(aktywne_konta,false)==0){
				aktywne_konta.push_back(nkonto);
			}
		}
		if(!linia.getVal(2).empty() && !linia.getVal(3).empty()){
			Mini_postac npostac(linia.getVal(0),linia.getVal(2),linia.getVal(3),1);
			if(npostac.get(aktywne_postacie)==0){
				aktywne_postacie.push_back(npostac);
			}
		}
	}
	size_t lrk = konta.size();
	size_t lrp = postacie.size();
	for(size_t i=0;i<lrk;i++){
		Mini_konto nkonto(konta[i]->getLogin(),konta[i]->getPassword(),2);
		Mini_konto* poa = nkonto.get(aktywne_konta,true);
		Mini_konto* poa2 = nkonto.get(aktywne_konta,false);
		if(poa==0 && poa2!=0){
			poa2->from = 4;
		}else if(poa==0 && poa2==0){
			aktywne_konta.push_back(nkonto);
		}else{
			poa->from = 3;
		}
	}
	for(size_t i=0;i<lrp;i++){
		Konta* konto = postacie[i]->getParent();
		Mini_postac npostac(postacie[i]->getLogin(),postacie[i]->getWorld(),
			postacie[i]->getNick(),2);
		Mini_postac* poa = npostac.get(aktywne_postacie);
		if(poa==0 && konto!=0){
			aktywne_postacie.push_back(npostac);
		}else if(poa!=0 && konto==0){
			poa->from = 2;
		}else if(poa!=0){
			poa->from = 3;
		}
	}
	//analiza
	praca_lock();
	size_t lk = aktywne_konta.size();
	for(size_t i=0;i<lk;i++){
		std::string klogin = aktywne_konta[i].login;
		std::string kpassword = aktywne_konta[i].password;
		switch(aktywne_konta[i].from){
			case 1:
				konta.push_back(new Konta(klogin,kpassword));
			break;
			case 2:{
				size_t lrk = konta.size();
				for(size_t ri=0;ri<lrk;ri++){
					if(konta[ri]->getLogin()==klogin){
						delete konta[ri];
						konta.erase(konta.begin()+ri);
						break;
					}
				}
			}break;
			case 4:
				size_t lrk = konta.size();
				for(size_t ri=0;ri<lrk;ri++){
					if(konta[ri]->getLogin()==klogin){
						konta[ri]->setPassword(kpassword);
						break;
					}
				}
			break;
		}
	}
	size_t lp = aktywne_postacie.size();
	for(size_t i=0;i<lp;i++){
		std::string klogin = aktywne_postacie[i].login;
		std::string pworld = aktywne_postacie[i].world;
		std::string pnick = aktywne_postacie[i].nickname;
		if(!aktywne_postacie[i].hasParent()){
			aktywne_postacie[i].from=2;
		}
		switch(aktywne_postacie[i].from){
			case 1:
				postacie.push_back(new Postac(klogin,pworld,pnick));
			break;
			case 2:{
				size_t lrp = postacie.size();
				for(size_t ri=0;ri<lrp;ri++){
					if(postacie[ri]->getLogin()==klogin &&
						postacie[ri]->getWorld()==pworld &&
						postacie[ri]->getNick()==pnick){
							delete postacie[ri];
							postacie.erase(postacie.begin()+ri);
							break;
					}
				}
			}break;
		}
	}
	praca_unlock();
	return 0;
}

int main(){
	srand(time(0));
	fline linia;
	int ret=0;
	//wczytywanie konfigu i start watkow
	ret=read_config();
	if(ret!=0){
		return ret;
	}
	government_init();
	//aktualizacja z pliku
	while(1){
		ret=read_konta();
		if(ret!=0){
			return ret;
		}
		government_func();
	}
}
