////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   PlayerProfileImplDurango.cpp
//  Created:     17/05/2013 by Yeonwoon JUNG.
//  Description: Created based on PlayerProfileImplConsole:
//
//               Player profile implementation for consoles which manage
//               the profile data via the OS and not via a file system 
//               which may not be present.
//
//               Problem with Durango is that we need to split attributes
//               into multiple categories, and each one has to have diff-
//               erent container.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "PlayerProfileImplDurango.h"

#if defined(DURANGO)

extern "C" {
#include "PlayerProfiles/jsmn/jsmn.h"
}

#include <ICryOnlineStorage.h>
#include <IPlatformOS.h>
//#include <ISmartGlass.h>
#include <IXml.h>
#include <StringUtils.h>
#include <TypeInfo_impl.h>

#include "CryActionCVars.h"

#include "PlayerProfile.h"
#include "Serialization/XmlSaveGame.h"
#include "Serialization/XmlLoadGame.h"

#include "Serialization/SerializeWriterXMLCPBin.h"
#include "Serialization/SerializeReaderXMLCPBin.h"
#include "serialization/XMLCPBin/XMLCPB_Utils.h"

using namespace PlayerProfileImpl;

#define PLAYERPROFILE_CONTAINER_NAME			L"PlayerProfile"
#define PLAYERPROFILE_ATTR_CONTAINER_NAME	L"PlayerProfileAttributes"

#ifndef RELEASE
#define DEBUG_PROFILE_IMPL_ALLOW_TRACE
#endif


namespace // anonymous
{
	const size_t kMaxProfileDataBlockSize = 32 * 1024;

	typedef std::vector<uint8>						TDataBlockBuffer;
	typedef std::vector<TDataBlockBuffer>	TDataBlockBufferPool;

	//------------------------------------------------------------------------
	void StringToDataBlockBuffer(TDataBlockBuffer& dataBlockBuffer, const string& xmlString)
	{
		size_t dataLength = xmlString.length();
		assert(dataLength < kMaxProfileDataBlockSize);

		dataBlockBuffer.resize(dataLength);
		memcpy_s(&dataBlockBuffer[0], dataBlockBuffer.size(), xmlString.c_str(), dataLength);
	}

	//------------------------------------------------------------------------
	// IOnlineStorageListener adapter class for functor
	class COnlineStorageQueryListenerAdapter : public IOnlineStorageListener
	{
		typedef std::function<void(const CryOnlineStorageQueryData&)> TCallback;

	public:
		COnlineStorageQueryListenerAdapter(TCallback fun) : m_callback(fun)
		{}

	private:
		VIRTUAL void OnOnlineStorageOperationComplete(const CryOnlineStorageQueryData& queryData)
		{
			m_callback(queryData);
			delete this;
		}
		TCallback m_callback;
	};

	//------------------------------------------------------------------------
	// encode or decode JSON stuff
	template <typename Fun1, typename Fun2>
	void EncodeProfileAttributesToJSON(XmlNodeRef attributes, Fun1 funNormalAttr, Fun2 funBlobJSON)
	{
		// assumption: children are alphabetically ordered by 'name' attribute (see CPlayerProfile::TAttributeMap)
		stack_string outputJson;

		stack_string blockPath;
		stack_string part;

		typedef std::vector<stack_string> TPathPartStack;
		TPathPartStack partStack;

		size_t maxDepthsEstimated = 8;
		partStack.reserve(maxDepthsEstimated);

		auto requestEndingBrackets = [](int num) -> const char* {
			static char endingBrackets[] = "}}}}}}}}}}}}}}}}}}}}"; // 20 brackets
			assert(num < _countof(endingBrackets));
			return &endingBrackets[_countof(endingBrackets)-1-num];
		};

		int numChildren = attributes->getChildCount();
		for (int i = 0; i < numChildren; ++i)
		{
			XmlNodeRef attr = attributes->getChild(i);
			assert(attr->isTag("Attr"));

			blockPath = attr->getAttr("name");

			if (blockPath.find('/') != stack_string::npos)
			{
				int curDepth = 0; // index
				int maxDepth = (int)std::count(blockPath.begin(), blockPath.end(), '/');

				int pos = 0;
				while (!(part = blockPath.Tokenize("/", pos)).empty())
				{
					bool newSibling = partStack.size() > curDepth && (partStack[curDepth].compareNoCase(part) != 0);

					if (newSibling && curDepth == 0)
					{
						outputJson.append(requestEndingBrackets(partStack.size()));

						funBlobJSON(partStack[0], outputJson);

						partStack.clear();
						outputJson.clear();
						newSibling = false;
					}

					if (partStack.size() == curDepth || newSibling)
					{
						bool openBrackets = partStack.size() > curDepth;
						if (openBrackets)
						{
							int numBracketsToClose = partStack.size() - curDepth - 1;
							outputJson.append(requestEndingBrackets(numBracketsToClose));

							// clear out stack items used in previous node
							partStack.erase(partStack.begin() + curDepth, partStack.end());
						}

						// adding key
						outputJson.append(newSibling ? ",\"" : "{\"");
						outputJson.append(part);
						outputJson.append("\":");

						// adding value if node is leaf
						if (curDepth == maxDepth)
						{
							outputJson.append("\"");
							outputJson.append(attr->getAttr("value"));
							outputJson.append("\"");
						}

						partStack.push_back(part);
					}

					curDepth ++;
				}
			}
			else
			{
				funNormalAttr(attr);
			}
		}

		if (!partStack.empty())
		{
			outputJson.append(requestEndingBrackets(partStack.size()));
			funBlobJSON(partStack[0], outputJson);
		}
	}

	//------------------------------------------------------------------------
	template <typename Fun>
	int navigateObject(const char* blobJSON, jsmntok_t* pTokens, int index, stack_string& path, Fun fun)
	{
		int processed = 0;
		size_t prevPathLen = path.length();

		assert(pTokens[index].type == JSMN_OBJECT);
		for (int i = 0; i < pTokens[index].size; ++i)
		{
			int next = index + processed + 1;

			assert(pTokens[next].type != JSMN_ARRAY);
			if (pTokens[next].type == JSMN_OBJECT)
			{
				processed += navigateObject(blobJSON, pTokens, next, path, fun);
				path.erase(prevPathLen);
			}
			else
			{
				const char* valuePtr = blobJSON + pTokens[next].start;
				size_t valueLen = pTokens[next].end - pTokens[next].start;

				bool foundAttr = (i & 0x1);
				if (foundAttr)
				{
					stack_string value(valuePtr,valueLen);
					fun(path, value);
					path.erase(path.rfind('/'));
				}
				else
				{
					if (path.empty())
					{
						path.assign(valuePtr, valueLen);
					}
					else
					{
						path.append("/");
						path.append(valuePtr, valueLen);
					}
				}
			}

			processed ++;
		}

		return processed;
	}

	//------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable:6262) // Function uses '33340' bytes of stack
	template <typename Fun>
	bool DecodeProfileAttributesFromJSON(const char* blobJSON, Fun fun)
	{
		jsmn_parser js;
		jsmn_init(&js);

		const int numTokensExpected = 4096;
		jsmntok_t tokens[numTokensExpected]; // 64K (increased to handle large numbers of progression stats)

		jsmnerr_t result = jsmn_parse(&js, blobJSON, tokens, numTokensExpected);
		if (result == JSMN_SUCCESS)
		{
			if (tokens[0].type == JSMN_OBJECT)
			{
				stack_string path("");
				navigateObject(blobJSON, tokens, 0, path, fun);
			}
			return true;
		}

		return false;
	}
}
#pragma warning(pop)

const wchar_t* CPlayerProfileImplDurango::ms_profileBlockNames[] = { L"Profile", L"ActionMaps", L"Attributes" };

//------------------------------------------------------------------------
bool CPlayerProfileImplDurango::Initialize(CPlayerProfileManager* pMgr)
{
	if (CPlayerProfileImplConsole::Initialize(pMgr) == false)
	{
		return false;
	}

	m_defaultSaveLocation = eEST_ConnectedStorage;
	return true;
}

//------------------------------------------------------------------------
void CPlayerProfileImplDurango::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CPlayerProfileImplDurango::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
}

//------------------------------------------------------------------------
bool CPlayerProfileImplDurango::SavePlayerProfile(XmlNodeRef rootNode, XmlNodeRef actionMap, XmlNodeRef attributes)
{
	XmlNodeRef attributesClone = attributes->clone();
	// save attributes have multiple path; remove items have written.
	if (SavePlayerProfileAttributes(attributesClone))
	{
		TDataBlockBuffer writeBuffer[ePB_Count];

		IPlatformOS::TContainerDataBlocks dataBlocks;
		dataBlocks.resize(ePB_Count);

		// save attributes' own data except children data; but this also has list of children to be able to load them.
		StringToDataBlockBuffer(writeBuffer[ePB_Attributes], attributesClone->getXML());

		dataBlocks[ePB_Attributes].blockName = ms_profileBlockNames[ePB_Attributes];
		dataBlocks[ePB_Attributes].pDataBlock = &writeBuffer[ePB_Attributes][0];
		dataBlocks[ePB_Attributes].dataBlockSize = writeBuffer[ePB_Attributes].size();
	
		// save action-map
		StringToDataBlockBuffer(writeBuffer[ePB_ActionMaps], actionMap->getXML());

		dataBlocks[ePB_ActionMaps].blockName = ms_profileBlockNames[ePB_ActionMaps];
		dataBlocks[ePB_ActionMaps].pDataBlock = &writeBuffer[ePB_ActionMaps][0];
		dataBlocks[ePB_ActionMaps].dataBlockSize = writeBuffer[ePB_ActionMaps].size();

		// save profile info itself
		rootNode->removeAllChilds();
		StringToDataBlockBuffer(writeBuffer[ePB_Profile], rootNode->getXML());

		dataBlocks[ePB_Profile].blockName = ms_profileBlockNames[ePB_Profile];
		dataBlocks[ePB_Profile].pDataBlock = &writeBuffer[ePB_Profile][0];
		dataBlocks[ePB_Profile].dataBlockSize = writeBuffer[ePB_Profile].size();

		return GetISystem()->GetPlatformOS()->SaveIntoStorage(PLAYERPROFILE_CONTAINER_NAME, dataBlocks);
	}

	return false;
}

//------------------------------------------------------------------------
bool CPlayerProfileImplDurango::SavePlayerProfileAttributes(XmlNodeRef& attributes)
{
	XmlNodeRef attrSkeleton = gEnv->pSystem->CreateXmlNode(attributes->getTag());
	attrSkeleton->copyAttributes(attributes);

	// buffers for connected storage
	int numExpectedBlocks = 16;
	int currentBlock = 0;

	IPlatformOS::TContainerDataBlocks dataBlocks;
	dataBlocks.resize(numExpectedBlocks);

	TDataBlockBufferPool writeBufferPool;
	writeBufferPool.resize(numExpectedBlocks);

	std::vector<wstring> blockNameBuffers;
	blockNameBuffers.resize(numExpectedBlocks);

	// start converting profile data into JSON
	EncodeProfileAttributesToJSON(attributes, 

		// for entities have no sub path; saves along with root xml node (not JSON)
		[attrSkeleton](XmlNodeRef attr) {

			attrSkeleton->addChild(attr);
	},

		// this function is called when block of attributes are made.
		[&](const string& blockName, const string& blobJSON) {

			attrSkeleton->newChild(blockName.c_str());

			// connected storage
			assert(currentBlock < numExpectedBlocks);
			if (currentBlock >= numExpectedBlocks)
			{
				GameWarning("SavePlayerProfileAttributes: reserved buffers are not sufficient; realloc memory");

				const int numChunksToGrow = 4;
				numExpectedBlocks += numChunksToGrow;
				dataBlocks.resize(numExpectedBlocks);
				writeBufferPool.resize(numExpectedBlocks);
				blockNameBuffers.resize(numExpectedBlocks);
			}
			CryStringUtils::UTF8ToWStr(blockName, blockNameBuffers[currentBlock]);

			uint storageTypeFlag = stl::find_in_map(m_attributeBlockStorageInfos, blockName, SAttributeBlockStorageInfo(m_defaultSaveLocation)).storageTypeFlag;

			if (storageTypeFlag & eEST_ConnectedStorage)
			{
				StringToDataBlockBuffer(writeBufferPool[currentBlock], blobJSON);

				IPlatformOS::SContainerDataBlock& dataBlock = dataBlocks[currentBlock];
				dataBlock.blockName = blockNameBuffers[currentBlock].c_str();
				dataBlock.pDataBlock = &writeBufferPool[currentBlock][0];
				dataBlock.dataBlockSize = writeBufferPool[currentBlock].size();
				currentBlock ++;
			}
			
			if (storageTypeFlag & eEST_TitleStorage)
			{
				UploadProfileBlockToTMS(blockName, blobJSON);
			}
			
		/*	if (storageTypeFlag & eEST_SmartGlass)
			{
				ISmartGlass* pSmartGlass = gEnv->pSystem->GetISmartGlass();
				if (pSmartGlass)
				{
					pSmartGlass->SendSmartGlassCommandValue("ProfileUpdated", blobJSON);
				}
			}*/
	});

	// swap to drop attributes already have been written.
	attributes = attrSkeleton;

	if (currentBlock > 0)
	{
		dataBlocks.resize(currentBlock);
		return GetISystem()->GetPlatformOS()->SaveIntoStorage(PLAYERPROFILE_ATTR_CONTAINER_NAME, dataBlocks);
	}

	return true;
}

//------------------------------------------------------------------------
bool CPlayerProfileImplDurango::SaveProfile(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* name, bool initialSave /*= false*/, int reason /*= ePR_All*/)
{
	IPlatformOS *pOS = gEnv->pSystem->GetPlatformOS();

	// check if it's a valid filename
	if (IsValidFilename(name) == false)
		return false;

	if (pEntry)
	{
		// !initialSave condition is needed here to avoid overwrite profile with default value at start-up
		if (!pEntry->unknownUser && !initialSave && pOS->UsePlatformSavingAPI())
		{
			if (reason & (ePR_Game|ePR_Options))
			{
				XmlNodeRef rootNode = GetISystem()->CreateXmlNode(PROFILE_ROOT_TAG);
				rootNode->setAttr(PROFILE_NAME_TAG, name);
				CSerializerXML serializer(rootNode, false);
				pProfile->SerializeXML(&serializer);

				XmlNodeRef attributes = serializer.GetSection(CPlayerProfileManager::ePPS_Attribute);
				XmlNodeRef actionMap = serializer.GetSection(CPlayerProfileManager::ePPS_Actionmap);

				return SavePlayerProfile(rootNode, actionMap, attributes);
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------
XmlNodeRef CPlayerProfileImplDurango::LoadPlayerProfile()
{
	TDataBlockBuffer readBuffers[ePB_Count];

	IPlatformOS::TContainerDataBlocks dataBlocks;
	dataBlocks.resize(ePB_Count);

	for (int i = 0; i < ePB_Count; ++i)
	{
		dataBlocks[i].blockName = ms_profileBlockNames[i];

		readBuffers[i].resize(kMaxProfileDataBlockSize);
		dataBlocks[i].pDataBlock = &readBuffers[i][0];
		dataBlocks[i].dataBlockSize = kMaxProfileDataBlockSize;
	}

	XmlNodeRef profile;

	if (GetISystem()->GetPlatformOS()->LoadFromStorage(IPlatformOS::eCST_ERA, PLAYERPROFILE_CONTAINER_NAME, dataBlocks))
	{
		// Check we can load all the blocks we requested; if not, we failed to properly load
		bool allOk = true;
		for (int i = 0; i < ePB_Count; ++i)
		{
			if(dataBlocks[i].errorCode != IPlatformOS::SContainerDataBlock::eEC_Ok)
			{
				allOk = false;
				break;
			}
		}

		if(allOk)
		{
			profile = GetISystem()->LoadXmlFromBuffer((const char*)dataBlocks[ePB_Profile].pDataBlock, dataBlocks[ePB_Profile].dataBlockSize);

			XmlNodeRef actionMaps = GetISystem()->LoadXmlFromBuffer((const char*)dataBlocks[ePB_ActionMaps].pDataBlock, dataBlocks[ePB_ActionMaps].dataBlockSize);
			XmlNodeRef attributesSkeleton = GetISystem()->LoadXmlFromBuffer((const char*)dataBlocks[ePB_Attributes].pDataBlock, dataBlocks[ePB_Attributes].dataBlockSize);

			XmlNodeRef attributes = LoadPlayerProfileAttributes(attributesSkeleton);

			if (actionMaps)
			{
				profile->addChild(actionMaps);
			}

			if (attributes)
			{
				profile->addChild(attributes);
			}
		}
	}
	else
	{
		GameWarning("CPlayerProfileImplDurango::LoadPlayerProfile: failed to load 'Profile'");
	}

	return profile;
}

//------------------------------------------------------------------------
XmlNodeRef CPlayerProfileImplDurango::LoadPlayerProfileAttributes(XmlNodeRef attributesSkeleton)
{
	XmlNodeRef attributes = attributesSkeleton->clone();

	// sub nodes which have 'value' attribute are root attributes of attributes node.
	// as we only need to load sub attributes, it needs to be filtered out.
	size_t numSubNodes = 0;
	for (int i = 0; i < attributesSkeleton->getChildCount(); ++i)
	{
		XmlNodeRef childNode = attributesSkeleton->getChild(i);
		if (!childNode->haveAttr("value"))
		{
			numSubNodes ++;
		}
	}

	if (numSubNodes)
	{
		IPlatformOS::TContainerDataBlocks dataBlocks;
		dataBlocks.resize(numSubNodes);

		TDataBlockBufferPool readBufferPool;
		readBufferPool.resize(numSubNodes);

		std::vector<wstring> blockNameBuffers;
		blockNameBuffers.resize(numSubNodes);

		int subIndex = 0;
		for (int i = 0; i < attributesSkeleton->getChildCount(); ++i)
		{
			XmlNodeRef childNode = attributesSkeleton->getChild(i);
			if (!childNode->haveAttr("value"))
			{
				const char* tagName = childNode->getTag();

				// remove skeleton node, new node with data will be added again after loading.
				attributes->removeChild(attributes->findChild(tagName));

				CryStringUtils::UTF8ToWStr(tagName, blockNameBuffers[subIndex]);

				IPlatformOS::SContainerDataBlock& dataBlock = dataBlocks[subIndex];

				dataBlock.blockName = blockNameBuffers[subIndex].c_str();

				readBufferPool[subIndex].resize(kMaxProfileDataBlockSize);
				dataBlock.pDataBlock = &readBufferPool[subIndex][0];
				dataBlock.dataBlockSize = kMaxProfileDataBlockSize - 1; // preserve space for null termination; needed for jsmn

				subIndex ++;
			}
		}

		if (GetISystem()->GetPlatformOS()->LoadFromStorage(IPlatformOS::eCST_ERA, PLAYERPROFILE_ATTR_CONTAINER_NAME, dataBlocks))
		{
			for (auto& dataBlock : dataBlocks)
			{
				dataBlock.pDataBlock[dataBlock.dataBlockSize] = '\0';
				const char* blobJSON = (const char*)dataBlock.pDataBlock;

				bool decoded = DecodeProfileAttributesFromJSON(blobJSON, [attributes](const stack_string& path, const stack_string& value) {

					XmlNodeRef node = attributes->newChild("Attr");
					node->setAttr("name", path.c_str());
					node->setAttr("value", value.c_str());

				});

				// NB: json parser is not that strict, sometimes it returns true even though format is not really JSON.
				assert(decoded);
			}
		}
		else
		{
			GameWarning("CPlayerProfileImplDurango::LoadPlayerProfileAttributes: failed to load data from connected storage");
		}
	}

	return attributes;
}

//------------------------------------------------------------------------
bool CPlayerProfileImplDurango::LoadProfile(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* name)
{
	IPlatformOS *pOS = gEnv->pSystem->GetPlatformOS();

	// to load default profile if there is no profile in connected storage
	string path; 
	InternalMakeFSPath(NULL, CPlayerProfileManager::FACTORY_DEFAULT_NAME, path);

	XmlNodeRef rootNode = GetISystem()->CreateXmlNode(PROFILE_ROOT_TAG);
	CSerializerXML serializer(rootNode, true);

	XmlNodeRef profile;

	if (pEntry && !pEntry->unknownUser && pOS->UsePlatformSavingAPI())
	{
		profile = LoadPlayerProfile();
	}

	if (profile == NULL)
	{
		profile = LoadXMLFile(path+"profile.xml");
	}

	bool ok = false;
	if(profile)
	{
		XmlNodeRef attrNode = profile->findChild("Attributes");
		XmlNodeRef actionNode = profile->findChild("ActionMaps");

		if(!(attrNode && actionNode))
		{
			attrNode = LoadXMLFile(path+"attributes.xml");
			actionNode = LoadXMLFile(path+"actionmaps.xml");
		}

		if(attrNode && actionNode)
		{
			serializer.SetSection(CPlayerProfileManager::ePPS_Attribute, attrNode);
			serializer.SetSection(CPlayerProfileManager::ePPS_Actionmap, actionNode);

			ok = pProfile->SerializeXML(&serializer);
		}
	}

	return ok;
}

//////////////////////////////////////////////////////////////////////////
class CXMLCPBinSaveGameDurangoConnectedStorage : public CXmlSaveGame
{
	typedef CXmlSaveGame inherited;

	class CSerializeCtx : public _reference_target_t
	{
	public:
		CSerializeCtx( XMLCPB::CNodeLiveWriterRef node, XMLCPB::CWriterInterface& binWriter )
		{
			m_pWriterXMLCPBin = std::auto_ptr<CSerializeWriterXMLCPBin>( new CSerializeWriterXMLCPBin( node, binWriter  ) );
			m_pWriter = std::auto_ptr<ISerialize>(new CSimpleSerializeWithDefaults<CSerializeWriterXMLCPBin>( *m_pWriterXMLCPBin ));
		}

		TSerialize GetTSerialize() { return TSerialize(m_pWriter.get()); }

	private:
		std::auto_ptr<ISerialize> m_pWriter;
		std::auto_ptr<CSerializeWriterXMLCPBin> m_pWriterXMLCPBin;
	};

	virtual bool Init(const char* name)
	{
		bool ok = inherited::Init( name );
		if (ok)
			m_binXmlWriter.Init( "SaveGame", GetFileName() );
		return ok;
	}

	// adds a node with only attributes to the root
	void AddSimpleXmlToRoot( XmlNodeRef node )
	{
		XMLCPB::CNodeLiveWriterRef binNode = m_binXmlWriter.GetRoot();
		assert( binNode.IsValid() );
		assert( node->getChildCount()==0 );
		binNode = binNode->AddChildNode( node->getTag() );
		for (int i=0; i<node->getNumAttributes(); ++i)
		{
			const char* pKey; 
			const char* pVal; 
			node->getAttributeByIndex( i, &pKey, &pVal );
			binNode->AddAttr( pKey, pVal );
		}
		binNode->Done();
	}

	virtual bool Write( const char * filename, XmlNodeRef data )
	{
		bool bResult = true;

		AddSimpleXmlToRoot( GetMetadataXmlNode() );

		if (m_binXmlWriter.FinishWritingFile())
		{
			// additional data are meta and thumbnail, both are not necessary for Durango at the moment. -- 31/5/2013 yeonwoon
			//bResult = inherited::WriteAdditionalData(filename, data);
		}

		return bResult;
	}

	TSerialize AddSection( const char * section )
	{
		XMLCPB::CNodeLiveWriterRef nodeRef = m_binXmlWriter.GetRoot()->AddChildNode( section );
		_smart_ptr<CSerializeCtx> pCtx = new CSerializeCtx(nodeRef, m_binXmlWriter );
		m_sections.push_back(pCtx);
		return pCtx->GetTSerialize();
	}

	XMLCPB::CWriterInterface m_binXmlWriter;      // XMLCPB -> Compressed Binary XML  TODO: change all those XMLCPB names...
	std::vector<_smart_ptr<CSerializeCtx> > m_sections;
};

//------------------------------------------------------------------------
ISaveGame* CPlayerProfileImplDurango::CreateSaveGame(SUserEntry* pEntry)
{
	if (pEntry && !pEntry->unknownUser && GetISystem()->GetPlatformOS()->UsePlatformSavingAPI())
	{
		// TODO: supporting possible options -- 31/5/2013 yeonwoon
		// - Normal XML
		// - CPlayerProfileManager::sUseRichSaveGames

		return new CXMLCPBinSaveGameDurangoConnectedStorage();
	}
	else
	{
		return CPlayerProfileImplConsole::CreateSaveGame(pEntry);
	}
}

//////////////////////////////////////////////////////////////////////////
class CXMLCPBinLoadGameDurangoConnectedStorage : public CXmlLoadGame
{
	typedef CXmlLoadGame inherited;

	class CSerializeCtx : public _reference_target_t
	{
	public:
		CSerializeCtx( XMLCPB::CNodeLiveReaderRef node, XMLCPB::CReaderInterface& binReader )
		{
			m_pReaderXMLCPBin = std::auto_ptr<CSerializeReaderXMLCPBin>( new CSerializeReaderXMLCPBin( node, binReader ) );
			m_pReader = std::auto_ptr<ISerialize>(new CSimpleSerializeWithDefaults<CSerializeReaderXMLCPBin>( *m_pReaderXMLCPBin ));
		}

		TSerialize GetTSerialize() { return TSerialize(m_pReader.get()); }

	private:
		std::auto_ptr<ISerialize> m_pReader;
		std::auto_ptr<CSerializeReaderXMLCPBin> m_pReaderXMLCPBin;
	};


public:
	CXMLCPBinLoadGameDurangoConnectedStorage()
		: m_pHeap(XMLCPB::CReader::CreateHeap())
		, m_binXmlReader(m_pHeap)
		, m_metadataNode(m_binXmlReader.CreateNodeRef())
	{
	}

	virtual bool Init(const char* name)
	{
		m_containerName = name;

		bool ok = m_binXmlReader.ReadBinaryFile( m_containerName.c_str() );
		if (ok)
		{
#ifdef XMLCPB_DEBUGUTILS
			if (CCryActionCVars::Get().g_XMLCPBGenerateXmlDebugFiles==1)
				XMLCPB::CDebugUtils::DumpToXmlFile( m_binXmlReader.GetRoot(), "LastBinaryLoaded.xml" );
#endif
			m_metadataNode = m_binXmlReader.GetRoot()->GetChildNode( "Metadata" );
		}
				
		return ok;
	}

	IGeneralMemoryHeap* GetHeap()
	{
		return m_pHeap;
	}
	
	const char* GetMetadata( const char* pTag )
	{
		const char* pVal = NULL;
		m_metadataNode->ReadAttr( pTag, pVal );
		return pVal;
	}

	bool GetMetadata( const char* pTag, int& value )
	{
		return m_metadataNode->ReadAttr( pTag, value );
	}

	bool HaveMetadata( const char* pTag )
	{
		return m_metadataNode->HaveAttr( pTag );
	}

	std::auto_ptr<TSerialize> GetSection( const char* pSection )
	{
		XMLCPB::CNodeLiveReaderRef node = m_binXmlReader.GetRoot()->GetChildNode( pSection );
		if (!node.IsValid())
			return std::auto_ptr<TSerialize>();
		_smart_ptr<CSerializeCtx> pCtx = new CSerializeCtx(node, m_binXmlReader);
		m_sections.push_back( pCtx );
		return std::auto_ptr<TSerialize>( new TSerialize(pCtx->GetTSerialize()) );
	}

	bool HaveSection( const char* pSection )
	{
		return m_binXmlReader.GetRoot()->GetChildNode(pSection).IsValid(); 
	}

	const char* GetFileName() const
	{
		return m_containerName.c_str();
	}		

	_smart_ptr<IGeneralMemoryHeap> m_pHeap;
	XMLCPB::CReaderInterface m_binXmlReader;      // XMLCPB -> Compressed Binary XML  TODO: change all those XMLCPB names...
	std::vector<_smart_ptr<CSerializeCtx> > m_sections;
	XMLCPB::CNodeLiveReaderRef m_metadataNode;
	string m_containerName;
};

//------------------------------------------------------------------------
ILoadGame* CPlayerProfileImplDurango::CreateLoadGame(SUserEntry* pEntry)
{
	if (pEntry && !pEntry->unknownUser && GetISystem()->GetPlatformOS()->UsePlatformSavingAPI())
	{
		// TODO: supporting possible options -- 31/5/2013 yeonwoon
		// - Normal XML
		// - CPlayerProfileManager::sUseRichSaveGames

		return new CXMLCPBinLoadGameDurangoConnectedStorage();
	}
	else
	{
		return CPlayerProfileImplConsole::CreateLoadGame(pEntry);
	}
}

//------------------------------------------------------------------------
bool CPlayerProfileImplDurango::DeleteSaveGame(SUserEntry* pEntry, const char* name)
{
	if (pEntry && !pEntry->unknownUser)
	{
		if (GetISystem()->GetPlatformOS()->UsePlatformSavingAPI())
		{
			wchar_t	containerNameW[_MAX_PATH];

			size_t numConverted = 0;
			mbstowcs_s(&numConverted, containerNameW, ARRAY_COUNT(containerNameW), name, _TRUNCATE);

			return GetISystem()->GetPlatformOS()->DeleteStorage(containerNameW);
		}
		else
		{
			// TODO: supporting possible options -- 4/6/2013 yeonwoon
			// - from file system
			// - rich stuff

			//return CPlayerProfileImplConsole::DeleteSaveGame(pEntry, name);
		}
	}

	return false;
}

//------------------------------------------------------------------------
bool CPlayerProfileImplDurango::ResolveAttributeBlock(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* attrBlockName, IResolveAttributeBlockListener* pListener, int reason)
{
	SResolveAttributeRequest request;
	request.attrBlockNames[ 0 ] = attrBlockName;
	request.attrBlockNames[ 1 ] = NULL;

	return ResolveAttributeBlock(pEntry, pProfile, request, pListener, reason);
}

namespace
{
	void DebugPrintResolve( const char* reason, const char* profileName, const IPlayerProfileManager::SResolveAttributeRequest& request )
	{
#ifdef DEBUG_PROFILE_IMPL_ALLOW_TRACE
		IPlatformOS::TUserName user = "";
		ISystem* pSystem = GetISystem();
		if (pSystem)
		{
			int userIndex = pSystem->GetPlatformOS()->GetFirstSignedInUser();
			if (userIndex != IPlatformOS::Unknown_User)
			{
				pSystem->GetPlatformOS()->UserGetName(userIndex, user);
			}
		}
		CryWarning( VALIDATOR_MODULE_NETWORK, VALIDATOR_WARNING, "[>PROFILE_DEBUG<][ATTRBLK] = = = = = = %s = = = = = (%s)",   reason, profileName );
		CryWarning( VALIDATOR_MODULE_NETWORK, VALIDATOR_WARNING, "[>PROFILE_DEBUG<][ATTRBLK] current user = %s, profile = %s", *user.c_str() ? user.c_str() : "#####NoId#####", profileName );
		CryWarning( VALIDATOR_MODULE_NETWORK, VALIDATOR_WARNING, "[>PROFILE_DEBUG<][ATTRBLK] path = %s",                       request.attrPathPrefix );
		CryWarning( VALIDATOR_MODULE_NETWORK, VALIDATOR_WARNING, "[>PROFILE_DEBUG<][ATTRBLK] container = %s",                  request.containerName );
		for( int i = 0; ( i < IPlayerProfileManager::SResolveAttributeRequest::eRABC_MaxResolveCountPerRequest ) && request.attrBlockNames[ i ]; ++i )
		{
			CryWarning( VALIDATOR_MODULE_NETWORK, VALIDATOR_WARNING, "[>PROFILE_DEBUG<][ATTRBLK] block %d = %s",                 i, request.attrBlockNames[ i ] );
		}
		CryWarning( VALIDATOR_MODULE_NETWORK, VALIDATOR_WARNING, "[>PROFILE_DEBUG<][ATTRBLK] ~ ~ ~ ~ ~ ~ %s ~ ~ ~ ~ ~",        reason );
#endif // DEBUG_PROFILE_IMPL_ALLOW_TRACE
	}

	void DebugPrintWrite( const char* reason, const char* profileName, const char* blockName )
	{
#ifdef DEBUG_PROFILE_IMPL_ALLOW_TRACE
		IPlatformOS::TUserName user = "";
		ISystem* pSystem = GetISystem();
		if (pSystem)
		{
			int userIndex = pSystem->GetPlatformOS()->GetFirstSignedInUser();
			if (userIndex != IPlatformOS::Unknown_User)
			{
				pSystem->GetPlatformOS()->UserGetName(userIndex, user);
			}
		}
		CryWarning( VALIDATOR_MODULE_NETWORK, VALIDATOR_WARNING, "[>PROFILE_DEBUG<][ATTRBLK] = = = = = = %s = = = = = (%s)",   reason, profileName );
		CryWarning( VALIDATOR_MODULE_NETWORK, VALIDATOR_WARNING, "[>PROFILE_DEBUG<][ATTRBLK] current user = %s, profile = %s", *user.c_str() ? user.c_str() : "#####NoId#####", profileName );
		CryWarning( VALIDATOR_MODULE_NETWORK, VALIDATOR_WARNING, "[>PROFILE_DEBUG<][ATTRBLK] block = %s", blockName );
		CryWarning( VALIDATOR_MODULE_NETWORK, VALIDATOR_WARNING, "[>PROFILE_DEBUG<][ATTRBLK] ~ ~ ~ ~ ~ ~ %s ~ ~ ~ ~ ~",        reason );
#endif // DEBUG_PROFILE_IMPL_ALLOW_TRACE
	}
}


//------------------------------------------------------------------------
bool CPlayerProfileImplDurango::ResolveAttributeBlock(SUserEntry* pEntry, CPlayerProfile* pProfile, const SResolveAttributeRequest& attrBlockNameRequest, IResolveAttributeBlockListener* pListener, int reason)
{
	// NB: please ensure that every code path either calls OnPostResolve or OnFailure on the listener.
	IPlatformOS *pOS = gEnv->pSystem->GetPlatformOS();

	DebugPrintResolve( "Resolve attribute block", pProfile->GetName(), attrBlockNameRequest );

	if (pEntry && !pEntry->unknownUser && pOS->UsePlatformSavingAPI())
	{
		int requestCount = 0;
		std::set< EPlatformStorageType > storageTypes;
		while( const char* const pAttributeName = attrBlockNameRequest.attrBlockNames[requestCount] )
		{
			storageTypes.insert( stl::find_in_map(m_attributeBlockStorageInfos, pAttributeName, SAttributeBlockStorageInfo()).resolveFrom );
			if( ++requestCount >= SResolveAttributeRequest::eRABC_MaxResolveCountPerRequest )
			{
				break;
			}
		}
		
		if( requestCount == 0 )
		{
			if(pListener)
			{
				pListener->OnPostResolve();
			}
			return true;	// Succeeded in reading no attribute block ...
		}

		if( storageTypes.size() != 1 )
		{
			GameWarning("CPlayerProfileImplDurango::ResolveAttributeBlock: requesting attribute blocks from different storages in a single request is not supported !");

			if(pListener)
			{
				pListener->OnFailure();
			}

			return false;
		}

		EPlatformStorageType resolveFrom = *storageTypes.begin();

		switch (resolveFrom)
		{
		case eEST_ConnectedStorage:
			{
				IPlatformOS::TContainerDataBlocks dataBlocks;
				dataBlocks.resize(requestCount);
				TDataBlockBuffer readBuffers[ SResolveAttributeRequest::eRABC_MaxResolveCountPerRequest ];
				
				std::vector<wstring> blockNames;
				blockNames.resize( requestCount );
				for(int i = 0; i < requestCount; ++i)
				{
					CryStringUtils::UTF8ToWStr(attrBlockNameRequest.attrBlockNames[ i ], blockNames[ i ]);

					TDataBlockBuffer& readBuffer = readBuffers[ i ];
					readBuffer.resize(kMaxProfileDataBlockSize);

					IPlatformOS::SContainerDataBlock& dataBlock = dataBlocks[i];
					dataBlock.blockName = blockNames[ i ].c_str();
					dataBlock.pDataBlock = &readBuffer[0];
					dataBlock.dataBlockSize = kMaxProfileDataBlockSize - 1; // preserve space for null termination; needed for jsmn
					dataBlock.pDataBlock[0] = '\0';
				}

				wstring containerNameW = PLAYERPROFILE_ATTR_CONTAINER_NAME;
				if(attrBlockNameRequest.containerName)
				{
					CryStringUtils::UTF8ToWStr(attrBlockNameRequest.containerName, containerNameW);
				}

				if (GetISystem()->GetPlatformOS()->LoadFromStorage(IPlatformOS::eCST_SRA, containerNameW.c_str(), dataBlocks))
				{
					for( int i = 0; i < requestCount; ++i )
					{
						IPlatformOS::SContainerDataBlock& dataBlock = dataBlocks[i];
						dataBlock.pDataBlock[dataBlock.dataBlockSize] = '\0';
						const char* blobJSON = (const char*)dataBlock.pDataBlock;

						bool decoded = DecodeProfileAttributesFromJSON(blobJSON, [pProfile, attrBlockNameRequest](const stack_string& path, const stack_string& value) {

							pProfile->SetAttribute(attrBlockNameRequest.attrPathPrefix + path, TFlowInputData(string(value)));

						});

						// NB: json parser is not that strict, sometimes it returns true even though format is not really JSON.
						assert(decoded);
					}

					GetManager()->ReloadProfile(pProfile, reason);

					if (pListener)
					{
						pListener->OnPostResolve();
					}

					return true;
				}
				else
				{
					GameWarning("CPlayerProfileImplDurango::ResolveAttributeBlock: failed to load data from connected storage");

					if (pListener)
					{
						pListener->OnFailure();
					}

					return false;
				}
			}
			break;

		case eEST_TitleStorage:
			if( requestCount > 1 )
			{
				// TODO : support this feature in the same fashion as above : listener should only be called once, when everything's been properly read.
				GameWarning("CPlayerProfileImplDurango::ResolveAttributeBlock: requesting multiple attribute blocks from Title Storage in a single request is not supported !");

				if(pListener)
				{
					pListener->OnFailure();
				}

				return false;
			}
			else
			{

				const char* attrBlockName = attrBlockNameRequest.attrBlockNames[ 0 ];
				DownloadProfileBlockFromTMS(attrBlockName, [this, pProfile, pListener, reason](const char* blobJSON) {

					// NB: this lambda function is called asynchronously
					bool decoded = DecodeProfileAttributesFromJSON(blobJSON, [pProfile](const stack_string& path, const stack_string& value) {

						pProfile->SetAttribute(path, TFlowInputData(string(value)));

					});

					// NB: json parser is not that strict, sometimes it returns true even though format is not really JSON.
					assert(decoded);

					GetManager()->ReloadProfile(pProfile, reason);

					if (pListener)
					{
						pListener->OnPostResolve();
					}
				},
					[pListener]() {

					if (pListener)
					{
						pListener->OnFailure();
					}
				});
				return true;
			}
			break;

		case eEST_SmartGlass:
			assert(!"invalid behaviour");
			if(pListener)
			{
				pListener->OnFailure();
			}
			break;

		default:
			if(pListener)
			{
				pListener->OnFailure();
			}
			break;
		}
	}

	return false;
}

//------------------------------------------------------------------------
bool CPlayerProfileImplDurango::WriteAttributeBlock(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* attrBlockName, int reason)
{
	IPlatformOS *pOS = gEnv->pSystem->GetPlatformOS();

	DebugPrintWrite( "Write attribute block", pProfile->GetName(), attrBlockName );
	if (pEntry && !pEntry->unknownUser && pOS->UsePlatformSavingAPI())
	{
		XmlNodeRef attributes = GetISystem()->CreateXmlNode(CPlayerProfile::ATTRIBUTES_TAG);

		TFlowInputData attrData;
		string temp;

		// since we don't have concept of attribute group in our profile attribute, we need to iterate all the attributes for now.
		const CPlayerProfile::TAttributeMap& attributeMap = pProfile->GetAttributeMap();
		std::for_each(attributeMap.begin(), attributeMap.end(), [&](const CPlayerProfile::TAttributeMap::value_type& item) {

			const string& path = item.first;

			int pos = 0;
			temp = path.Tokenize("/", pos);
			if (temp.compareNoCase(attrBlockName) == 0)
			{
				pProfile->GetAttribute(path, attrData);
				attrData.GetValueWithConversion(temp);

				XmlNodeRef child = attributes->newChild("Attr");
				child->setAttr("name", path);
				child->setAttr("value", temp);
			}

		});

		if (attributes->getChildCount() > 0)
		{
			return SavePlayerProfileAttributes(attributes);
		}
		else
		{
			GameWarning("CPlayerProfileImplDurango::WriteAttributeBlock: no attributes belong to block: %s", attrBlockName);
		}
	}

	return false;
}

//------------------------------------------------------------------------
void CPlayerProfileImplDurango::UploadProfileBlockToTMS(const char* attrBlockName, const string& blobJSON)
{
	ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
	ICryLobbyService* pLobbyService = pLobby ? pLobby->GetLobbyService(eCLS_Online) : NULL;
	ICryOnlineStorage* pOnlineStorage = pLobbyService ? pLobbyService->GetOnlineStorage() : NULL;
	if (pOnlineStorage)
	{
		struct SUploadParams : _reference_target_t
		{
			string itemUrl;
			string blob;
		};

		_smart_ptr<SUploadParams> pUploadParams = new SUploadParams();
		pUploadParams->itemUrl = attrBlockName;
		pUploadParams->blob = blobJSON;

		CryOnlineStorageQueryData queryData;
		queryData.userID = 0; // not used; just using current user
		queryData.operationType = eCOSO_Upload;
		queryData.storageLocation = eCOSL_User;
		queryData.dataType = eCOSDT_Json;
		queryData.szItemURL = pUploadParams->itemUrl.c_str();
		queryData.pBuffer = const_cast<char*>(pUploadParams->blob.c_str()); // pBuffer needs to be char* for the case download. safe to just remove constness here.
		queryData.bufferSize = pUploadParams->blob.length();

		queryData.pListener = new COnlineStorageQueryListenerAdapter([pUploadParams](const CryOnlineStorageQueryData& queryData) {

			if (queryData.eResult != eCLE_Success)
			{
				GameWarning("OnlineStorageDataQuery failed: %d", queryData.eResult);
			}
		});

		ECryLobbyError result = pOnlineStorage->OnlineStorageDataQuery(&queryData);
		if (result != eCLE_Success)
		{
			GameWarning("OnlineStorageDataQuery failed: %d", result);
		}
	}
}

//------------------------------------------------------------------------
void CPlayerProfileImplDurango::DownloadProfileBlockFromTMS(const char* attrBlockName, std::function<void(const char*)> onSuccess, std::function<void()> onFailure)
{
	ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
	ICryLobbyService* pLobbyService = pLobby ? pLobby->GetLobbyService(eCLS_Online) : NULL;
	ICryOnlineStorage* pOnlineStorage = pLobbyService ? pLobbyService->GetOnlineStorage() : NULL;
	if (pOnlineStorage)
	{
		struct SDownloadParams : _reference_target_t
		{
			string itemUrl;
			
			typedef std::vector<char> TDataBuffer;
			TDataBuffer dataBuffer;
		};

		_smart_ptr<SDownloadParams> pUploadParams = new SDownloadParams();
		pUploadParams->itemUrl = attrBlockName;
		pUploadParams->dataBuffer.resize(kMaxProfileDataBlockSize);

		CryOnlineStorageQueryData queryData;
		queryData.userID = 0; // not used; just using current user
		queryData.operationType = eCOSO_Download;
		queryData.storageLocation = eCOSL_User;
		queryData.dataType = eCOSDT_Json;
		queryData.szItemURL = pUploadParams->itemUrl.c_str();
		queryData.pBuffer = &pUploadParams->dataBuffer[0];
		queryData.bufferSize = kMaxProfileDataBlockSize - 1; // preserve space for null termination; needed for jsmn

		queryData.pListener = new COnlineStorageQueryListenerAdapter([pUploadParams,onSuccess,onFailure](const CryOnlineStorageQueryData& queryData) {

			if (queryData.eResult != eCLE_Success)
			{
				GameWarning("OnlineStorageDataQuery failed: %d", queryData.eResult);
				onFailure();
			}
			else
			{
				queryData.pBuffer[queryData.bufferSize] = '\0';
				onSuccess(queryData.pBuffer);
			}
		});

		ECryLobbyError result = pOnlineStorage->OnlineStorageDataQuery(&queryData);
		if (result != eCLE_Success)
		{
			GameWarning("OnlineStorageDataQuery failed: %d", result);
		}
	}
}

#endif
