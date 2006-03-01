#define FILE_VERSION	0, 4, 2, 3
#define PRODUCT_VERSION	0, 4, 2, 3

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 2, UNICODE build 3"
	#define PRODUCT_VERSION_STR	"0, 4, 2, UNICODE build 3"
#else
	#define FILE_VERSION_STR	"0, 4, 2, DEBUG UNICODE build 3"
	#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG UNICODE build 3"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 2, build 3"
	#define PRODUCT_VERSION_STR	"0, 4, 2, build 3"
#else
	#define FILE_VERSION_STR	"0, 4, 2, DEBUG build 3"
	#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG build 3"
#endif
#endif
