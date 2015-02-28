// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "XmlSaveGame.h"
#include "XmlSerializeHelper.h"

#include <IPlatformOS.h>

//#define XML_SAVEGAME_USE_COMPRESSION		// must be in sync with XmlLoadGame.cpp
//#undef  XML_SAVEGAME_USE_COMPRESSION   // undef because does NOT work with aliases yet

static const int XML_SAVEGAME_MAX_CHUNK_SIZE = 32767/2;

struct CXmlSaveGame::Impl
{
	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->Add(*this);
		pSizer->AddObject(sections);
		pSizer->AddObject(root);
		pSizer->AddObject(metadata);
		pSizer->AddObject(outputFile);
	}

	XmlNodeRef root;
	XmlNodeRef metadata;
	string outputFile;

	typedef std::vector<_smart_ptr<CXmlSerializeHelper> > TContexts;
	TContexts sections;
};

CXmlSaveGame::CXmlSaveGame() : m_pImpl(new Impl), m_eReason(eSGR_QuickSave)
{
}

CXmlSaveGame::~CXmlSaveGame()
{
}

bool CXmlSaveGame::Init( const char * name )
{
	m_pImpl->outputFile = name;
	m_pImpl->root = GetISystem()->CreateXmlNode("SaveGame", true);
	m_pImpl->metadata = m_pImpl->root->createNode("Metadata");
	m_pImpl->root->addChild(m_pImpl->metadata);
	return true;
}


XmlNodeRef CXmlSaveGame::GetMetadataXmlNode() const
{
	return m_pImpl->metadata;
}

void CXmlSaveGame::AddMetadata( const char * tag, const char * value )
{
	m_pImpl->metadata->setAttr(tag, value);
}

void CXmlSaveGame::AddMetadata( const char * tag, int value )
{
	m_pImpl->metadata->setAttr(tag, value);
}

TSerialize CXmlSaveGame::AddSection( const char * section )
{
	XmlNodeRef node = m_pImpl->root->createNode(section);
	m_pImpl->root->addChild(node);

	_smart_ptr<CXmlSerializeHelper> pSerializer = new CXmlSerializeHelper;
	m_pImpl->sections.push_back(pSerializer);

	return TSerialize(pSerializer->GetWriter(node));
}

uint8* CXmlSaveGame::SetThumbnail(const uint8* imageData, int width, int height, int depth)
{
	return 0;
}

bool CXmlSaveGame::SetThumbnailFromBMP(const char *filename)
{
	return false;
}

bool CXmlSaveGame::Complete( bool successfulSoFar )
{
	if (successfulSoFar)
	{
		successfulSoFar &= Write( m_pImpl->outputFile.c_str(), m_pImpl->root );
	}
	delete this;
	return successfulSoFar;
}

const char *CXmlSaveGame::GetFileName() const
{
	if(m_pImpl.get())
		return m_pImpl.get()->outputFile;
	return NULL;
}

void CXmlSaveGame::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(m_pImpl);
}

bool CXmlSaveGame::Write( const char * filename, XmlNodeRef data )
{
	#ifdef WIN32
		CrySetFileAttributes( filename,0x00000080 ); // FILE_ATTRIBUTE_NORMAL
	#endif //WIN32

	// Try opening file for creation first, will also create directory.
	FILE *pFile = NULL;
	
	if (!GetISystem()->GetPlatformOS()->UsePlatformSavingAPI())
	{
		pFile = gEnv->pCryPak->FOpen(filename,"wb");
		if (!pFile)
		{
			CryWarning( VALIDATOR_MODULE_GAME,VALIDATOR_WARNING,"Failed to create file %s",filename );
			return false;
		}
	}

	#ifdef XML_SAVEGAME_USE_COMPRESSION
		#error Savegame compression not supported with IPlatformOS
		CRY_ASSERT("SAVEGAME compression not supported because it's allocating 6MB chunks of memory.");
		#if defined(XENON) || defined(PS3)
				CryWarning(VALIDATOR_MODULE_GAME,VALIDATOR_ERROR, "CXmlSaveGame::Write not supported yet, strings cannot grow beyond 32767 chars\n");
				return true;//otherwise stopping
		#endif
		gEnv->pCryPak->FClose(pFile);
		_smart_ptr<IXmlStringData> pXmlStrData = data->getXMLData( 6000000 );
		return GetISystem()->WriteCompressedFile( filename,(void*)pXmlStrData->GetString(),8*pXmlStrData->GetStringLength() ); // Length is in bits (so mult by 8)
	#else
		const bool bSavedToFile = data->saveToFile(filename, XML_SAVEGAME_MAX_CHUNK_SIZE, pFile);
		if (pFile)
		{
			gEnv->pCryPak->FClose(pFile);
		}
		return bSavedToFile;
	#endif
}
