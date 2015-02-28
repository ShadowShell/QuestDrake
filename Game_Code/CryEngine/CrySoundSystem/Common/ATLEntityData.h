// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef ATLENTITYDATA_H_INCLUDED
#define ATLENTITYDATA_H_INCLUDED
//////////////////////////////////////////////////////////////////////////
// Description:
//			This file defines the data-types used in the IAudioSystemImplementation.h
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// <title IATLAudioObjectData>
// Summary:
//			An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
//			data needed for identifying and using the corresponding ATLAudioObject (e.g. a middleware-specific unique ID)
//////////////////////////////////////////////////////////////////////////
struct IATLAudioObjectData
{
	virtual ~IATLAudioObjectData() {}
};

//////////////////////////////////////////////////////////////////////////
// <title IATLListenerData>
// Summary:
//			An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
//			data needed for identifying and using the corresponding ATLListenerObject (e.g. a middleware-specific unique ID)
//////////////////////////////////////////////////////////////////////////
struct IATLListenerData
{
	virtual ~IATLListenerData() {}
};

//////////////////////////////////////////////////////////////////////////
// <title IATLTriggerImplData>
// Summary::
//			An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
//			data needed for identifying and using the corresponding ATLTriggerImpl 
//			(e.g. a middleware-specific event ID or name, a sound-file name to be passed to an API function)
//////////////////////////////////////////////////////////////////////////
struct IATLTriggerImplData
{
	virtual ~IATLTriggerImplData() {}
};

//////////////////////////////////////////////////////////////////////////
// <title IATLRtpcImplData>
// Summary:
//			An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
//			data needed for identifying and using the corresponding ATLRtpcImpl
//			(e.g. a middleware-specific control ID or a parameter name to be passed to an API function)
//////////////////////////////////////////////////////////////////////////
struct IATLRtpcImplData
{
	virtual ~IATLRtpcImplData() {}
};

//////////////////////////////////////////////////////////////////////////
// <title IATLSwitchStateImplData>
// Summary:
//			An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
//			data needed for identifying and using the corresponding ATLSwitchStateImpl
//			(e.g. a middleware-specific control ID or a switch and state names to be passed to an API function)
//////////////////////////////////////////////////////////////////////////
struct IATLSwitchStateImplData
{
	virtual ~IATLSwitchStateImplData() {}
};

//////////////////////////////////////////////////////////////////////////
// <title IATLEnvironmentImplData>
// Summary:
//			An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
//			data needed for identifying and using the corresponding ATLEnvironmentImpl
//			(e.g. a middleware-specific bus ID or name to be passed to an API function)
//////////////////////////////////////////////////////////////////////////
struct IATLEnvironmentImplData
{
	virtual ~IATLEnvironmentImplData() {}
};

//////////////////////////////////////////////////////////////////////////
// <title IATLEventData>
// Summary:
//			An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
//			data needed for identifying and using the corresponding ATLEvent
//			(e.g. a middleware-specific playingID of an active event/sound for a play event)
//////////////////////////////////////////////////////////////////////////
struct IATLEventData
{
	virtual ~IATLEventData() {}
};

//////////////////////////////////////////////////////////////////////////
// <title IATLAudioFileEntryData>
// Summary:
//			An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
//			data needed for identifying and using the corresponding ATLAudioFileEntry.
//			(e.g. a middleware-specific bank ID if the AudioFileEntry represents a soundbank)
//////////////////////////////////////////////////////////////////////////
struct IATLAudioFileEntryData
{
	virtual ~IATLAudioFileEntryData() {}
};

//////////////////////////////////////////////////////////////////////////
// <title SATLAudioFileEntryInfo>
// Summary:
//			This is a POD structure used to pass the information about a file preloaded into memory between 
//			the CryAudioSystem and an AudioSystemImplementation
//			Note: This struct cannot define a constructor, it needs to be a POD!
//////////////////////////////////////////////////////////////////////////
struct SATLAudioFileEntryInfo
{
	void*										pFileData;						// pointer to the memory location of the file's contents
	size_t									nMemoryBlockAlignment;// memory alignment to be used for storing this file's contents in memory
	size_t									nSize;								// file size
	char const*							sFileName;						// file name
	bool										bLocalized;						// is the file localized
	IATLAudioFileEntryData*	pImplData;						// pointer to the implementation-specific data needed for this AudioFileEntry
};

//////////////////////////////////////////////////////////////////////////
// <title SAudioImplMemoryInfo>
// Summary:
//			This is a POD structure used to pass the information about an AudioSystemImplementation's memory usage
//			Note: This struct cannot define a constructor, it needs to be a POD!
//////////////////////////////////////////////////////////////////////////
struct SAudioImplMemoryInfo
{
	size_t nPrimaryPoolSize;					// total size in bytes of the Primary Memory Pool used by an AudioSystemImplementation
	size_t nPrimaryPoolUsedSize;			// bytes allocated inside the Primary Memory Pool used by an AudioSystemImplementation
	size_t nPrimaryPoolAllocations;		// number of allocations performed in the Primary Memory Pool used by an AudioSystemImplementation
	size_t nSecondaryPoolSize;				// total size in bytes of the Secondary Memory Pool used by an AudioSystemImplementation
	size_t nSecondaryPoolUsedSize;		// bytes allocated inside the Secondary Memory Pool used by an AudioSystemImplementation
	size_t nSecondaryPoolAllocations;	// number of allocations performed in the Secondary Memory Pool used by an AudioSystemImplementation
	size_t nBucketUsedSize;						// bytes allocated by the Bucket allocator (used for small object allocations)
	size_t nBucketAllocations;				// total number of allocations by the Bucket allocator (used for small object allocations)
};

#endif // ATLENTITYDATA_H_INCLUDED
