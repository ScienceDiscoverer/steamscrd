#define CURL_STATICLIB
#include "ext/curl/curl.h"

#ifdef _DEBUG
#pragma comment( lib, "ext/curl/libcurl_a_debug" )
#else
#pragma comment( lib, "ext/curl/libcurl_a" )
#endif

#pragma comment( lib, "Normaliz" )
#pragma comment( lib, "ws2_32" )
#pragma comment( lib, "wldap32" )
#pragma comment( lib, "crypt32" )
#pragma comment( lib, "advapi32" )