#define FILE_VERSION	0, 4, 2, 7
#define PRODUCT_VERSION	0, 4, 2, 7

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 2, UNICODE build 7"
	#define PRODUCT_VERSION_STR	"0, 4, 2, UNICODE build 7"
#else
	#define FILE_VERSION_STR	"0, 4, 2, DEBUG UNICODE build 7"
	#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG UNICODE build 7"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 2, build 7"
	#define PRODUCT_VERSION_STR	"0, 4, 2, build 7"
#else
	#define FILE_VERSION_STR	"0, 4, 2, DEBUG build 7"
	#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG build 7"
#endif
#endif
