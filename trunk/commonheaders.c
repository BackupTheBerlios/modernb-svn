#include "commonheaders.h"


#define SAFE_PTR(a) a?(IsBadReadPtr(a,1)?a=NULL:a):a
#define SAFE_STR(a) a?a:""



int mir_realloc_proxy(void *ptr,int size)
{
	/*	if (IsBadCodePtr((FARPROC)ptr))
		{
			char buf[256];
			mir_snprintf(buf,sizeof(buf),"Bad code ptr in mir_realloc_proxy ptr: %x\r\n",ptr);
			//ASSERT("Bad code ptr");		
			TRACE(buf);
			DebugBreak();
			return 0;
		}
		*/
	memoryManagerInterface.mmi_realloc(ptr,size);
	return 0;

}


int mir_free_proxy(void *ptr)
{
	if (ptr==NULL) //||IsBadCodePtr((FARPROC)ptr))
		return 0;
    memoryManagerInterface.mmi_free(ptr);
	return 0;
}
BOOL __cdecl strstri(const char *a, const char *b)
{
    char * x, *y;
	SAFE_PTR(a);
	SAFE_PTR(b);
    if (!a || !b) return FALSE;
    x=_strdup(a);
    y=_strdup(b);
    x=_strupr(x);
    y=_strupr(y);
    if (strstr(x,y))
    {
        
        free(x);
        //if (x!=y) 
            free(y);
        return TRUE;
    }
    free(x);
   // if (x!=y) 
        free(y);
    return FALSE;
}
int __cdecl mir_strcmpi(const char *a, const char *b)
{
	SAFE_PTR(a);
	SAFE_PTR(b);
	if (a==NULL && b==NULL) return 0;
	if (a==NULL || b==NULL) return _stricmp(a?a:"",b?b:"");
    return _stricmp(a,b);
}

int __cdecl mir_tstrcmpi(const TCHAR *a, const TCHAR *b)
{
	SAFE_PTR(a);
	SAFE_PTR(b);
	if (a==NULL && b==NULL) return 0;
	if (a==NULL || b==NULL) return _tcsicmp(a?a:TEXT(""),b?b:TEXT(""));
	return _tcsicmp(a,b);
}
BOOL __cdecl mir_bool_strcmpi(const char *a, const char *b)
{
	SAFE_PTR(a);
	SAFE_PTR(b);
	if (a==NULL && b==NULL) return 1;
	if (a==NULL || b==NULL) return _stricmp(a?a:"",b?b:"")==0;
    return _stricmp(a,b)==0;
}

BOOL __cdecl mir_bool_tstrcmpi(const TCHAR *a, const TCHAR *b)
{
	SAFE_PTR(a);
	SAFE_PTR(b);
	if (a==NULL && b==NULL) return 1;
	if (a==NULL || b==NULL) return _tcsicmp(a?a:TEXT(""),b?b:TEXT(""))==0;
	return _tcsicmp(a,b)==0;
}

#ifdef strlen
#undef strcmp
#undef strlen
#endif

int __cdecl mir_strcmp (const char *a, const char *b)
{
	SAFE_PTR(a);
	SAFE_PTR(b);
	if (!(a&&b)) return a!=b;
	return (strcmp(a,b));
};

_inline int mir_strlen (const char *a)	
{	

	SAFE_PTR(a);
	if (a==NULL) return 0;	
	return (strlen(a));	
};	
 	 	
#define strlen(a) mir_strlen(a)
#define strcmp(a,b) mir_strcmp(a,b)


 	 	
__inline void *mir_calloc( size_t num, size_t size )
{
 	void *p=mir_alloc(num*size);
	if (p==NULL) return NULL;
	memset(p,0,num*size);
    return p;
};

extern __inline wchar_t * mir_strdupW(const wchar_t * src)
{
	wchar_t * p;
	if (src==NULL) return NULL;
	if (IsBadStringPtrW(src,255)) return NULL;
	p=(wchar_t *) mir_alloc((lstrlenW(src)+1)*sizeof(wchar_t));
	if (!p) return 0;
	lstrcpyW(p, src);
	return p;
}

//__inline TCHAR * mir_tstrdup(const TCHAR * src)
//{
//	TCHAR * p;
//	if (src==NULL) return NULL;
//    p= mir_alloc((lstrlen(src)+1)*sizeof(TCHAR));
//    if (!p) return 0;
//	lstrcpy(p, src);
//	return p;
//}
//

__inline char * mir_strdup(const char * src)
{
	char * p;
	if (src==NULL) return NULL;
    p= mir_alloc( strlen(src)+1 );
    if (!p) return 0;
	strcpy(p, src);
	return p;
}


TCHAR *DBGetStringT(HANDLE hContact,const char *szModule,const char *szSetting)
{
	TCHAR *str=NULL;
    DBVARIANT dbv={0};
	if (!DBGetContactSettingTString(hContact,szModule,szSetting,&dbv))
		str=mir_tstrdup(dbv.ptszVal);		
	DBFreeVariant(&dbv);
	return str;
}
char *DBGetStringA(HANDLE hContact,const char *szModule,const char *szSetting)
{
	char *str=NULL;
    DBVARIANT dbv={0};
	DBGetContactSetting(hContact,szModule,szSetting,&dbv);
	if(dbv.type==DBVT_ASCIIZ)
    {
        str=mir_strdup(dbv.pszVal);
        //mir_free(dbv.pszVal);
    }
    DBFreeVariant(&dbv);
	return str;
}
wchar_t *DBGetStringW(HANDLE hContact,const char *szModule,const char *szSetting)
{
	wchar_t *str=NULL;
	DBVARIANT dbv={0};
	DBGetContactSetting(hContact,szModule,szSetting,&dbv);
	if(dbv.type==DBVT_WCHAR)
	{
		str=mir_strdupW(dbv.pwszVal);
		//mir_free(dbv.pwszVal);
	}
	//else  TODO if no unicode string (only ansi)
	//
	DBFreeVariant(&dbv);
	return str;
}

DWORD exceptFunction(LPEXCEPTION_POINTERS EP) 
{ 
    //printf("1 ");                     // printed first 
	char buf[4096];
	
	
	sprintf(buf,"\r\nExceptCode: %x\r\nExceptFlags: %x\r\nExceptAddress: %x\r\n",
		EP->ExceptionRecord->ExceptionCode,
		EP->ExceptionRecord->ExceptionFlags,
		EP->ExceptionRecord->ExceptionAddress
		);
	TRACE(buf);
	MessageBoxA(0,buf,"clist_mw Exception",0);

    
	return EXCEPTION_EXECUTE_HANDLER; 
} 

#ifdef _DEBUG
#undef DeleteObject
#endif 

void TRACE_ERROR()
{
		DWORD t = GetLastError();
		LPVOID lpMsgBuf;
		if (!FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			t,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL ))
		{
		// Handle the error.
		return ;
		}
#ifdef _DEBUG
		MessageBox( NULL, (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );		
		DebugBreak();
#endif
		LocalFree( lpMsgBuf );

}

BOOL DebugDeleteObject(HGDIOBJ a)
{
	BOOL res=DeleteObject(a);
	if (!res) 
	{
		DWORD t = GetLastError();
		LPVOID lpMsgBuf;
		if (!FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			t,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL ))
		{
		// Handle the error.
		return res;
		}

		MessageBox( NULL, (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );
		LocalFree( lpMsgBuf );

	}
	return res;
}

BOOL mod_DeleteDC(HDC hdc)
{
  ResetEffect(hdc);
  return DeleteDC(hdc);
}
#ifdef _DEBUG
#define DeleteObject(a) DebugDeleteObject(a)
#endif 

