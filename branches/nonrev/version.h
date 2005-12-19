#define FILE_VERSION	0, 3, 3, 9
#define PRODUCT_VERSION	0, 3, 3, 9

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 3, 3, UNICODE build 9"
	#define PRODUCT_VERSION_STR	"0, 3, 3, UNICODE build 9"
#else
	#define FILE_VERSION_STR	"0, 3, 3, DEBUG UNICODE build 9"
	#define PRODUCT_VERSION_STR	"0, 3, 3, DEBUG UNICODE build 9"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 3, 3, build 9"
	#define PRODUCT_VERSION_STR	"0, 3, 3, build 9"
#else
	#define FILE_VERSION_STR	"0, 3, 3, DEBUG build 9"
	#define PRODUCT_VERSION_STR	"0, 3, 3, DEBUG build 9"
#endif
#endif
