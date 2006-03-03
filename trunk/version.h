#define FILE_VERSION	0, 4, 2, 5
#define PRODUCT_VERSION	0, 4, 2, 5

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 2, UNICODE build 5"
	#define PRODUCT_VERSION_STR	"0, 4, 2, UNICODE build 5"
#else
	#define FILE_VERSION_STR	"0, 4, 2, DEBUG UNICODE build 5"
	#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG UNICODE build 5"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 2, build 5"
	#define PRODUCT_VERSION_STR	"0, 4, 2, build 5"
#else
	#define FILE_VERSION_STR	"0, 4, 2, DEBUG build 5"
	#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG build 5"
#endif
#endif
