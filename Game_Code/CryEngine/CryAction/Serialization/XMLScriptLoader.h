// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __XMLSCRIPTLOADER_H__
#define __XMLSCRIPTLOADER_H__

#pragma once

SmartScriptTable XmlScriptLoad( const char * definitionFile, const char * dataFile );
SmartScriptTable XmlScriptLoad( const char * definitionFile, XmlNodeRef data );
XmlNodeRef XmlScriptSave( const char * definitionFile, SmartScriptTable scriptTable );
bool XmlScriptSave( const char * definitionFile, const char * dataFile, SmartScriptTable scriptTable );

#endif
