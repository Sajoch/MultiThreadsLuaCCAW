#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <lua5.1/lua.hpp>
#include "class.hpp"
using namespace rapidjson;

void object(lua_State* s, Value& a);
void array(lua_State* s, Value& a){
  lua_newtable(s);
  for (SizeType i = 0; i < a.Size(); i++){
    if(a[i].IsArray()){
      array(s,a[i]);
    }else if(a[i].IsObject()){
      object(s,a[i]);
    }else if(a[i].IsNull()){
      lua_pushnil(s);
    }else if(a[i].IsBool()){
      lua_pushinteger(s,a[i].GetBool()?1:0);
    }else if(a[i].IsString()){
      lua_pushstring(s,a[i].GetString());
    }else if(a[i].IsInt64()){
      lua_pushinteger(s,a[i].GetInt64());
    }else if(a[i].IsDouble()){
      lua_pushnumber(s,a[i].GetDouble());
    }
    lua_rawseti(s,-2,i+1);
  }
}
void object(lua_State* s, Value& a){
  lua_newtable(s);
  for (Value::MemberIterator itr = a.MemberBegin(); itr != a.MemberEnd(); ++itr){
    if(itr->value.IsArray()){
      array(s,itr->value);
    }else if(itr->value.IsObject()){
      object(s,itr->value);
    }else if(itr->value.IsNull()){
      lua_pushnil(s);
    }else if(itr->value.IsBool()){
      lua_pushinteger(s,itr->value.GetBool()?1:0);
    }else if(itr->value.IsString()){
      lua_pushstring(s,itr->value.GetString());
    }else if(itr->value.IsInt64()){
      lua_pushinteger(s,itr->value.GetInt64());
    }else if(itr->value.IsDouble()){
      lua_pushnumber(s,itr->value.GetDouble());
    }
    lua_setfield(s,-2,itr->name.GetString());
  }
}
int _Lua_rj_parse(lua_State* s){
	Document ret;
;
	ret.Parse(lua_tostring(s,1));//
  if(ret.IsArray()){
    array(s, ret);
  }else if(ret.IsObject()){
    object(s,ret);
  }else{
    if(ret.IsNull()){
      lua_pushnil(s);
    }else if(ret.IsBool()){
      lua_pushinteger(s,ret.GetBool()?1:0);
    }else if(ret.IsString()){
      lua_pushstring(s,ret.GetString());
    }else if(ret.IsInt64()){
      lua_pushinteger(s,ret.GetInt64());
    }else if(ret.IsDouble()){
      lua_pushnumber(s,ret.GetDouble());
    }
  }
	return 1;
}
