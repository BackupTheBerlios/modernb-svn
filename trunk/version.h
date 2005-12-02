#define FILE_VERSION	0, 3, 2, 22
#define PRODUCT_VERSION	0, 3, 2, 22

#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 3, 2, build 22"
	#define PRODUCT_VERSION_STR	"0, 3, 2, build 22"
#else
	#define FILE_VERSION_STR	"0, 3, 2, DEBUG build 22"
	#define PRODUCT_VERSION_STR	"0, 3, 2, DEBUG build 22"
#endif
