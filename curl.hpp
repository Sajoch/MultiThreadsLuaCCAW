#include <curl/curl.h>
#include <lua5.1/lua.hpp>
#include <string>
#include <iostream>
#include <vector>

class Cookie{
public:
	std::string name;
	std::string value;
	std::string domain;
	time_t expires;
	std::string path;
	bool validHost(std::string host);
};

class CURL_myobj{
	std::string url;
	size_t redirect;
	CURL *curl;
	std::string response;
  std::vector<Cookie*> cookies;
public:
	CURL_myobj();
	~CURL_myobj();
	void reset();
	CURL* getCurl();
	std::string& getResponse();
  void addCookie(std::string& http, std::string host);
  Cookie* addCookie(std::string name, std::string value, time_t time, std::string domain, std::string path);
	static int _Lua_curl_reset(lua_State* s);
	static int _Lua_curl_set(lua_State* s);
  static int _Lua_curl_exec(lua_State* s);
  static int _Lua_curl_get_body(lua_State* s);
	static int _Lua_add_cookie(lua_State* s);
	static size_t writefunc(const char *ptr, size_t size, size_t nmemb, void *stream);
	static size_t headercb(char *ptr, size_t size, size_t nitems, void *a);
};
