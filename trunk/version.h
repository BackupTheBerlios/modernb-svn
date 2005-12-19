#define FILE_VERSION	0, 4, 1, 1
#define PRODUCT_VERSION	0, 4, 1, 1

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 1"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 1"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 1"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 1"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 1"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 1"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 1"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 1"
#endif
#endif
