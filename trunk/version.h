#define FILE_VERSION	0, 3, 2, 6
#define PRODUCT_VERSION	0, 3, 2, 6

#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 3, 2, build 6"
	#define PRODUCT_VERSION_STR	"0, 3, 2, build 6"
#else
	#define FILE_VERSION_STR	"0, 3, 2, DEBUG build 6"
	#define PRODUCT_VERSION_STR	"0, 3, 2, DEBUG build 6"
#endif
