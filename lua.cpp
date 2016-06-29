#include "class.hpp"
#include <iostream>
#include <unistd.h>
#include <mutex>
#include <chrono>
#include <thread>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include "file.hpp"

using namespace std;

void praca_lock();
void praca_unlock();
bool praca_locked();
std::mutex LuaThread::print_mutex;



/*
 * 0 - pause
 * 1 - resume
 * 2 - stop
*/
void LuaThread::send_command(int a){
	std::unique_lock<std::mutex> lk(*m);
	command = a;
	cv->notify_one();
}
bool LuaThread::isFree(){
	return command==0;
}
size_t LuaThread::getId(){
	return id;
}
bool LuaThread::addPostac(Postac* p){
	std::unique_lock<std::mutex> lk(*m);
	Konta* konto = p->getParent();
	if(praca==0 && konto!=0){
		konto->inthread=true;
		praca=p;
		command = 1;
		skasuj = false;
		cv->notify_one();
		return true;
	}
	return false;
}
bool LuaThread::isPostac(Postac* p){
	return praca==p;
}
Postac* LuaThread::getPostac(){
	return praca;
}
void LuaThread::delPostac(){
	std::unique_lock<std::mutex> lk(*m);
	skasuj = true;
	command = 1;
	cv->notify_one();
}
LuaThread::LuaThread(){
	command = 0;
	praca = 0;
	skasuj = false;
	id=numbers;
	numbers++;
	luaS=0;
	m=new std::mutex();
	cv=new std::condition_variable();
	worker=new std::thread(start_thread, this);
}
LuaThread::~LuaThread(){
	send_command(2);
	worker->join();
	delete worker;
	delete m;
	delete cv;
}
void LuaThread::start_thread(LuaThread* a){
	a->thread();
}
void LuaThread::thread(){
	while(1){
		m->lock();
		int _command=command;
		Postac* _praca=praca;
		bool _skasuj=skasuj;
		lua_State* _luaS=luaS;
		m->unlock();
		switch(_command){
			case 0:{
				std::unique_lock<std::mutex> lk(*m);
				cv->wait(lk);
			}break;
			case 1:{
				//do sth
				//cout<<"do"<<endl;
				if(_skasuj){
					m->lock();
					Konta* konto = praca->getParent();
					if(konto!=0){
						konto->inthread=false;
					}
					praca = 0;
					command=0;
					if(luaS!=0){
						lua_close(luaS);
						luaS=0;
					}
					m->unlock();
					_praca = praca;
					_command = command;
					_luaS = luaS;
					size_t sleep_after_work=5;
					sleep_after_work+=rand()%10;
					std::this_thread::sleep_for(std::chrono::seconds(sleep_after_work));
				}else if(_praca!=0){
					if(_luaS==0){
						//init lua for first time
						m->lock();
						luaS=luaL_newstate();
						m->unlock();
						_luaS=luaS;
						const luaL_Reg lualibs[] = {
							{"", luaopen_base},
							//{LUA_LOADLIBNAME, luaopen_package},
							{LUA_TABLIBNAME, luaopen_table},
							//{LUA_IOLIBNAME, luaopen_io},
							//{LUA_OSLIBNAME, luaopen_os},
							{LUA_STRLIBNAME, luaopen_string},
							{LUA_MATHLIBNAME, luaopen_math},
							//{LUA_DBLIBNAME, luaopen_debug},
							{NULL, NULL}
						};
						const luaL_Reg *lib = lualibs;
						for (; lib->func; lib++) {
							lua_pushcfunction(luaS, lib->func);
							lua_pushstring(luaS, lib->name);
							lua_call(luaS, 1, 0);
						}
						if(luaL_loadfile(luaS,"system.lua")){
							cout<<"Lua fatal error: luaL_loadfile() failed\n\t"<<lua_tostring(luaS,-1)<<endl;
							exit(1);
						}
						if (lua_pcall(luaS, 0, 0, 0)){
							cout<<"Lua fatal error: lua_pcall() failed\n\t"<<lua_tostring(luaS,-1)<<endl;
							exit(1);
						}

						lua_register(luaS,"print",_Lua_print);
						//lua_register(luaS,"sleep",_Lua_sleep);
						lua_register(luaS,"mstime",_Lua_mstime);
						lua_register(luaS,"time",_Lua_time);
						//lua_register(luaS,"setnext",_Lua_setnext);
						lua_register(luaS,"md5",_Lua_md5);
						lua_register(luaS,"sha1",_Lua_sha1);

						//rapidjson

						lua_register(luaS,"RJ_parse",_Lua_rj_parse);

						//file

						lua_register(luaS,"File_open",_Lua_File_open);
						lua_register(luaS,"File_close",_Lua_File_close);
						lua_register(luaS,"File_isGood",_Lua_File_isGood);
						lua_register(luaS,"File_isEOF",_Lua_File_isEOF);
						lua_register(luaS,"File_getLine",_Lua_File_getLine);

						//lua_register(luaS,"curl_init",CURL_myobj::_Lua_curl_init);
						lua_register(luaS,"curl_set",CURL_myobj::_Lua_curl_set);
						lua_register(luaS,"curl_exec",CURL_myobj::_Lua_curl_exec);
						lua_register(luaS,"curl_get_body",CURL_myobj::_Lua_curl_get_body);
						lua_register(luaS,"curl_add_cookie",CURL_myobj::_Lua_add_cookie);
						lua_register(luaS,"curl_reset",CURL_myobj::_Lua_curl_reset);
						//lua_register(luaS,"curl_clean",CURL_myobj::_Lua_curl_clean);

						lua_getfield(luaS, LUA_GLOBALSINDEX, "Postac");
						praca_lock();
						Konta* konto = praca->getParent();
						if(konto==0){
							cout<<"konto is 0"<<endl;
							exit(1);
						}
						//lua_pushlightuserdata(luaS, this);
						lua_pushstring(luaS, konto->getLogin().c_str());
						lua_pushstring(luaS, konto->getPassword().c_str());
						lua_pushstring(luaS, praca->getNick().c_str());
						lua_pushstring(luaS, praca->getWorld().c_str());
						praca_unlock();
						if(lua_pcall(luaS, 4, 1, 0) != 0){
							cout<<"error running function 'new Postac': "<<lua_tostring(luaS,-1)<<endl;
							exit(1);
						}
           	lua_setfield(luaS, LUA_GLOBALSINDEX, "account");
					}
					//tick lua
					lua_getfield(luaS, LUA_GLOBALSINDEX, "account");
   				lua_getfield(luaS, -1, "tick");
   				lua_remove(luaS,-2);
   				lua_getfield(luaS, LUA_GLOBALSINDEX, "account");
					if(lua_isfunction(luaS,-2)){
						praca_lock();
						Konta* konto = praca->getParent();
						if(konto==0){
							praca_unlock();
							m->lock();
							skasuj = true;
							m->unlock();
							_skasuj=skasuj;
							continue;
						}
						if(konto->curl==0){
							cout<<"critical error curl_easy_init() returns 0"<<endl;
							exit(1);
						}
						lua_pushlightuserdata(luaS, konto->curl);
						if(lua_pcall(luaS, 2, 2, 0) != 0){
							cout<<"error running function 'account.tick': "<<lua_tostring(luaS,-1)<<endl;
							exit(1);
						}
						praca_unlock();
						size_t sleep_time = lua_tointeger(luaS,-2);
						size_t logout_time = lua_tointeger(luaS,-1);
						lua_pop(luaS,2);
						if(logout_time==0){
							if(sleep_time>0){
								std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
							}
						}else{
							praca_lock();
							praca->setNext(logout_time);
							praca_unlock();
							delPostac();
						}
					}
				}else{
					m->lock();
					command=0;
					if(luaS!=0){
						lua_close(luaS);
					}
					luaS=0;
					m->unlock();
					_luaS=0;
					_command=command;
				}
			}break;
			case 2:{
				return;
			}
		}
	}
}
int LuaThread::_Lua_sleep(lua_State* s){
	int n = lua_gettop(s);
	if(n==1 && lua_isnumber(s,1)){
		size_t t=lua_tonumber(s,1);
		std::this_thread::sleep_for(std::chrono::milliseconds(t));
	}
	return 0;
}
int LuaThread::_Lua_print(lua_State* s){
	bool print_it=true;
	if(!print_it){
		return 0;
	}
	print_mutex.lock();
	int n = lua_gettop(s);
	for(int i=1;i<=n;i++){
		int typ = lua_type(s,i);
		switch(typ){
			case LUA_TSTRING:
				cout<<std::string(lua_tostring(s,i),lua_strlen(s,i));
			break;
			case LUA_TBOOLEAN:
				cout<<(lua_toboolean(s,i)?"true":"false");
			break;
			case LUA_TNUMBER:
				cout<<lua_tonumber(s,i);
			break;
			//case LUA_TTABLE:
			//	cout<<"table";
			//break;
			default:
				cout<<lua_typename(s,typ);
			break;
		}
	}
	cout<<endl;
	print_mutex.unlock();
	return 0;
}
int LuaThread::_Lua_mstime(lua_State* s){
	std::chrono::milliseconds req =
		std::chrono::duration_cast< std::chrono::milliseconds >(
			std::chrono::system_clock::now().time_since_epoch()
		);
	size_t t=req.count();
	lua_pushinteger(s,t);
	return 1;
}
int LuaThread::_Lua_time(lua_State* s){
	std::chrono::seconds req =
		std::chrono::duration_cast< std::chrono::seconds >(
			std::chrono::system_clock::now().time_since_epoch()
		);
	size_t t=req.count();
	lua_pushinteger(s,t);
	return 1;
}
int LuaThread::_Lua_setnext(lua_State* s){
	int n = lua_gettop(s);
	if(n==2 && lua_isuserdata(s,1) && lua_isnumber(s,2)){
		LuaThread* pt=(LuaThread*)lua_touserdata(s,1);
		time_t t=lua_tonumber(s,2);
		if(pt!=0){
			praca_lock();
			pt->praca->setNext(t);
			praca_unlock();
			pt->delPostac();
		}
	}
	return 0;
}
int LuaThread::_Lua_sha1(lua_State* s){
	int n = lua_gettop(s);
	if(n==1 && lua_isstring(s,1)){
		unsigned char result[SHA_DIGEST_LENGTH];
		SHA1((const unsigned char*)lua_tostring(s,1),lua_strlen(s,1),result);
		std::string ret;
		const char abc[]="0123456789abcdef";
		for(int i=0;i<SHA_DIGEST_LENGTH;i++){
			ret+=abc[(result[i]&0xf0)>>4];
			ret+=abc[result[i]&0xf];
		}
		lua_pushstring(s,ret.c_str());
		return 1;
	}
	return 0;
}
int LuaThread::_Lua_md5(lua_State* s){
	int n = lua_gettop(s);
	if(n==1 && lua_isstring(s,1)){
		unsigned char result[MD5_DIGEST_LENGTH];
		MD5((const unsigned char*)lua_tostring(s,1),lua_strlen(s,1),result);
		std::string ret;
		const char abc[]="0123456789abcdef";
		for(int i=0;i<MD5_DIGEST_LENGTH;i++){
			ret+=abc[(result[i]&0xf0)>>4];
			ret+=abc[result[i]&0xf];
		}
		lua_pushstring(s,ret.c_str());
		return 1;
	}
	return 0;
}
int LuaThread::_Lua_File_open(lua_State* s){
	int n = lua_gettop(s);
	if(n==3 && lua_isstring(s,1) && lua_isstring(s,2) && lua_isstring(s,3)){
		file* nf = new file(lua_tostring(s,1),lua_tostring(s,2),lua_tostring(s,3));
		lua_pushlightuserdata(s,nf);
		return 1;
	}
	return 0;
}
int LuaThread::_Lua_File_close(lua_State* s){
	int n = lua_gettop(s);
	if(n==1 && lua_isuserdata(s,1)){
		delete ((file*)lua_touserdata(s,1));
	}
	return 0;
}
int LuaThread::_Lua_File_isGood(lua_State* s){
	int n = lua_gettop(s);
	if(n==1 && lua_isuserdata(s,1)){
		int ret=((file*)lua_touserdata(s,1))->isGood()?1:0;
		lua_pushinteger(s,ret);
		return 1;
	}
	return 0;
}
int LuaThread::_Lua_File_isEOF(lua_State* s){
	int n = lua_gettop(s);
	if(n==1 && lua_isuserdata(s,1)){
		int ret=((file*)lua_touserdata(s,1))->isEOF()?1:0;
		lua_pushinteger(s,ret);
		return 1;
	}
	return 0;
}
int LuaThread::_Lua_File_getLine(lua_State* s){
	int n = lua_gettop(s);
	if(n==1 && lua_isuserdata(s,1)){
		fline ret=((file*)lua_touserdata(s,1))->getLine();
		//cout<<ret.getLine()<<endl;
		if(ret.isCommented()){
			lua_pushinteger(s,0);
			return 1;
		}
		size_t l = ret.countVals();
		lua_pushinteger(s,l);
		for(size_t i=0;i<l;i++){
			lua_pushstring(s,ret.getVal(i).c_str());
		}
		return 1+l;
	}
	return 0;
}
int LuaThread::traceback (lua_State *L) {
  if (!lua_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}
