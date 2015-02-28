#include "StdAfx.h"
#include "CryHash.h"

CryHashStringId CryHashStringId::GetIdForName( const char* _name )
{
	CRY_ASSERT(_name);

	return CryHashStringId(_name);
}
