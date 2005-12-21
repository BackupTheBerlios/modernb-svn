#define FILE_VERSION	0, 4, 1, 2
#define PRODUCT_VERSION	0, 4, 1, 2

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 2"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 2"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 2"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 2"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 2"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 2"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 2"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 2"
#endif
#endif
