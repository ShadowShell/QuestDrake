// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "TagDefinitionXml.h"

#include "ICryMannequin.h"



#if defined( USER_pau )
#define TEST_TAG_DEFINITION_XML
#endif

#if defined( _RELEASE )
#define RECURSIVE_IMPORT_CHECK (0)
#else
#define RECURSIVE_IMPORT_CHECK (1)
#endif



namespace mannequin
{
	static void NormalizeFilename(char outFilename[DEF_PATH_LENGTH], const char *inFilename)
	{
		outFilename[0] = '\0';
		strncpy(outFilename, inFilename, DEF_PATH_LENGTH);
		for (size_t i = 0; i < DEF_PATH_LENGTH; ++i)
		{
			if (outFilename[i] == '\\')
				outFilename[i] = '/';
		}
	}
}


//////////////////////////////////////////////////////////////////////////
#if RECURSIVE_IMPORT_CHECK
typedef std::vector< string > TRecursiveGuardList;
namespace
{
	bool FindInRecursiveGuardList( const TRecursiveGuardList& list, const char* const filename )
	{
		for ( int i = 0; i < list.size(); ++i )
		{
			const char* const listFilename = list[ i ];
			if ( stricmp( filename, listFilename ) == 0 )
			{
				return true;
			}
		}
		return false;
	}
}
#define RECURSIVE_GUARD_LIST_PUSH( filename ) recursiveGuardListOut.push_back( filename );
#define RECURSIVE_GUARD_LIST_POP() recursiveGuardListOut.pop_back();
#define RECURSIVE_GUARD_LIST_FIND( filename ) ( FindInRecursiveGuardList( recursiveGuardListOut, filename ) )
#else
typedef int TRecursiveGuardList;
#define RECURSIVE_GUARD_LIST_PUSH( filename )
#define RECURSIVE_GUARD_LIST_POP()
#define RECURSIVE_GUARD_LIST_FIND( filename ) ( false )
#endif

typedef std::vector< string > TImportsList;

//////////////////////////////////////////////////////////////////////////
namespace mannequin
{
	int GetTagDefinitionVersion( XmlNodeRef pXmlNode );
	bool LoadTagDefinitionImpl( const char* const normalizedFilename, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo );

	XmlNodeRef SaveTagDefinitionImpl( const CTagDefinition& tagDefinition, const TImportsList& importsList );

	namespace impl
	{
		std::map< uint32, STagDefinitionImportsInfo > g_defaultImportInfo;

		bool LoadTagDefinitionImplXml( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo );
	}
}


//////////////////////////////////////////////////////////////////////////
STagDefinitionImportsInfo& mannequin::GetDefaultImportsInfo( const char* const filename )
{
	assert( filename );
	assert( filename[ 0 ] );

	const uint32 crc = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase( filename );
	STagDefinitionImportsInfo& importsInfo = impl::g_defaultImportInfo[ crc ];
	importsInfo.SetFilename( filename );
	return importsInfo;
}


//////////////////////////////////////////////////////////////////////////
void mannequin::OnDatabaseManagerUnload()
{
	impl::g_defaultImportInfo.clear();
}



//////////////////////////////////////////////////////////////////////////
namespace TagDefinitionXmlVersion2
{
	bool Load( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo );
	void Save( XmlNodeRef pXmlNode, const CTagDefinition& tagDefinition, const TImportsList& importsList );

	namespace impl
	{
		bool LoadImportsNode( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo );
		bool LoadImportNode( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo );
		bool LoadTagsNode( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo );
		bool LoadTagOrGroupNode( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo );
		bool LoadTagNode( XmlNodeRef pXmlNode, const char* const groupName, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo );
		bool LoadGroupNode( XmlNodeRef pXmlNode, const int childCount, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo );

		XmlNodeRef CreateChildNode( XmlNodeRef pXmlNode, const char* const childName );

		void SaveImportsNode( XmlNodeRef pXmlNode, const TImportsList& importsList );
		void SaveImportNode( XmlNodeRef pXmlNode, const string& filename );
		void SaveTagsNode( XmlNodeRef pXmlNode, const CTagDefinition& tagDefinition );
	}
};


bool TagDefinitionXmlVersion2::Load( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo )
{
	assert( pXmlNode != 0 );

	XmlNodeRef pXmlImportsNode = pXmlNode->findChild( "Imports" );
	const bool importsLoadSuccess = impl::LoadImportsNode( pXmlImportsNode, tagDefinitionOut, recursiveGuardListOut, importsInfo );
	if ( ! importsLoadSuccess )
	{
		return false;
	}

	XmlNodeRef pXmlTagsNode = pXmlNode->findChild( "Tags" );
	const bool loadTagsSuccess = impl::LoadTagsNode( pXmlTagsNode, tagDefinitionOut, recursiveGuardListOut, importsInfo );
	return loadTagsSuccess;
}

void TagDefinitionXmlVersion2::Save( XmlNodeRef pXmlNode, const CTagDefinition& tagDefinition, const TImportsList& importsList )
{
	pXmlNode->setAttr( "version", 2 );

	impl::SaveImportsNode( pXmlNode, importsList );
	impl::SaveTagsNode( pXmlNode, tagDefinition );
}


bool TagDefinitionXmlVersion2::impl::LoadImportsNode( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo )
{
	importsInfo.ClearImports();
	if ( ! pXmlNode )
	{
		return true;
	}

	const int childCount = pXmlNode->getChildCount();
	for ( int i = 0; i < childCount; ++i )
	{
		XmlNodeRef pXmlChildNode = pXmlNode->getChild( i );
		const bool loadSuccess = LoadImportNode( pXmlChildNode, tagDefinitionOut, recursiveGuardListOut, importsInfo );
		if ( ! loadSuccess )
		{
			return false;
		}
	}
	return true;
}


bool TagDefinitionXmlVersion2::impl::LoadImportNode( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo )
{
	assert( pXmlNode != 0 );

	const char* const filename = pXmlNode->getAttr( "filename" );
	if ( ! filename )
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Tag definition failed to find 'filename' attribute in an 'Import' node." );
		return false;
	}

	char normalizedFilename[ DEF_PATH_LENGTH ];
	mannequin::NormalizeFilename( normalizedFilename, filename );

	STagDefinitionImportsInfo& subImportsInfo = importsInfo.FindOrAddImport( normalizedFilename );
	const bool loadImportedTagDefinitionSuccess = mannequin::LoadTagDefinitionImpl( normalizedFilename, tagDefinitionOut, recursiveGuardListOut, subImportsInfo );
	return loadImportedTagDefinitionSuccess;
}


bool TagDefinitionXmlVersion2::impl::LoadTagsNode( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo )
{
	importsInfo.ClearTags();
	if ( ! pXmlNode )
	{
		return true;
	}

	bool loadSuccess = true;

	const int childCount = pXmlNode->getChildCount();
	for ( int i = 0; i < childCount; ++i )
	{
		XmlNodeRef pXmlChildNode = pXmlNode->getChild( i );
		loadSuccess &= LoadTagOrGroupNode( pXmlChildNode, tagDefinitionOut, recursiveGuardListOut, importsInfo );
	}

	return loadSuccess;
}


bool TagDefinitionXmlVersion2::impl::LoadTagOrGroupNode( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo )
{
	assert( pXmlNode != 0 );
	const int childCount = pXmlNode->getChildCount();
	const bool isGroupNode = ( childCount != 0 );
	if ( isGroupNode )
	{
		const bool loadGroupSuccess = LoadGroupNode( pXmlNode, childCount, tagDefinitionOut, recursiveGuardListOut, importsInfo );
		return loadGroupSuccess;
	}
	else
	{
		const bool loadTagSuccess = LoadTagNode( pXmlNode, NULL, tagDefinitionOut, recursiveGuardListOut, importsInfo );
		return loadTagSuccess;
	}
}


bool TagDefinitionXmlVersion2::impl::LoadTagNode( XmlNodeRef pXmlNode, const char* const groupName, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo )
{
	assert( pXmlNode != 0 );

	uint32 priority = 0;
	pXmlNode->getAttr( "priority", priority );

	const char* const tagName = pXmlNode->getAttr( "name" );
	if ( ! tagName )
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to find a 'name' attribute in a 'tag' node." );
		return false;
	}

	const TagID tagId = tagDefinitionOut.AddTag( tagName, groupName, priority );
	if ( tagId == TAG_ID_INVALID )
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Duplicate tag '%s'", tagName );
		// We will continue loading
	}
	else
	{
		const char * const subTagsFilename = pXmlNode->getAttr( "subTagDef" );
		const bool hasSubTags = ( subTagsFilename && subTagsFilename[ 0 ] );
		if ( hasSubTags )
		{
			char subTagsNormalizedFilename[ DEF_PATH_LENGTH ];
			mannequin::NormalizeFilename( subTagsNormalizedFilename, subTagsFilename );

			CCryAction* const pCryAction = CCryAction::GetCryAction();
			CRY_ASSERT( pCryAction );

			IMannequin& mannequinSys = pCryAction->GetMannequinInterface();
			IAnimationDatabaseManager& animationDatabaseManager = mannequinSys.GetAnimationDatabaseManager();

			const CTagDefinition* pSubTagDef = animationDatabaseManager.FindTagDef( subTagsNormalizedFilename );
			if ( pSubTagDef == NULL )
			{
				CTagDefinition* pNewTagDef = new CTagDefinition();
				pNewTagDef->SetFilename( subTagsNormalizedFilename );

				STagDefinitionImportsInfo& subImportsInfo = mannequin::GetDefaultImportsInfo( subTagsNormalizedFilename );
				const bool loadSubTagDefSuccess = mannequin::LoadTagDefinitionImpl( subTagsNormalizedFilename, *pNewTagDef, recursiveGuardListOut, subImportsInfo );
				pNewTagDef->AssignBits();

				if ( loadSubTagDefSuccess )
				{
					const bool registerTagDefSuccess = animationDatabaseManager.RegisterTagDef( pNewTagDef );
					CRY_ASSERT( registerTagDefSuccess );
					pSubTagDef = pNewTagDef;
				}
				else
				{
					CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to load sub tag definition for tag '%s'", tagName );
					delete pNewTagDef;

					return false;
				}
			}

			if ( pSubTagDef != NULL )
			{
				tagDefinitionOut.SetSubTagDefinition( tagId, pSubTagDef );
			}
		}
	}

	importsInfo.AddTag( tagId );

	return true;
}


bool TagDefinitionXmlVersion2::impl::LoadGroupNode( XmlNodeRef pXmlNode, const int childCount, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo )
{
	assert( pXmlNode != 0 );
	assert( pXmlNode->getChildCount() == childCount );

	const char* const groupName = pXmlNode->getAttr( "name" );
	if ( ! groupName )
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to find a 'name' attribute in a 'group' node." );
		return false;
	}
	else if (tagDefinitionOut.FindGroup(groupName) != GROUP_ID_NONE)
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Group '%s' overrides existing group.", groupName);
		return false;
	}

	bool loadSuccess = true;
	for ( int i = 0; i < childCount; ++i )
	{
		XmlNodeRef pXmlChildNode = pXmlNode->getChild( i );
		loadSuccess &= LoadTagNode( pXmlChildNode, groupName, tagDefinitionOut, recursiveGuardListOut, importsInfo );
	}
	return loadSuccess;
}


XmlNodeRef TagDefinitionXmlVersion2::impl::CreateChildNode( XmlNodeRef pXmlNode, const char* const childName )
{
	assert( pXmlNode != 0 );
	assert( childName );
	assert( childName[ 0 ] );

	XmlNodeRef pXmlChildNode = pXmlNode->createNode( childName );
	pXmlNode->addChild( pXmlChildNode );
	return pXmlChildNode;
}


void TagDefinitionXmlVersion2::impl::SaveImportsNode( XmlNodeRef pXmlNode, const TImportsList& importsList )
{
	assert( pXmlNode != 0 );

	const size_t importsCount = importsList.size();
	if ( importsCount == 0 )
	{
		return;
	}

	XmlNodeRef pXmlImportsNode = CreateChildNode( pXmlNode, "Imports" );

	for ( size_t i = 0; i < importsCount; ++i )
	{
		const string& filename = importsList[ i ];
		SaveImportNode( pXmlImportsNode, filename );
	}
}

void TagDefinitionXmlVersion2::impl::SaveImportNode( XmlNodeRef pXmlNode, const string& filename )
{
	assert( pXmlNode != 0 );

	XmlNodeRef pXmlImportNode = CreateChildNode( pXmlNode, "Import" );
	pXmlImportNode->setAttr( "filename", filename.c_str() );
}


void TagDefinitionXmlVersion2::impl::SaveTagsNode( XmlNodeRef pXmlNode, const CTagDefinition& tagDefinition )
{
	const TagID tagCount = tagDefinition.GetNum();
	if ( tagCount == 0 )
	{
		return;
	}

	XmlNodeRef pXmlTagsNode = CreateChildNode( pXmlNode, "Tags" );

	std::vector< XmlNodeRef > groupXmlNodes;

	const TagGroupID groupCount = tagDefinition.GetNumGroups();
	groupXmlNodes.resize( groupCount );
	for ( TagGroupID i = 0; i < groupCount; ++i )
	{
		const char* const groupName = tagDefinition.GetGroupName( i );
		XmlNodeRef pXmlTagGroup = CreateChildNode( pXmlTagsNode, "Group" );
		pXmlTagGroup->setAttr( "name", groupName );

		groupXmlNodes[ i ] = pXmlTagGroup;
	}

	for ( TagID i = 0; i < tagCount; ++i )
	{
		const char* const tagName = tagDefinition.GetTagName( i );
		XmlNodeRef pXmlTagNode = pXmlTagsNode->createNode( "Tag" );
		pXmlTagNode->setAttr( "name", tagName );

		const uint32 tagPriority = tagDefinition.GetPriority( i );
		if ( tagPriority != 0 )
		{
			pXmlTagNode->setAttr( "priority", tagPriority );
		}

		const CTagDefinition *pTagDef = tagDefinition.GetSubTagDefinition(i);
		if (pTagDef)
		{
			pXmlTagNode->setAttr( "subTagDef", pTagDef->GetFilename() );
		}

		const TagGroupID groupId = tagDefinition.GetGroupID( i );
		if ( groupId == GROUP_ID_NONE )
		{
			pXmlTagsNode->addChild( pXmlTagNode );
		}
		else
		{
			XmlNodeRef pXmlTagGroup = groupXmlNodes[ groupId ];
			pXmlTagGroup->addChild( pXmlTagNode );
		}
	}
}



//////////////////////////////////////////////////////////////////////////
int mannequin::GetTagDefinitionVersion( XmlNodeRef pXmlNode )
{
	if ( stricmp( pXmlNode->getTag(), "TagDefinition" ) != 0 )
	{
		return -1;
	}

	int version = 1;
	pXmlNode->getAttr( "version", version );
	return version;
}



//////////////////////////////////////////////////////////////////////////
bool mannequin::LoadTagDefinitionImpl( const char* const normalizedFilename, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo )
{
	assert( normalizedFilename );
	assert( normalizedFilename[ 0 ] );

	const bool foundCyclicDependency = RECURSIVE_GUARD_LIST_FIND( normalizedFilename );
	if ( foundCyclicDependency )
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to load tag definition file '%s' due to a cyclic dependency.", normalizedFilename );
		return false;
	}

	RECURSIVE_GUARD_LIST_PUSH( normalizedFilename );

	XmlNodeRef pXmlNode = GetISystem()->LoadXmlFromFile( normalizedFilename );
	if ( ! pXmlNode )
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to load tag definition file '%s'", normalizedFilename );
		return false;
	}

	CRY_DEFINE_ASSET_SCOPE( "TagDefinition", normalizedFilename );
	const bool loadSuccess = impl::LoadTagDefinitionImplXml( pXmlNode, tagDefinitionOut, recursiveGuardListOut, importsInfo );

	RECURSIVE_GUARD_LIST_POP();

	return loadSuccess;
}



//////////////////////////////////////////////////////////////////////////
bool mannequin::impl::LoadTagDefinitionImplXml( XmlNodeRef pXmlNode, CTagDefinition& tagDefinitionOut, TRecursiveGuardList& recursiveGuardListOut, STagDefinitionImportsInfo& importsInfo )
{
	assert( pXmlNode != 0 );

	bool loadSuccess = false;

	const int version = GetTagDefinitionVersion( pXmlNode );
	switch ( version )
	{
	case 2:
		loadSuccess = TagDefinitionXmlVersion2::Load( pXmlNode, tagDefinitionOut, recursiveGuardListOut, importsInfo );
		break;
	default:
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to load tag definition: Unsupported version '%d'", version );
		break;
	}

	return loadSuccess;
}


//////////////////////////////////////////////////////////////////////////
bool mannequin::LoadTagDefinition( const char* const normalizedFilename, CTagDefinition& tagDefinitionOut, bool isTags )
{
	assert( normalizedFilename );
	assert( normalizedFilename[ 0 ] );

	tagDefinitionOut.Clear();
	tagDefinitionOut.SetFilename( normalizedFilename );

	TRecursiveGuardList recursiveGuardListOut;

	STagDefinitionImportsInfo& importsInfo = GetDefaultImportsInfo( normalizedFilename );
	importsInfo.Clear();
	const bool loadSuccess = LoadTagDefinitionImpl( normalizedFilename, tagDefinitionOut, recursiveGuardListOut, importsInfo );

	if (isTags)
	{
		tagDefinitionOut.AssignBits();
	}

	return loadSuccess;
}


//////////////////////////////////////////////////////////////////////////
XmlNodeRef mannequin::SaveTagDefinitionImpl( const CTagDefinition& tagDefinition, const TImportsList& importList )
{
	XmlNodeRef pXmlNode = GetISystem()->CreateXmlNode( "TagDefinition" );
	TagDefinitionXmlVersion2::Save( pXmlNode, tagDefinition, importList );
	return pXmlNode;
}


//////////////////////////////////////////////////////////////////////////
XmlNodeRef mannequin::SaveTagDefinition( const CTagDefinition& tagDefinition )
{
	const TImportsList emptyImportList;
	return SaveTagDefinitionImpl( tagDefinition, emptyImportList );
}


//////////////////////////////////////////////////////////////////////////
namespace
{
	size_t FindImportInfoIndex( const std::vector< const STagDefinitionImportsInfo* >& importsInfoList, const STagDefinitionImportsInfo& importsInfo )
	{
		const size_t totalImportsInfoCount = importsInfoList.size();
		for ( size_t i = 0; i < totalImportsInfoCount; ++i )
		{
			const STagDefinitionImportsInfo* pCurrentImportsInfo = importsInfoList[ i ];
			if ( pCurrentImportsInfo == &importsInfo )
			{
				return i;
			}
		}
		assert( false );
		return size_t( ~0 );
	}
}


//////////////////////////////////////////////////////////////////////////
void mannequin::SaveTagDefinition( const CTagDefinition& tagDefinition, TTagDefinitionSaveDataList& saveDataListOut )
{
	const char* const mainFilename = tagDefinition.GetFilename();
	const STagDefinitionImportsInfo& importsInfo = GetDefaultImportsInfo( mainFilename );

	std::vector< const STagDefinitionImportsInfo* > importsInfoList;
	importsInfo.FlattenImportsInfo( importsInfoList );
	const size_t totalImportsInfoCount = importsInfoList.size();

	std::vector< CTagDefinition > saveTagDefinitionList;
	saveTagDefinitionList.resize( totalImportsInfoCount );

	for ( size_t i = 0; i < totalImportsInfoCount; ++i )
	{
		const STagDefinitionImportsInfo* pImportsInfo = importsInfoList[ i ];
		assert( pImportsInfo );
		const char* const filename = pImportsInfo->GetFilename();

		CTagDefinition& tagDefinitionSave = saveTagDefinitionList[ i ];
		tagDefinitionSave.SetFilename( filename );
	}

	const TagID tagCount = tagDefinition.GetNum();
	for ( TagID tagId = 0; tagId < tagCount; ++tagId )
	{
		const STagDefinitionImportsInfo& importInfoForTag = importsInfo.Find( tagId );

		const size_t importInfoId = FindImportInfoIndex( importsInfoList, importInfoForTag );
		assert( importInfoId < importsInfoList.size() );

		const char* const tagName = tagDefinition.GetTagName( tagId );
		const TagGroupID tagGroupId = tagDefinition.GetGroupID( tagId );
		const char* const groupName = ( tagGroupId == GROUP_ID_NONE ) ? NULL : tagDefinition.GetGroupName( tagGroupId );
		const int tagPriority = tagDefinition.GetPriority( tagId );

		CTagDefinition& tagDefinitionSave = saveTagDefinitionList[ importInfoId ];
		const TagID newTagID = tagDefinitionSave.AddTag( tagName, groupName, tagPriority );
		const CTagDefinition* pSubTagDefinition = tagDefinition.GetSubTagDefinition( tagId );
		tagDefinitionSave.SetSubTagDefinition( newTagID, pSubTagDefinition );
	}

	for ( size_t i = 0; i < totalImportsInfoCount; ++i )
	{
		const STagDefinitionImportsInfo* pImportsInfo = importsInfoList[ i ];
		const CTagDefinition& tagDefinitionSave = saveTagDefinitionList[ i ];

		TImportsList importList;
		const size_t subImportsCount = pImportsInfo->GetImportCount();
		for ( size_t j = 0; j < subImportsCount; ++j )
		{
			const STagDefinitionImportsInfo& subImportInfo = pImportsInfo->GetImport( j );
			const char* const filename = subImportInfo.GetFilename();
			stl::push_back_unique(importList, filename);
		}

		STagDefinitionSaveData saveInfo;
		saveInfo.pXmlNode = SaveTagDefinitionImpl( tagDefinitionSave, importList );
		saveInfo.filename = tagDefinitionSave.GetFilename();

		saveDataListOut.push_back( saveInfo );
	}
}






//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#ifdef TEST_TAG_DEFINITION_XML



namespace mannequin
{
	void CheckTagExists( const CTagDefinition& tags, const char* const tagName )
	{
		assert( tagName );
		assert( tagName[ 0 ] );

		const TagID tagId = tags.Find( tagName );
		const bool tagExists = ( tagId != TAG_ID_INVALID );
		CRY_UNIT_TEST_ASSERT( tagExists );
	}

	void CheckTagsC( const CTagDefinition& tags )
	{
		CheckTagExists( tags, "C0" );
		CheckTagExists( tags, "C1" );
		CheckTagExists( tags, "C2" );
	}

	void CheckTagsB0( const CTagDefinition& tags )
	{
		CheckTagExists( tags, "B0" );
		CheckTagsC( tags );
	}

	void CheckTagsB1( const CTagDefinition& tags )
	{
		CheckTagExists( tags, "B1" );
		CheckTagsC( tags );
	}

	void CheckTagsA( const CTagDefinition& tags )
	{
		CheckTagExists( tags, "A0" );
		CheckTagExists( tags, "A1" );
		CheckTagExists( tags, "A2" );

		CheckTagsB0( tags );
		CheckTagsB1( tags );
	}

	void CheckTagsS( const CTagDefinition& tags )
	{
		CheckTagExists( tags, "S0" );
		{
			const CTagDefinition* const pSubTags = tags.GetSubTagDefinition( tags.Find( "S0" ) );
			CRY_UNIT_TEST_ASSERT( pSubTags != NULL );

			CheckTagsA( *pSubTags );
		}

		CheckTagExists( tags, "S1" );
		{
			const CTagDefinition* const pSubTags = tags.GetSubTagDefinition( tags.Find( "S1" ) );
			CRY_UNIT_TEST_ASSERT( pSubTags != NULL );

			CheckTagsB0( *pSubTags );
		}

		CheckTagExists( tags, "S2" );
		{
			const CTagDefinition* const pSubTags = tags.GetSubTagDefinition( tags.Find( "S2" ) );
			CRY_UNIT_TEST_ASSERT( pSubTags != NULL );

			CheckTagsC( *pSubTags );
		}

		CheckTagsA( tags );
	}


	CRY_UNIT_TEST( TestImporting )
	{
		CTagDefinition tags;
		{
			const bool loadSuccess = mannequin::LoadTagDefinition( "Animations/Mannequin/Test/ATags.xml", tags );
			CRY_UNIT_TEST_ASSERT( loadSuccess );
		}

		mannequin::CheckTagsA( tags );

		mannequin::TTagDefinitionSaveDataList saveDataList;
		mannequin::SaveTagDefinition( tags, saveDataList );
		{
			CRY_UNIT_TEST_CHECK_EQUAL( saveDataList.size(), 4 );

			bool checked[ 4 ] = { false, false, false, false };

			for ( size_t i = 0; i < saveDataList.size(); ++i )
			{
				const mannequin::STagDefinitionSaveData& saveData = saveDataList[ i ];

				if ( strcmpi( saveData.filename, "Animations/Mannequin/Test/ATags.xml" ) == 0 )
				{
					CTagDefinition tagsA;
					TRecursiveGuardList recursiveGuardList;
					STagDefinitionImportsInfo importsInfo;

					const bool loadSuccess = impl::LoadTagDefinitionImplXml( saveData.pXmlNode, tagsA, recursiveGuardList, importsInfo );
					CRY_UNIT_TEST_ASSERT( loadSuccess );

					mannequin::CheckTagsA( tagsA );
					checked[ 0 ] = true;
				}
				else if ( strcmpi( saveData.filename, "Animations/Mannequin/Test/B0Tags.xml" ) == 0 )
				{
					CTagDefinition tagsB0;
					TRecursiveGuardList recursiveGuardList;
					STagDefinitionImportsInfo importsInfo;

					const bool loadSuccess = impl::LoadTagDefinitionImplXml( saveData.pXmlNode, tagsB0, recursiveGuardList, importsInfo );
					CRY_UNIT_TEST_ASSERT( loadSuccess );

					mannequin::CheckTagsB0( tagsB0 );
					checked[ 1 ] = true;
				}
				else if ( strcmpi( saveData.filename, "Animations/Mannequin/Test/B1Tags.xml" ) == 0 )
				{
					CTagDefinition tagsB1;
					TRecursiveGuardList recursiveGuardList;
					STagDefinitionImportsInfo importsInfo;

					const bool loadSuccess = impl::LoadTagDefinitionImplXml( saveData.pXmlNode, tagsB1, recursiveGuardList, importsInfo );
					CRY_UNIT_TEST_ASSERT( loadSuccess );

					mannequin::CheckTagsB1( tagsB1 );
					checked[ 2 ] = true;
				}
				else if ( strcmpi( saveData.filename, "Animations/Mannequin/Test/CTags.xml" ) == 0 )
				{
					CTagDefinition tagsC;
					TRecursiveGuardList recursiveGuardList;
					STagDefinitionImportsInfo importsInfo;

					const bool loadSuccess = impl::LoadTagDefinitionImplXml( saveData.pXmlNode, tagsC, recursiveGuardList, importsInfo );
					CRY_UNIT_TEST_ASSERT( loadSuccess );

					mannequin::CheckTagsC( tagsC );
					checked[ 3 ] = true;
				}
				else
				{
					CRY_UNIT_TEST_ASSERT( false );
				}
			}

			for ( size_t i = 0; i < 4; ++i )
			{
				CRY_UNIT_TEST_ASSERT( checked[ i ] );
			}
		}
	}


#if RECURSIVE_IMPORT_CHECK
	CRY_UNIT_TEST( TestImportingRecursive )
	{
		CTagDefinition tags;
		const bool loadSuccess = mannequin::LoadTagDefinition( "Animations/Mannequin/Test/RecATags.xml", tags );
		CRY_UNIT_TEST_ASSERT( ! loadSuccess );
	}
#endif


	CRY_UNIT_TEST( TestSubTagDefinition )
	{
		CTagDefinition tags;
		const bool loadSuccess = mannequin::LoadTagDefinition( "Animations/Mannequin/Test/SubTags.xml", tags );
		CRY_UNIT_TEST_ASSERT( loadSuccess );

		{
			mannequin::CheckTagsS( tags );
		}

		mannequin::TTagDefinitionSaveDataList saveDataList;
		mannequin::SaveTagDefinition( tags, saveDataList );
		{
			CRY_UNIT_TEST_CHECK_EQUAL( saveDataList.size(), 5 );

			bool checked[ 5 ] = { false, false, false, false, false };

			for ( size_t i = 0; i < saveDataList.size(); ++i )
			{
				const mannequin::STagDefinitionSaveData& saveData = saveDataList[ i ];

				if ( strcmpi( saveData.filename, "Animations/Mannequin/Test/ATags.xml" ) == 0 )
				{
					CTagDefinition tagsA;
					TRecursiveGuardList recursiveGuardList;
					STagDefinitionImportsInfo importsInfo;

					const bool loadSuccess = impl::LoadTagDefinitionImplXml( saveData.pXmlNode, tagsA, recursiveGuardList, importsInfo );
					CRY_UNIT_TEST_ASSERT( loadSuccess );

					mannequin::CheckTagsA( tagsA );
					checked[ 0 ] = true;
				}
				else if ( strcmpi( saveData.filename, "Animations/Mannequin/Test/B0Tags.xml" ) == 0 )
				{
					CTagDefinition tagsB0;
					TRecursiveGuardList recursiveGuardList;
					STagDefinitionImportsInfo importsInfo;

					const bool loadSuccess = impl::LoadTagDefinitionImplXml( saveData.pXmlNode, tagsB0, recursiveGuardList, importsInfo );
					CRY_UNIT_TEST_ASSERT( loadSuccess );

					mannequin::CheckTagsB0( tagsB0 );
					checked[ 1 ] = true;
				}
				else if ( strcmpi( saveData.filename, "Animations/Mannequin/Test/B1Tags.xml" ) == 0 )
				{
					CTagDefinition tagsB1;
					TRecursiveGuardList recursiveGuardList;
					STagDefinitionImportsInfo importsInfo;

					const bool loadSuccess = impl::LoadTagDefinitionImplXml( saveData.pXmlNode, tagsB1, recursiveGuardList, importsInfo );
					CRY_UNIT_TEST_ASSERT( loadSuccess );

					mannequin::CheckTagsB1( tagsB1 );
					checked[ 2 ] = true;
				}
				else if ( strcmpi( saveData.filename, "Animations/Mannequin/Test/CTags.xml" ) == 0 )
				{
					CTagDefinition tagsC;
					TRecursiveGuardList recursiveGuardList;
					STagDefinitionImportsInfo importsInfo;

					const bool loadSuccess = impl::LoadTagDefinitionImplXml( saveData.pXmlNode, tagsC, recursiveGuardList, importsInfo );
					CRY_UNIT_TEST_ASSERT( loadSuccess );

					mannequin::CheckTagsC( tagsC );
					checked[ 3 ] = true;
				}
				else if ( strcmpi( saveData.filename, "Animations/Mannequin/Test/SubTags.xml" ) == 0 )
				{
					CTagDefinition tagsS;
					TRecursiveGuardList recursiveGuardList;
					STagDefinitionImportsInfo importsInfo;

					const bool loadSuccess = impl::LoadTagDefinitionImplXml( saveData.pXmlNode, tagsS, recursiveGuardList, importsInfo );
					CRY_UNIT_TEST_ASSERT( loadSuccess );

					mannequin::CheckTagsS( tagsS );
					checked[ 4 ] = true;
				}
				else
				{
					CRY_UNIT_TEST_ASSERT( false );
				}
			}

			for ( size_t i = 0; i < 5; ++i )
			{
				CRY_UNIT_TEST_ASSERT( checked[ i ] );
			}
		}
	}


#if RECURSIVE_IMPORT_CHECK
	CRY_UNIT_TEST( TestSubTagDefinitionRecursive )
	{
		{
			CTagDefinition tags;
			const bool loadSuccess = mannequin::LoadTagDefinition( "Animations/Mannequin/Test/RecSubTags.xml", tags );
			CRY_UNIT_TEST_ASSERT( ! loadSuccess );
		}
		{
			CTagDefinition tags;
			const bool loadSuccess = mannequin::LoadTagDefinition( "Animations/Mannequin/Test/RecSubTagsB.xml", tags );
			CRY_UNIT_TEST_ASSERT( ! loadSuccess );
		}
	}
#endif


	CRY_UNIT_TEST( TestImportInfo )
	{
		for ( int i = 0; i < 2; ++i )
		{
			CTagDefinition tags;
			const bool loadSuccess = mannequin::LoadTagDefinition( "Animations/Mannequin/Test/ATags.xml", tags );
			CRY_UNIT_TEST_ASSERT( loadSuccess );
		}

		const STagDefinitionImportsInfo& importsInfo = mannequin::GetDefaultImportsInfo( "Animations/Mannequin/Test/ATags.xml" );
		const int importCount = importsInfo.GetImportCount();
		CRY_UNIT_TEST_CHECK_EQUAL( importCount, 2 );
	}
}


#endif
