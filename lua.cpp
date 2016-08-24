#include "class.hpp"
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
void PrintTable(lua_State *s, int index, int shift);
extern std::string lua_script;

void writeLog(string a);
void writeLog(string a, bool nl);
void writeLog(size_t a);
void writeLog(size_t a, bool nl);
void writeLog(double a);
void writeLog(double a, bool nl);

/*
 * 0 - pause
 * 1 - resume
 * 2 - stop
*/
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
lua_State* LuaThread::initLua(){
	luaS=luaL_newstate();
	const luaL_Reg lualibs[] = {
		{"", luaopen_base},
		{LUA_TABLIBNAME, luaopen_table},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_MATHLIBNAME, luaopen_math},
		{LUA_IOLIBNAME, luaopen_io},
		{LUA_OSLIBNAME, luaopen_os},
		{NULL, NULL}
	};
	const luaL_Reg *lib = lualibs;
	for (; lib->func; lib++) {
		lua_pushcfunction(luaS, lib->func);
		lua_pushstring(luaS, lib->name);
		lua_call(luaS, 1, 0);
	}
	if(luaL_loadfile(luaS,lua_script.c_str())){
		writeLog("Lua fatal error: luaL_loadfile() failed",true);
		writeLog(lua_tostring(luaS,-1),true);
		lua_close(luaS);
		return 0;
	}
	if (lua_pcall(luaS, 0, 0, 0)){
		writeLog("Lua fatal error: lua_pcall() failed",true);
		writeLog(lua_tostring(luaS,-1),true);
		lua_close(luaS);
		return 0;
	}
	lua_register(luaS,"print",_Lua_print);
	lua_register(luaS,"mstime",_Lua_mstime);
	lua_register(luaS,"time",_Lua_time);
	lua_register(luaS,"getHour",_Lua_getHour);
	lua_register(luaS,"getReadableDate",_Lua_getReadableDate);
	lua_register(luaS,"TimeFromUT",_Lua_TimeFromUT);
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
	//config
	lua_register(luaS,"getConfigString",_Lua_getConfigString);
	lua_register(luaS,"getConfigInt",_Lua_getConfigInt);
	lua_register(luaS,"getConfigBool",_Lua_getConfigBool);
	return luaS;
}


int LuaThread::work_call(){
	m_lock(__LINE__);
	if(del){ //clean and go sleep
		if(work_acc!=0){
			work_acc->inthread=false;
		}
		work = 0;
		command = 0;
		if(luaS!=0){
			lua_close(luaS);
			luaS=0;
		}
		m_unlock(__LINE__);
		size_t sleep_after_work=5;
		sleep_after_work+=rand()%10;
		std::this_thread::sleep_for(std::chrono::seconds(sleep_after_work));
		return 1;
	}
	if(work!=0){ //init lua and tick
		//cout<<"work"<<endl;
		if(luaS==0){ //init lua
			m_unlock(__LINE__);
			work_lock();
			if(work==0 || del || work_acc == 0){
				del = true;
				work_unlock();
				return 0;
			}
			if(initLua()==0){
				writeLog("error lua cannot be create",true);
				exit(1);
			}
			lua_getfield(luaS, LUA_GLOBALSINDEX, "Postac");
			//lua_pushlightuserdata(luaS, this);
			lua_pushstring(luaS, work_acc->getLogin().c_str());
			lua_pushstring(luaS, work_acc->getPassword().c_str());
			lua_pushstring(luaS, work->getNick().c_str());
			lua_pushstring(luaS, work->getWorld().c_str());
			lua_pushinteger(luaS,work->getMaxLVL());
			//stworzenie tablicy zawierajacej cookies
			size_t wacc = work_acc->cookies.size();
			lua_newtable(luaS);
			for(size_t i=0;i<wacc;i++){
				Cookie& o = work_acc->cookies[i];
				lua_createtable(luaS,0,5);
				//domain, name, path, value, expire
				LuaSetTable(luaS,8,"domain",o.getDomain().c_str());
				LuaSetTable(luaS,8,"name",o.getName().c_str());
				LuaSetTable(luaS,8,"path",o.getPath().c_str());
				LuaSetTable(luaS,8,"value",o.getValue().c_str());
				LuaSetTable(luaS,8,"expire",o.getExpires());
				lua_rawseti(luaS,7,luaL_getn(luaS,7)+1);
			}
			//PrintTable(luaS,7,0);
			//_Lua_print_stack(luaS);
			work_unlock();
			m_lock(__LINE__);
			if(lua_pcall(luaS, 6, 1, 0) != 0){
				writeLog("error running function 'new Postac': ");
				writeLog(lua_tostring(luaS,-1),true);
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
			m_unlock(__LINE__);
			work_lock();
			if(work_acc == 0 || work == 0 || del){
				del = true;
				work_unlock();
				return 0;
			}
			work_unlock();
			m_lock(__LINE__);
			if(lua_pcall(luaS, 1, 3, 0) != 0){
				writeLog("error running function 'account.tick': ");
				writeLog(lua_tostring(luaS,-1),true);
				exit(1);
			}
			size_t sleep_time = lua_tointeger(luaS,-3);
			size_t logout_time = lua_tointeger(luaS,-2);
			if(lua_istable(luaS,-1)){
				work_lock();
				if(work_acc == 0 || work == 0 || del){
					del = true;
					work_unlock();
					return 0;
				}
				//umieszczenie cookies z lua w pamieciu konta
				work_acc->cookies.clear();
				lua_pushnil(luaS);
			  while(lua_next(luaS, -2) != 0){
					Cookie tc;
					tc.setDomain(LuaGetStringTable(luaS,-2,"domain"));
					tc.setPath(LuaGetStringTable(luaS,-2,"path"));
					tc.setName(LuaGetStringTable(luaS,-2,"name"));
					tc.setValue(LuaGetStringTable(luaS,-2,"value"));
					tc.setExpires(LuaGetIntTable(luaS,-2,"expire"));
					work_acc->cookies.push_back(tc);
					lua_pop(luaS,1);
				}
				//sprawdzenie czy cookies nie wygasly
				std::chrono::seconds req =
					std::chrono::duration_cast< std::chrono::seconds >(
						std::chrono::system_clock::now().time_since_epoch()
					);
				size_t t=req.count();
				size_t i=work_acc->cookies.size();
				do{
					i--;
					Cookie& o = work_acc->cookies[i];
					if(o.getExpires()<t){
						work_acc->cookies.erase(work_acc->cookies.begin()+i);
					}
				}while(i!=0);
				work_unlock();
			}
			lua_pop(luaS, 3);
			m_unlock(__LINE__);
			if(logout_time==0){
				if(sleep_time>0){
					std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
				}
			}else{
				//cout<<"logout "<<(logout_time)<<endl;
				work_lock();
				std::chrono::seconds req =
					std::chrono::duration_cast< std::chrono::seconds >(
						std::chrono::system_clock::now().time_since_epoch()
					);
				size_t t=req.count();
				work->setNext(t+logout_time);
				work_unlock();
				m_lock(__LINE__);
				del = true;
				m_unlock(__LINE__);
			}
			return 0;
		}else{
			writeLog("error account:tick must be function",true);
			exit(1);
		}
		return 0;
	}
	m_lock(__LINE__);
	del = true;
	m_unlock(__LINE__);
	return 0;
}

void LuaThread::m_lock(int a){
	//cout<<"m_lock "<<a<<endl;
	m->lock();
}
void LuaThread::m_unlock(int a){
	//cout<<"m_unlock "<<a<<endl;
	m->unlock();
}
void LuaThread::thread(){
	while(1){
		m_lock(__LINE__);
		int _command=command;
		m_unlock(__LINE__);
		switch(_command){
			case 0:{//wait
				std::unique_lock<std::mutex> lk(*m);
				cv->wait(lk);
			}break;
			case 1:{
				int low_command = work_call();
				if(low_command==0){//ok

				}else if(low_command==1){//go to sleep

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
	writeLog("table",true);
  while(lua_next(s, index) != 0){
		int key_type = lua_type(s,-2);
		int val_type = lua_type(s,-1);
		writeLog(supp+"[");
		switch(key_type){
			case LUA_TSTRING:
				writeLog(std::string(lua_tostring(s,-2),lua_strlen(s,-2)));
			break;
			case LUA_TNUMBER:
				writeLog(lua_tonumber(s,-2));
			break;
		}
		writeLog("] = ");
		switch(val_type){
			case LUA_TSTRING:
				writeLog(std::string(lua_tostring(s,-1),lua_strlen(s,-1)),true);
			break;
			case LUA_TBOOLEAN:
				writeLog((lua_toboolean(s,-1)?"true":"false"),true);
			break;
			case LUA_TNUMBER:
				if(lua_tonumber(s,-1)==lua_tointeger(s,-1)){
					writeLog((size_t)lua_tointeger(s,-1),true);
				}else{
					writeLog(lua_tonumber(s,-1),true);
				}
			break;
			case LUA_TTABLE:{
				PrintTable(s,-2,shift+1);
			}break;
			default:
				writeLog(lua_typename(s,val_type),true);
			break;
		}
		lua_pop(s,1);
	}
}

int LuaThread::_Lua_print_stack(lua_State* s){
	print_mutex.lock();
	int n = lua_gettop(s);
	for(int i=1;i<=n;i++){
		int typ = lua_type(s,i);
		switch(typ){
			case LUA_TSTRING:
				writeLog(std::string(lua_tostring(s,i),lua_strlen(s,i)),true);
			break;
			case LUA_TBOOLEAN:
				writeLog((lua_toboolean(s,i)?"true":"false"),true);
			break;
			case LUA_TNUMBER:
				writeLog(lua_tonumber(s,i),true);
			break;
			default:
				writeLog(lua_typename(s,typ),true);
			break;
		}
	}
	print_mutex.unlock();
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
				writeLog(std::string(lua_tostring(s,i),lua_strlen(s,i)),true);
			break;
			case LUA_TBOOLEAN:
				writeLog((lua_toboolean(s,i)?"true":"false"),true);
			break;
			case LUA_TNUMBER:
				writeLog(lua_tonumber(s,i),true);
			break;
			case LUA_TTABLE:{
				PrintTable(s,i,0);
			}break;
			default:
				writeLog(lua_typename(s,typ),true);
			break;
		}
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
	if(n==2 && lua_isstring(s,1) && lua_isstring(s,2)){
		file* nf = new file(lua_tostring(s,1),std::fstream::in|std::fstream::binary,lua_tostring(s,2));
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
string encodeURI(string a);
int LuaThread::_Lua_http_request(lua_State* s){
	//url, method, cookies, postdata, additional
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
	string path = encodeURI(url.substr(br));
	string method = "GET";
	if(lua_isstring(s,2)){
		method = lua_tostring(s,2);
	}else{
		lua_pushstring(s,"wrong method");
		return 1;
	}
	std::chrono::seconds req =
		std::chrono::duration_cast< std::chrono::seconds >(
			std::chrono::system_clock::now().time_since_epoch()
		);
	size_t time_sec=req.count();
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
				tc.setMaxAge(time_sec-LuaGetIntTable(s,-2,"expire"));
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
			LuaSetTable(s,-3,"expire",time_sec+o.getMaxAge());
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
		lua_pushstring(s, "err");
		lua_pushstring(s, "");
		lua_settable(s, -3);
		return 1;
	}
}

int LuaThread::_Lua_getConfigString(lua_State* s){
	try{
		const char* name = luaL_checkstring(s,1);
		string tmp = getConfigString(name);
		lua_pushstring(s,tmp.c_str());
	}catch(...){
		lua_pushnil(s);
	}
	return 1;
}
int LuaThread::_Lua_getConfigInt(lua_State* s){
	try{
		const char* name = luaL_checkstring(s,1);
		size_t tmp = getConfigInt(name);
		lua_pushinteger(s,tmp);
	}catch(...){
		lua_pushnil(s);
	}
	return 1;
}
int LuaThread::_Lua_getConfigBool(lua_State* s){
	try{
		const char* name = luaL_checkstring(s,1);
		bool tmp = getConfigBool(name);
		lua_pushboolean(s,tmp);
	}catch(...){
		lua_pushnil(s);
	}
	return 1;
}
int LuaThread::_Lua_getHour(lua_State* s){
	time_t t = time(NULL);
	tm *tm_struct = localtime(&t);
	int hour = tm_struct->tm_hour;
	lua_pushnumber(s, hour);
	return 1;
}
int LuaThread::_Lua_getReadableDate(lua_State* s){
	time_t t = time(NULL);
	tm *tm_struct = localtime(&t);
	int tm_year = 1900 + tm_struct->tm_year;
	int tm_mon = 1 + tm_struct->tm_mon;
	int tm_mday = tm_struct->tm_mday;
	string buf = "";
	buf+=std::to_string(tm_year);
	buf+="-";
	buf+=std::to_string(tm_mon);
	buf+="-";
	buf+=std::to_string(tm_mday);
	lua_pushstring(s, buf.c_str());
	return 1;
}

int LuaThread::_Lua_TimeFromUT(lua_State* s){
	time_t ut = luaL_checknumber(s,1);
	tm *tm_struct = localtime(&ut);
	int tm_sec = tm_struct->tm_sec;
	int tm_min = tm_struct->tm_min;
	int tm_hour	=tm_struct->tm_hour;
	int tm_mday =tm_struct->tm_mday;
	int tm_mon = 1 + tm_struct->tm_mon;
	int tm_year = 1900 + tm_struct->tm_year;
	string buf = "";
	buf += std::to_string(tm_hour);
	buf += ":";
	if(tm_min<10){
		buf+="0";
	}
	buf += std::to_string(tm_min);
	buf += ":";
	if(tm_sec<10){
		buf+="0";
	}
	buf += std::to_string(tm_sec);
	buf += " ";
	if(tm_mday<10){
		buf+="0";
	}
	buf += std::to_string(tm_mday);
	buf += "-";
	if(tm_mon<10){
		buf+="0";
	}
	buf += std::to_string(tm_mon);
	buf += "-";
	buf += std::to_string(tm_year);
	lua_pushstring(s, buf.c_str());
	return 1;
}
