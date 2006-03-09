#define FILE_VERSION	0, 4, 2, 8
#define PRODUCT_VERSION	0, 4, 2, 8

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 2, UNICODE build 8"
	#define PRODUCT_VERSION_STR	"0, 4, 2, UNICODE build 8"
#else
	#define FILE_VERSION_STR	"0, 4, 2, DEBUG UNICODE build 8"
	#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG UNICODE build 8"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 2, build 8"
	#define PRODUCT_VERSION_STR	"0, 4, 2, build 8"
#else
	#define FILE_VERSION_STR	"0, 4, 2, DEBUG build 8"
	#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG build 8"
#endif
#endif
