#include "curl.hpp"

using namespace std;


bool Cookie::validHost(std::string host){
	std::string tdomain, tpath;
	size_t br=host.find("//");
	if(br!=std::string::npos){
		host=host.substr(br+2);
		br=host.find("/");
		if(br!=std::string::npos){
			tdomain=host.substr(0,br);
			tpath=host.substr(br);
		}
	}
	if(tpath.find(path)!=std::string::npos){
		std::string fdomain="."+domain;
		tdomain="."+tdomain;
		if(tdomain.find(fdomain)!=std::string::npos){
			return true;
		}
	}
	return false;
}


CURL_myobj::CURL_myobj(){
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headercb);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
}
CURL_myobj::~CURL_myobj(){
	curl_easy_cleanup(curl);
	while(cookies.size()>0){
		delete cookies[0];
		cookies.erase(cookies.begin());
	}
}
void CURL_myobj::reset(){
  curl_easy_setopt(curl, CURLOPT_POST, 0);
  response="";
}
CURL* CURL_myobj::getCurl(){
	return curl;
}
std::string& CURL_myobj::getResponse(){
	return response;
}
int CURL_myobj::_Lua_curl_reset(lua_State* s){
	int n = lua_gettop(s);
	if(n==1 && lua_isuserdata(s,1)){
		CURL_myobj* pt=(CURL_myobj*)lua_touserdata(s,1);
		pt->reset();
	}
	return 0;
}
int CURL_myobj::_Lua_curl_set(lua_State* s){
	int n = lua_gettop(s);
	if(n==3 && lua_isuserdata(s,1) && lua_isstring(s,2)){
		CURL_myobj* pt=(CURL_myobj*)lua_touserdata(s,1);
		std::string type=lua_tostring(s,2);

		if(type=="CURLOPT_URL" && lua_isstring(s,3)){
			curl_easy_setopt(pt->curl, CURLOPT_URL, lua_tostring(s,3));
			const char* str=lua_tostring(s,3);
			if(str!=NULL){
				pt->url=lua_tostring(s,3);
			}else{
				pt->url="";
			}
		}else if(type=="CURLOPT_REFERER" && lua_isstring(s,3)){
			curl_easy_setopt(pt->curl, CURLOPT_REFERER, lua_tostring(s,3));
		}else if(type=="CURLOPT_POSTFIELDSIZE" && lua_isnumber(s,3)){
			curl_easy_setopt(pt->curl, CURLOPT_POSTFIELDSIZE, lua_tointeger(s,3));
		}else if(type=="CURLOPT_POST" && lua_isnumber(s,3)){
			curl_easy_setopt(pt->curl, CURLOPT_POST, lua_tointeger(s,3));
		}else if(type=="CURLOPT_USERAGENT" && lua_isstring(s,3)){
			curl_easy_setopt(pt->curl, CURLOPT_USERAGENT, lua_tostring(s,3));
		}else if(type=="CURLOPT_COOKIE" && lua_isstring(s,3)){
			curl_easy_setopt(pt->curl, CURLOPT_COOKIE, lua_tostring(s,3));
		}else if(type=="CURLOPT_POSTFIELDS" && lua_isstring(s,3)){
			curl_easy_setopt(pt->curl, CURLOPT_POSTFIELDS, lua_tostring(s,3));
		}else{
			cout<<"type "<<type<<endl;
		}
	}
	return 0;
}
int CURL_myobj::_Lua_add_cookie(lua_State* s){
	int n = lua_gettop(s);
	if(n==6 && lua_isuserdata(s,1) && lua_isstring(s,2)  && lua_isnumber(s,3)  && lua_isstring(s,4)  && lua_isstring(s,5) && lua_isstring(s,6)){
		CURL_myobj* pt=(CURL_myobj*)lua_touserdata(s,1);
		pt->addCookie(lua_tostring(s,2), lua_tostring(s,3), lua_tointeger(s,4), lua_tostring(s,5),lua_tostring(s,6));
}
	return 0;
}
size_t CURL_myobj::writefunc(const char *ptr, size_t size, size_t nmemb, void *stream){
	std::string tmp(ptr,(size*nmemb));
	((std::string*)stream)->append(tmp);
	return (size*nmemb);
}
size_t CURL_myobj::headercb(char *ptr, size_t size, size_t nitems, void *a){
	CURL_myobj* r=(CURL_myobj*)a;
	size_t length=size*nitems;
	std::string row,name,value;
	size_t br;
	char* start=ptr;
	for(size_t i=1;i<length;i++){
		if(ptr[i-1]=='\r' && ptr[i]=='\n'){
			row=std::string(start,(ptr+i-1-start));
			br=row.find(": ");
			if(br!=std::string::npos){
				name=row.substr(0,br);
				value=row.substr(br+2);
				if(name=="Location"){
					if(value.size()>0 && value[0]=='/'){
						size_t brl=r->url.find("//");
						if(brl!=std::string::npos){
							brl=r->url.find("/",brl+2);
							if(brl!=std::string::npos){
								r->url=r->url.substr(0,brl);
								r->url+=value;
								curl_easy_setopt(r->curl, CURLOPT_URL, r->url.c_str());
								r->redirect++;
							}else{
								r->url=value;
								curl_easy_setopt(r->curl, CURLOPT_URL, r->url.c_str());
								r->redirect++;
							}
						}else{
							r->url=value;
							curl_easy_setopt(r->curl, CURLOPT_URL, r->url.c_str());
							r->redirect++;
						}
					}else{
						r->url=value;
						curl_easy_setopt(r->curl, CURLOPT_URL, r->url.c_str());
						r->redirect++;
					}
				}
				if(name=="Set-Cookie"){
					r->addCookie(value,r->url);
				}
			}
			start=ptr+i+1;
		}
	}
	return length;
}
void CURL_myobj::addCookie(std::string& http, std::string host){
	std::string name, value, time, domain, path;
	size_t br=http.find("; ");
	if(br!=std::string::npos){
		std::string needed=http.substr(0,br);
		std::string addtional=http.substr(br+2);
		br=needed.find("=");
		if(br!=std::string::npos){
			name=needed.substr(0,br);
			value=needed.substr(br+1);
			br=addtional.find("expires=");
			if(br!=std::string::npos){
				time=addtional.substr(br+8);
				br=time.find(";");
				if(br!=std::string::npos){
					time=time.substr(0,br);
				}
			}
			br=addtional.find("domain=");
			if(br!=std::string::npos){
				domain=addtional.substr(br+7);
				br=domain.find(";");
				if(br!=std::string::npos){
					domain=domain.substr(0,br);
				}
			}else{
				br=host.find("//");
				if(br!=std::string::npos){
					domain=host.substr(br+2);
					br=domain.find("/");
					if(br!=std::string::npos){
						domain=domain.substr(0,br);
					}
				}
			}
			br=addtional.find("path=");
			if(br!=std::string::npos){
				path=addtional.substr(br+5);
				br=path.find(";");
				if(br!=std::string::npos){
					path=path.substr(0,br);
				}
			}else{
				br=host.find("//");
				if(br!=std::string::npos){
					path=host.substr(br+2);
					br=path.find("/");
					if(br!=std::string::npos){
						path=path.substr(br);
					}
				}
			}
			time_t rtime=0;
			tm tmpt={0};
			std::string part;
			std::string miesiace[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
			br=time.find(", ");
			if(br!=std::string::npos){
				time=time.substr(br+2);
				br=time.find("-");
				if(br!=std::string::npos){
					part=time.substr(0,br);
					time=time.substr(br+1);
					tmpt.tm_mday=stoi(part);
					br=time.find("-");
					if(br!=std::string::npos){
						part=time.substr(0,br);
						time=time.substr(br+1);
						for(size_t i=0;i<12;i++){
							if(part==miesiace[i]){
								tmpt.tm_mon=i;
								break;
							}
						}
						br=time.find(" ");
						if(br!=std::string::npos){
							part=time.substr(0,br);
							time=time.substr(br+1);
							tmpt.tm_year=stoi(part)-1900;
							br=time.find(":");
							if(br!=std::string::npos){
								part=time.substr(0,br);
								time=time.substr(br+1);
								tmpt.tm_hour=stoi(part);
								br=time.find(":");
								if(br!=std::string::npos){
									part=time.substr(0,br);
									time=time.substr(br+1);
									tmpt.tm_min=stoi(part);
									br=time.find(" ");
									if(br!=std::string::npos){
										part=time.substr(0,br);
										time=time.substr(br+1);
										tmpt.tm_sec=stoi(part);
										rtime=timegm(&tmpt);
									}
								}
							}
						}
					}
				}
			}
			addCookie(name,value,rtime,domain,path);
		}
	}
}
Cookie* CURL_myobj::addCookie(std::string name, std::string value, time_t time, std::string domain, std::string path){
	size_t l=cookies.size();
	for(size_t i=0;i<l;i++){
		if(cookies[i]->name==name && cookies[i]->domain==domain && cookies[i]->path==path){
			cookies[i]->value=value;
			cookies[i]->expires=time;
			return 0;
		}
	}
	Cookie* nc=new Cookie();
	nc->name=name;
	nc->value=value;
	nc->expires=time;
	nc->domain=domain;
	nc->path=path;
	cookies.push_back(nc);
	return nc;
}
int CURL_myobj::_Lua_curl_exec(lua_State* s){
	int n = lua_gettop(s);
	if(n==1 && lua_isuserdata(s,1)){
		CURL_myobj* pt=(CURL_myobj*)lua_touserdata(s,1);
    std::string ret_cookies;
  	size_t l=pt->cookies.size();
  	for(size_t i=0;i<l;i++){
  		if(pt->cookies[i]->validHost(pt->url)){
  			ret_cookies+=pt->cookies[i]->name+"="+pt->cookies[i]->value+"; ";
  		}
  	}
  	if(ret_cookies.size()>2){
  		ret_cookies=ret_cookies.substr(0,ret_cookies.size()-2);
  	}
    curl_easy_setopt(pt->getCurl(),CURLOPT_COOKIE,ret_cookies.c_str());
		CURLcode res=curl_easy_perform(pt->getCurl());
		lua_pushinteger(s,res);
		return 1;
	}
	return 0;
}
int CURL_myobj::_Lua_curl_get_body(lua_State* s){
	int n = lua_gettop(s);
	if(n==1 && lua_isuserdata(s,1)){
		CURL_myobj* pt=(CURL_myobj*)lua_touserdata(s,1);
		lua_pushstring(s,pt->getResponse().c_str());
		return 1;
	}
	return 0;
}
