// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include <stdio.h>
#include "LuaCryPakIO.h"

// needed for crypak
#include <ISystem.h>
#include <ICryPak.h>

//////////////////////////////////////////////////////////////////////////
FILE	*CryPakOpen(const char *szFile,const char *szMode)
{
#ifdef USE_CRYPAK
	ICryPak *pPak=gEnv->pCryPak;
	return pPak->FOpen(szFile,szMode);
#else
	return (fopen(szFile,szMode));
#endif
}

//////////////////////////////////////////////////////////////////////////
int CryPakClose(FILE *fp)
{
#ifdef USE_CRYPAK
	ICryPak *pPak=gEnv->pCryPak;
	return pPak->FClose(fp);
#else
	return fclose(fp);
#endif
}

//////////////////////////////////////////////////////////////////////////
int CryPakFFlush(FILE *fp)
{
#ifdef USE_CRYPAK
  ICryPak *pPak=gEnv->pCryPak;
  return pPak->FFlush(fp);
#else
  return fflush(fp);
#endif
}

//////////////////////////////////////////////////////////////////////////
char *CryPakFGets(char *str, int n, FILE *handle)
{
#ifdef USE_CRYPAK
  ICryPak *pPak=gEnv->pCryPak;
  return pPak->FGets(str, n, handle);
#else
  return fgets(str, n, handle);
#endif
}

//////////////////////////////////////////////////////////////////////////
int   CryPakUngetc(int c, FILE *handle)
{
#ifdef USE_CRYPAK
  ICryPak *pPak=gEnv->pCryPak;
  return pPak->Ungetc(c, handle);
#else
	return(ungetc(c,handle));
#endif
}

//////////////////////////////////////////////////////////////////////////
int   CryPakGetc(FILE *handle)
{
#ifdef USE_CRYPAK
  ICryPak *pPak=gEnv->pCryPak;
  return pPak->Getc(handle);
#else
	return(getc(handle));
#endif
}

//////////////////////////////////////////////////////////////////////////
int CryPakFSeek(FILE *handle, long seek, int mode)
{
#ifdef USE_CRYPAK
  ICryPak *pPak=gEnv->pCryPak;
  return pPak->FSeek(handle, seek, mode);
#else
  return fseek(handle, seek, mode);
#endif
}


//////////////////////////////////////////////////////////////////////////
size_t CryPakFRead(void *data, size_t length, size_t elems, FILE *handle)
{
#ifdef USE_CRYPAK
  ICryPak *pPak=gEnv->pCryPak;
  return pPak->FRead(data, length, elems, handle);
#else
	return fread(data, length, elems, handle);
#endif
}

//////////////////////////////////////////////////////////////////////////
size_t CryPakFWrite(void *data, size_t length, size_t elems, FILE *handle)
{
#ifdef USE_CRYPAK
  ICryPak *pPak=gEnv->pCryPak;
  return pPak->FWrite(data, length, elems, handle);
#else
  return fwrite(data, length, elems, handle);
#endif
}

//////////////////////////////////////////////////////////////////////////
int   CryPakFEof(FILE *handle)
{
#ifdef USE_CRYPAK
  ICryPak *pPak=gEnv->pCryPak;
  return pPak->FEof(handle);
#else
	return (feof(handle));
#endif
}
