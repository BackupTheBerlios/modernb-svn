#define FILE_VERSION	0, 3, 3, 8
#define PRODUCT_VERSION	0, 3, 3, 8

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 3, 3, UNICODE build 8"
	#define PRODUCT_VERSION_STR	"0, 3, 3, UNICODE build 8"
#else
	#define FILE_VERSION_STR	"0, 3, 3, DEBUG UNICODE build 8"
	#define PRODUCT_VERSION_STR	"0, 3, 3, DEBUG UNICODE build 8"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 3, 3, build 8"
	#define PRODUCT_VERSION_STR	"0, 3, 3, build 8"
#else
	#define FILE_VERSION_STR	"0, 3, 3, DEBUG build 8"
	#define PRODUCT_VERSION_STR	"0, 3, 3, DEBUG build 8"
#endif
#endif
