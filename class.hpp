#include <string>
#include <ctime>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <lua5.1/lua.hpp>

std::string getConfigString(std::string name);
size_t getConfigInt(std::string name);
bool getConfigBool(std::string name);

class Account{
	std::string login;
	std::string password;
public:
	bool inthread;
	Account(std::string _login, std::string _password);
	~Account();
	std::string getLogin();
	std::string getPassword();
	void setPassword(std::string p);
};

class Character{
	std::string login;
	std::string world;
	std::string nick;
	size_t max_lvl;
	time_t next_use;
public:
	Character(std::string _login, std::string _world, std::string _nick);
	Character(const Character& a);
	Account* getParent();
	bool isParent(Account* _par);
	std::string getWorld();
	std::string getNick();
	time_t getNext();
	void setNext(time_t t);
	std::string getLogin();
};

class LuaThread{
	std::thread* worker;
	std::mutex* m;
	std::condition_variable* cv;
	int command;
	void send_command(int a);
	Character* work;
	Account* work_acc;
	bool del;
	lua_State* luaS;
	static void start_thread(LuaThread* a);
	void thread();
	void lock();
	void unlock();
	size_t id;
	static size_t numbers;
	void clear_thread();
public:
	LuaThread();
	~LuaThread();
	bool isFree();
	size_t getId();
	bool addChar(Character* p);
	void delChar();
	bool isChar(Character* p);
	bool isAcc(Account* a);
	Character* getChar();


	//Lua functions
	static int _Lua_sleep(lua_State* s);
	static int _Lua_print(lua_State* s);
	static int _Lua_mstime(lua_State* s);
	static int _Lua_time(lua_State* s);
	static int _Lua_setnext(lua_State* s);
	static int _Lua_md5(lua_State* s);
	static int _Lua_sha1(lua_State* s);
	static int _Lua_getHour(lua_State* s);

	static std::mutex print_mutex;

	static int _Lua_http_request(lua_State* s);

	static int _Lua_getConfigString(lua_State* s);
	static int _Lua_getConfigInt(lua_State* s);
	static int _Lua_getConfigBool(lua_State* s);

	//file
	static int _Lua_File_open(lua_State* s);
	static int _Lua_File_close(lua_State* s);
	static int _Lua_File_isGood(lua_State* s);
	static int _Lua_File_isEOF(lua_State* s);
	static int _Lua_File_getLine(lua_State* s);
	static int _Lua_Linia_isCommented(lua_State* s);
	static int _Lua_Linia_countVals(lua_State* s);
	static int _Lua_Linia_getVal(lua_State* s);

	//debugging
	static int traceback (lua_State *L);
};

int _Lua_rj_parse(lua_State* s);
int _Lua_rj_type(lua_State* s);
int _Lua_rj_get(lua_State* s);
int _Lua_rj_at(lua_State* s);
int _Lua_rj_clean(lua_State* s);
