/***********************************************************************************************************************
*
* Copyright (c) 2022 by Tech Soft 3D, LLC.
* The information contained herein is confidential and proprietary to Tech Soft 3D, LLC., and considered a trade secret
* as defined under civil and criminal statutes. Tech Soft 3D shall pursue its civil and criminal remedies in the event
* of unauthorized use or misappropriation of its trade secrets. Use of this information by anyone other than authorized
* employees of Tech Soft 3D, LLC. is granted only under a written non-disclosure agreement, expressly prescribing the
* scope and manner of such use.
*
***********************************************************************************************************************/
/**
\file common.hpp

Definitions of macros, data and functions common to Exchange samples.

This header requires prior inclusion of <A3DSDKIncludes.h>.

**/

#ifndef EXCHANGE_SAMPLES_COMMON_HPP
#define EXCHANGE_SAMPLES_COMMON_HPP

#ifdef _MSC_VER
#	pragma warning(disable: 4505) // "unreferenced local function has been removed"
#	pragma warning(disable: 4996) // "this function or variable may be unsafe"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#	include <unordered_map>

//######################################################################################################################
#define CHECK_RET(FunctionCall)\
{\
	const A3DStatus iRet__ = FunctionCall;\
	if (iRet__ != A3D_SUCCESS)\
	{\
		if (A3DMiscGetErrorMsg)\
			fprintf(stderr, #FunctionCall " returned error %d = %s\n", iRet__ , A3DMiscGetErrorMsg(iRet__) ); \
		else\
			fprintf(stderr, #FunctionCall " returned error %d\n", iRet__ ); \
		return iRet__;\
	}\
}

//######################################################################################################################
#define TEST_RET(FunctionCall)\
{\
	const A3DStatus iRet__ = FunctionCall;\
	if (iRet__ != A3D_SUCCESS)\
	{\
		if (A3DMiscGetErrorMsg)\
			fprintf(stderr, #FunctionCall " returned error %d = %s\n", iRet__ , A3DMiscGetErrorMsg(iRet__) ); \
		else\
			fprintf(stderr, #FunctionCall " returned error %d\n", iRet__ ); \
	}\
}

//######################################################################################################################
#ifdef _MSC_VER
#	define MY_CHAR A3DUniChar
#	define MY_MAIN wmain
#else
#	define MY_CHAR A3DUTF8Char
#	define MY_MAIN main
#	define _T(_x) _x
#endif

//######################################################################################################################
#if defined _UNICODE || defined UNICODE
#	define MY_STRCMP(X,Y) wcscmp(X,_T(Y))
#	define MY_STRICMP(X,Y) _wcsicmp(X,_T(Y))
#	define MY_STRLEN wcslen
#	define MY_ATOI _wtoi
#	define MY_ATOF _wtof
#	define MY_FOPEN(X,Y) _wfopen(X,_T(Y))
#	define MY_STRCPY(X,Y) wcscpy(X,Y)
#	define MY_STRCAT(X,Y) wcscat(X,Y)
#	define MY_STRCAT2 wcscat
#	define MY_FPRINTF(X,Y,Z) fwprintf(X,_T(Y),Z)
#	define MY_PRINTF(X) wprintf(_T(X))
#	define MY_PRINTF2(X,Y) wprintf(_T(X),Y)
#	define MY_SPRINTF(X,Y,Z) wsprintf(X,_T(Y),Z)
#	define MY_SPRINTF2(W,X,Y,Z) wsprintf(W,_T(X),Y,Z)
#	define MY_SSCANF(X,Y,Z) swscanf(X,_T(Y),Z)
#else
#	define MY_STRCMP strcmp
#	define MY_STRICMP stricmp
#	define MY_STRLEN strlen
#	define MY_ATOI atoi
#	define MY_ATOF atof
#	define MY_FOPEN fopen
#	define MY_STRCPY strcpy
#	define MY_STRCAT strcat
#	define MY_STRCAT2 strcat
#	define MY_FPRINTF fprintf
#	define MY_PRINTF(X) printf(X)
#	define MY_PRINTF2(X,Y) printf(X,Y)
#	define MY_SPRINTF(X,Y,Z) sprintf(X,Y,Z)
#	define MY_SPRINTF2(W,X,Y,Z) sprintf(W,X,Y,Z)
#	define MY_SSCANF sscanf
#endif

#define PRINTLOGMESSAGEVALUE(format, value) fprintf(GetLogFile(), format, value);

//######################################################################################################################
#ifdef _MSC_VER
#	define DEFAULT_INPUT_CAD     _T(SAMPLES_DATA_DIRECTORY"\\catiaV5\\CV5_Aquo_Bottle\\_Aquo Bottle.CATProduct")
#	define DEFAULT_INPUT_DRAWING _T(SAMPLES_DATA_DIRECTORY"\\drawing\\Carter.CATDrawing")
#else
#	define DEFAULT_INPUT_CAD     SAMPLES_DATA_DIRECTORY"/catiaV5/CV5_Aquo_Bottle/_Aquo Bottle.CATProduct"
#	define DEFAULT_INPUT_DRAWING SAMPLES_DATA_DIRECTORY"/drawing/Carter.CATDrawing"
#endif

//######################################################################################################################
// single point of access to the static log file pointer from all translation units; avoids to add a .cpp file or face
// multiple definitions error at link time
// call with a non-null file name to create the log file
// default return value is stdout
inline FILE* GetLogFile(const MY_CHAR* pFileName = NULL)
{
	// fflush does not work correctly on stdout and stderr if redirected to a file with freopen, so I need a dedicated FILE
	static FILE* pLogFile = NULL;
	if (pFileName)
	{
		if (pLogFile)
		{
			fclose(pLogFile);
			pLogFile = NULL;
		}
		pLogFile = MY_FOPEN(pFileName, "w");
	}
	return pLogFile ? pLogFile : stdout;
}

//######################################################################################################################
inline A3DInt32 PrintLogMessage(A3DUTF8Char* pMsg)
{
	return fprintf(GetLogFile(), "%s", pMsg ? pMsg : "");
}

//######################################################################################################################
inline A3DInt32 PrintLogWarning(A3DUTF8Char* pKod, A3DUTF8Char* pMsg)
{
	return fprintf(GetLogFile(), "WAR %s - %s", pKod ? pKod : "", pMsg ? pMsg : "");
}

//######################################################################################################################
inline A3DInt32 PrintLogError(A3DUTF8Char* pKod, A3DUTF8Char* pMsg)
{
	FILE* pLogFile = GetLogFile();
	if (pLogFile == stdout)
		pLogFile = stderr;
	fprintf(pLogFile, "ERR %s - %s", pKod ? pKod : "", pMsg ? pMsg : "");
	return fflush(pLogFile);
}

//######################################################################################################################
// These functions exist just to show that you can do your own memory management.



static std::unordered_map<void*, A3DInt32 > stvalloc;

inline A3DPtr CheckMalloc(size_t uiByteSize)
{
	static A3DInt32 siCheckMalloc = 0;
	if (!uiByteSize)
		return nullptr;
	siCheckMalloc++;


	A3DPtr Ptr = calloc(1, uiByteSize);

	stvalloc[Ptr] = siCheckMalloc;

	return Ptr;
}

inline A3DVoid CheckFree(A3DPtr ptr)
{
	if (!ptr)
		return;

	auto it = stvalloc.find(ptr);
	if (it == stvalloc.end())
	{
		printf("Memory error on free!\n");
	}
	else
	{
		stvalloc.erase(it);
	}
	free(ptr);

}

inline size_t ListLeaks()
{
	for (const auto& key_value : stvalloc)
	{
		printf("Leak at alloc %d\n", key_value.second);
	}
	return stvalloc.size();
}

template<typename T>
T* CheckMallocT(const size_t uiObjectCount)
{
	return static_cast<T*>(CheckMalloc(uiObjectCount * sizeof(T)));
}

//######################################################################################################################
// Automatically free memory allocated with malloc and calloc.
class MemoryGuard
{
public:
	explicit MemoryGuard(void* ptr) : m_ptr(ptr) {}
	~MemoryGuard()
	{
		free(m_ptr);
		m_ptr = NULL;
	}

private:
	void* m_ptr;

	// non-copyable class: prevent use of copy constructor and operator.
	// (in C++11 and later it would be better to disable them with "= delete")
	MemoryGuard(const MemoryGuard& other);
	MemoryGuard& operator=(const MemoryGuard& other);
};

//######################################################################################################################
// Automatically free A3D data.
template<typename T, typename D>
class DataGuard
{
public:
	typedef A3DStatus(*FunctionType)(const T*, D*);

	DataGuard(D& data, FunctionType& function) :
		data_(data),
		function_(function)
	{}

	~DataGuard()
	{
		TEST_RET(function_(NULL, &data_))
	}

private:
	D&                 data_;
	const FunctionType function_;

	// non-copyable class: prevent use of copy constructor and operator.
	// (in C++11 and later it would be better to disable them with "= delete")
	DataGuard(const DataGuard& other);
	DataGuard& operator=(const DataGuard& other);
};

#define DATAGUARD(Type) DataGuard<Type, Type##Data>

//######################################################################################################################
#endif // EXCHANGE_SAMPLES_COMMON_HPP
