#ifndef __I_XML_ARCHIVE__H__
#define __I_XML_ARCHIVE__H__

#include "Serialization/IArchive.h"
#include "CryExtension/ICryUnknown.h"
#include "CryExtension/CryCreateClassInstance.h"

namespace Serialization
{
	struct IXmlArchive
		: public ICryUnknown
		, public IArchive
	{
	public:
		CRYINTERFACE_DECLARE( IXmlArchive, 0x1386c94ded174f96, 0xab14d20e1b616588 );

		using IArchive::operator();

		virtual void SetXmlNode( XmlNodeRef pRootNode ) = 0;
		virtual XmlNodeRef GetXmlNode() const = 0;

	protected:
		IXmlArchive( int caps ) : IArchive( caps | IArchive::NO_EMPTY_NAMES ) {}
	};


	typedef boost::shared_ptr< IXmlArchive > IXmlArchivePtr;


	inline IXmlArchivePtr CreateXmlInputArchive()
	{
		IXmlArchivePtr pArchive;
		CryCreateClassInstance( "CXmlIArchive", pArchive );
		return pArchive;
	}


	inline IXmlArchivePtr CreateXmlInputArchive( XmlNodeRef pXmlNode )
	{
		if ( pXmlNode )
		{
			IXmlArchivePtr pArchive = CreateXmlInputArchive();
			if ( pArchive )
			{
				pArchive->SetXmlNode( pXmlNode );
			}
			return pArchive;
		}
		return IXmlArchivePtr();
	}


	inline IXmlArchivePtr CreateXmlInputArchive( const char* const filename )
	{
		XmlNodeRef pXmlNode  = gEnv->pSystem->LoadXmlFromFile( filename );
		return CreateXmlInputArchive( pXmlNode );
	}


	inline IXmlArchivePtr CreateXmlOutputArchive()
	{
		IXmlArchivePtr pArchive;
		CryCreateClassInstance( "CXmlOArchive", pArchive );
		return pArchive;
	}


	inline IXmlArchivePtr CreateXmlOutputArchive( XmlNodeRef pXmlNode )
	{
		if ( pXmlNode )
		{
			IXmlArchivePtr pArchive = CreateXmlOutputArchive();
			if ( pArchive )
			{
				pArchive->SetXmlNode( pXmlNode );
			}
			return pArchive;
		}
		return IXmlArchivePtr();
	}


	inline IXmlArchivePtr CreateXmlOutputArchive( const char* const xmlRootElementName )
	{
		XmlNodeRef pXmlNode = gEnv->pSystem->CreateXmlNode( xmlRootElementName );
		return CreateXmlOutputArchive( pXmlNode );
	}


	template< typename T >
	bool StructFromXml( const char* const filename, T& dataOut )
	{
		Serialization::IXmlArchivePtr pXmlArchive = Serialization::CreateXmlInputArchive( filename );
		if ( pXmlArchive )
		{
			Serialization::SStruct serializer = Serialization::SStruct( dataOut );
			const bool success = serializer( *pXmlArchive );
			return success;
		}
		return false;
	}


	template< typename T >
	bool StructFromXml( XmlNodeRef pXmlNode, T& dataOut )
	{
		Serialization::IXmlArchivePtr pXmlArchive = Serialization::CreateXmlInputArchive( pXmlNode );
		if ( pXmlArchive )
		{
			Serialization::SStruct serializer = Serialization::SStruct( dataOut );
			const bool success = serializer( *pXmlArchive );
			return success;
		}
		return false;
	}


	template< typename T >
	XmlNodeRef StructToXml( const char* const xmlRootElementName, const T& dataIn )
	{
		Serialization::IXmlArchivePtr pXmlArchive = Serialization::CreateXmlOutputArchive( xmlRootElementName );
		if ( pXmlArchive )
		{
			Serialization::SStruct serializer = Serialization::SStruct( const_cast< T& >( dataIn ) );
			const bool success = serializer( *pXmlArchive );
			if ( success )
			{
				return pXmlArchive->GetXmlNode();
			}
		}
		return XmlNodeRef();
	}


	template< typename T >
	bool StructToXml( XmlNodeRef pXmlNode, const T& dataIn )
	{
		Serialization::IXmlArchivePtr pXmlArchive = Serialization::CreateXmlOutputArchive( pXmlNode );
		if ( pXmlArchive )
		{
			Serialization::SStruct serializer = Serialization::SStruct( const_cast< T& >( dataIn ) );
			const bool success = serializer( *pXmlArchive );
			return success;
		}
		return false;
	}


	template< typename T >
	bool StructToXml( const char* const filename, const char* const xmlRootElementName, const T& dataIn )
	{
		XmlNodeRef pXmlNode = Serialization::StructToXml( xmlRootElementName, dataIn );
		if ( pXmlNode )
		{
			return pXmlNode->saveToFile( filename );
		}
		return false;
	}
}

#endif