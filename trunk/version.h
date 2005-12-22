#define FILE_VERSION	0, 4, 1, 3
#define PRODUCT_VERSION	0, 4, 1, 3

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 3"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 3"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 3"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 3"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 3"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 3"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 3"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 3"
#endif
#endif
