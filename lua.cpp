#include "class.hpp"
#include <iostream>
#include <unistd.h>
#include <mutex>
#include <chrono>
#include <thread>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include "file.hpp"
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/NameValueCollection.h>

using namespace std;

void work_lock();
void work_unlock();
bool work_locked();
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
bool LuaThread::addChar(Character* p){
	std::unique_lock<std::mutex> lk(*m);
	Account* acc = p->getParent();
	if(work==0 && acc!=0){
		acc->inthread = true;
		work = p;
		work_acc = acc;
		command = 1;
		del = false;
		cv->notify_one();
		return true;
	}
	return false;
}
bool LuaThread::isAcc(Account* a){
	return work_acc==a;
}
bool LuaThread::isChar(Character* p){
	return work==p;
}
Character* LuaThread::getChar(){
	return work;
}
void LuaThread::delChar(){
	std::unique_lock<std::mutex> lk(*m);
	del = true;
	command = 1;
	cv->notify_one();
}
LuaThread::LuaThread(){
	command = 0;
	work = 0;
	work_acc = 0;
	del = false;
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
		Character* _work=work;
		bool _del=del;
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
				if(_del){
					m->lock();
					Account* acc = work->getParent();
					if(acc!=0){
						acc->inthread=false;
					}
					work = 0;
					command=0;
					if(luaS!=0){
						lua_close(luaS);
						luaS=0;
					}
					m->unlock();
					_work = work;
					_command = command;
					_luaS = luaS;
					size_t sleep_after_work=5;
					sleep_after_work+=rand()%10;
					std::this_thread::sleep_for(std::chrono::seconds(sleep_after_work));
				}else if(_work!=0){
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
						lua_register(luaS,"getHour",_Lua_getHour);
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

						//http

						lua_register(luaS,"http_request",_Lua_http_request);

						lua_register(luaS,"getConfigString",_Lua_getConfigString);
						lua_register(luaS,"getConfigInt",_Lua_getConfigInt);
						lua_register(luaS,"getConfigBool",_Lua_getConfigBool);

						lua_getfield(luaS, LUA_GLOBALSINDEX, "Postac");
						work_lock();
						if(work==0 || del){
							m->lock();
							del=true;
							work = 0;
							m->unlock();
							break;
						}
						Account* acc = work->getParent();
						if(acc==0){
							cout<<"acc is 0"<<endl;
							//exit(1);
							m->lock();
							del=true;
							m->unlock();
							break;
						}
						//lua_pushlightuserdata(luaS, this);
						lua_pushstring(luaS, acc->getLogin().c_str());
						lua_pushstring(luaS, acc->getPassword().c_str());
						lua_pushstring(luaS, work->getNick().c_str());
						lua_pushstring(luaS, work->getWorld().c_str());
						lua_pushnil(luaS);
						work_unlock();
						if(lua_pcall(luaS, 5, 1, 0) != 0){
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
						work_lock();
						Account* acc = work->getParent();
						if(acc==0){
							work_unlock();
							m->lock();
							del = true;
							m->unlock();
							_del=del;
							continue;
						}
						//lua_pushlightuserdata(luaS, konto->curl);
						if(lua_pcall(luaS, 1, 2, 0) != 0){
							cout<<"error running function 'account.tick': "<<lua_tostring(luaS,-1)<<endl;
							exit(1);
						}
						work_unlock();
						size_t sleep_time = lua_tointeger(luaS,-2);
						size_t logout_time = lua_tointeger(luaS,-1);
						lua_pop(luaS,2);
						if(logout_time==0){
							if(sleep_time>0){
								std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
							}
						}else{
							work_lock();
							work->setNext(logout_time);
							work_unlock();
							delChar();
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
void PrintTable(lua_State *s, int index, int shift){
	lua_pushnil(s);
	string supp = "";
	for(int i=0;i<shift;i++){
		supp+=" ";
	}
	cout<<"table"<<endl;
  while(lua_next(s, index) != 0){
		int key_type = lua_type(s,-2);
		int val_type = lua_type(s,-1);
		cout<<supp<<"[";
		switch(key_type){
			case LUA_TSTRING:
				cout<<std::string(lua_tostring(s,-2),lua_strlen(s,-2));
			break;
			case LUA_TNUMBER:
				cout<<lua_tonumber(s,-2);
			break;
		}
		cout<<"] = ";
		switch(val_type){
			case LUA_TSTRING:
				cout<<std::string(lua_tostring(s,-1),lua_strlen(s,-1))<<endl;
			break;
			case LUA_TBOOLEAN:
				cout<<(lua_toboolean(s,-1)?"true":"false")<<endl;
			break;
			case LUA_TNUMBER:
				cout<<lua_tonumber(s,-1)<<endl;
			break;
			case LUA_TTABLE:{
				PrintTable(s,-2,shift+1);
			}break;
			default:
				cout<<lua_typename(s,val_type)<<endl;
			break;
		}
		lua_pop(s,1);
	}
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
			case LUA_TTABLE:{
				PrintTable(s,i,0);
			}break;
			default:
				cout<<lua_typename(s,typ);
			break;
		}
		cout<<endl;
	}
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
void LuaSetTable(lua_State* s, int index, const char* key, const char* value){
	lua_pushstring(s,key);
	lua_pushstring(s,value);
	lua_settable(s,index);
}
void LuaSetTable(lua_State* s, int index, const char* key, size_t value){
	lua_pushstring(s,key);
	lua_pushinteger(s,value);
	lua_settable(s,index);
}
string LuaGetStringTable(lua_State* s, int index, const char* key){
	string ret;
	lua_pushstring(s,key);
	lua_gettable(s,index);
	if(lua_isstring(s,-1)){
		ret = lua_tostring(s,-1);
	}
	lua_pop(s,1);
	return ret;
}
size_t LuaGetIntTable(lua_State* s, int index, const char* key){
	size_t ret;
	lua_pushstring(s,key);
	lua_gettable(s,index);
	if(lua_isstring(s,-1)){
		ret = lua_tointeger(s,-1);
	}
	lua_pop(s,1);
	return ret;
}
int LuaThread::_Lua_http_request(lua_State* s){
	//url, method, cookies, postdata, additional
	//TODO add catcher of exceptions
	int n = lua_gettop(s);
	if(n<3){
		lua_pushstring(s,"too few arguments");
		return 1;
	}
	string url = lua_tostring(s,1);
	size_t br = url.find("://");
	if(br==std::string::npos){
		lua_pushstring(s,"wrong address");
		return 1;
	}
	string protocol = url.substr(0,br);
	url=url.substr(br+3);
	br=url.find("/");
	if(br==std::string::npos){
		lua_pushstring(s,"wrong address");
		return 1;
	}
	string host = url.substr(0,br);
	string path = url.substr(br);
	string method = "GET";
	if(lua_isstring(s,2)){
		method = lua_tostring(s,2);
	}else{
		lua_pushstring(s,"wrong method");
		return 1;
	}
	try{
		Poco::Net::NameValueCollection wcookies;
		std::vector < Poco::Net::HTTPCookie > cookies;
		if(lua_istable(s,3)){
			lua_pushnil(s);
		  while(lua_next(s, 3) != 0){
				Poco::Net::HTTPCookie tc;
				tc.setDomain(LuaGetStringTable(s,-2,"domain"));
				tc.setPath(LuaGetStringTable(s,-2,"path"));
				tc.setName(LuaGetStringTable(s,-2,"name"));
				tc.setValue(LuaGetStringTable(s,-2,"value"));
				tc.setMaxAge(LuaGetIntTable(s,-2,"expire"));
				cookies.push_back(tc);
				lua_pop(s,1);
			}
		}else{
			lua_pushstring(s,"wrong cookies");
			return 1;
		}
		//cout<<"http request p "<<protocol<<" h "<<host<<" p "<<path<<endl;
		string postdata ="";
		if(lua_isstring(s,4)){
			postdata = lua_tostring(s,4);
		}
		Poco::Net::HTTPClientSession session(host);
		session.setKeepAlive(true);
		Poco::Net::HTTPRequest request(method,path,"HTTP/1.1");
		request.add("Host",host);
		request.setKeepAlive(true);
		request.add("User-Agent","Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit (KHTML, like Gecko) Chrome Safari");
		string cookie_domain="."+host;
		for(size_t i=0;i<cookies.size();i++){
			Poco::Net::HTTPCookie& o = cookies[i];
			if(path.find(o.getPath())!=std::string::npos && cookie_domain.find("."+o.getDomain())){
				wcookies.add(o.getName(),o.getValue());
			}
		}
		if(wcookies.size()>0){
			request.setCookies(wcookies);
		}
		if(method=="POST"){
			request.setContentType("application/x-www-form-urlencoded");
			request.setContentLength(postdata.length());
			ostream& myOStream = session.sendRequest(request);
		  myOStream << postdata;
		}else{
			session.sendRequest(request);
		}
		//request.write(cout);
		Poco::Net::HTTPResponse res;
		istream& i = session.receiveResponse(res);
		size_t len = res.getContentLength();
		string body;
		body.resize(len);
		i.read(&body[0],len);

		std::vector < Poco::Net::HTTPCookie > vcookies;
		res.getCookies(vcookies);
		for(size_t i=0;i<vcookies.size();i++){
			Poco::Net::HTTPCookie& o = vcookies[i];
			lua_createtable(s,0,5);
			//domain, name, path, value, expire
			LuaSetTable(s,-3,"domain",o.getDomain().c_str());
			LuaSetTable(s,-3,"name",o.getName().c_str());
			LuaSetTable(s,-3,"path",o.getPath().c_str());
			LuaSetTable(s,-3,"value",o.getValue().c_str());
			LuaSetTable(s,-3,"expire",o.getMaxAge());
			size_t id = 0;
			lua_pushnil(s);
		  while(lua_next(s, 3) != 0){
				size_t key = lua_tointeger(s,-2);
				if(LuaGetStringTable(s,-2,"name")==o.getName() &&
					 LuaGetStringTable(s,-2,"domain")==o.getDomain() &&
					 LuaGetStringTable(s,-2,"path")==o.getPath()){
						 id = key;
				}
				lua_pop(s,1);
			}
			if(id==0){
				lua_rawseti(s,3,luaL_getn(s,3)+1);
			}else{
				lua_rawseti(s,3,id);
			}
		}
		lua_createtable(s, 0, 2);
		lua_pushstring(s, "code");
		lua_pushnumber(s, res.getStatus());
		lua_settable(s, -3);
		lua_pushstring(s, "body");
		lua_pushstring(s, body.c_str());
		lua_settable(s, -3);
		return 1;
	}catch(...){
		lua_createtable(s, 0, 2);
		lua_pushstring(s, "code");
		lua_pushnumber(s, 1000);
		lua_settable(s, -3);
		lua_pushstring(s, "body");
		lua_pushstring(s, "");
		lua_settable(s, -3);
		return 1;
	}
}

int LuaThread::_Lua_getConfigString(lua_State* s){
	const char* name = luaL_checkstring(s,1);
	string tmp = getConfigString(name);
	lua_pushstring(s,tmp.c_str());
	return 1;
}
int LuaThread::_Lua_getConfigInt(lua_State* s){
	const char* name = luaL_checkstring(s,1);
	size_t tmp = getConfigInt(name);
	lua_pushinteger(s,tmp);
	return 1;
}
int LuaThread::_Lua_getConfigBool(lua_State* s){
	const char* name = luaL_checkstring(s,1);
	bool tmp = getConfigBool(name);
	lua_pushboolean(s,tmp);
	return 1;
}
int LuaThread::_Lua_getHour(lua_State* s){
	time_t t = time(NULL);
	tm *tm_struct = localtime(&t);
	int hour = tm_struct->tm_hour;
	lua_pushnumber(s, hour);
	return 1;
}
