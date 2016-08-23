#include <string>
#include <iostream>

using namespace std;

string encodeURI(string a){
	const char unencoded[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789;,/?:@&=+$-_.!~*'()#";
	const char hex[]="0123456789ABCDEF";
	char c;
	string buf;
	size_t len = a.size();
	for(size_t i=0;i<len;i++){
		c=a[i];
		size_t u=0;
		for(;unencoded[u]!=0;u++){
			if(unencoded[u]==c){
				buf+=c;
				break;
			}
		}
		if(unencoded[u]==0){
			buf+="%";
			buf+=hex[(c>>4)&0xf];
			buf+=hex[c&0xf];
		}
	}
	return buf;
}
