#define FILE_VERSION	0, 4, 2, 4
#define PRODUCT_VERSION	0, 4, 2, 4

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 2, UNICODE build 4"
	#define PRODUCT_VERSION_STR	"0, 4, 2, UNICODE build 4"
#else
	#define FILE_VERSION_STR	"0, 4, 2, DEBUG UNICODE build 4"
	#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG UNICODE build 4"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 2, build 4"
	#define PRODUCT_VERSION_STR	"0, 4, 2, build 4"
#else
	#define FILE_VERSION_STR	"0, 4, 2, DEBUG build 4"
	#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG build 4"
#endif
#endif
