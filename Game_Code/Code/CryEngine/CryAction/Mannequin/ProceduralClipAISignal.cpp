////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>
#include "IAIObject.h"
#include "IAIActor.h"


// This procedural clip will send a signal directly to the AI actor interface
// of the entity on which the clip is playing.
class CProceduralClipAISignal : public TProceduralClip<SProceduralParams>
{
public:

	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipAISignal, "AISignal", 0x85871F959F704CE4, 0xBE4A68D18706AFF7)

	virtual void OnEnter(float blendTime, float duration, const SProceduralParams &params)
	{
		// TODO: Temporary work-around: we need to be able to store 2 signal 
		// names because the params.dataCRC string is not available in 
		// release/console builds, and also used a CRC generator that is 
		// incompatible with the one used by the AI signal systems.
		// This work-around uses a separator '|' character, so that we don't 
		// need to add extra string support throughout the Mannequin editors and 
		// such.
		CryFixedStringT<64> onEnterSignalName;
		ExtractSignalNames(params.dataString.c_str(), &onEnterSignalName, &m_onExitSignalName);

		IAIActor* aiActor = GetAIActor();
		IF_UNLIKELY (aiActor == NULL)
		{
			return;
		}

		if (!onEnterSignalName.empty())
		{
			aiActor->SetSignal(
				AISIGNAL_DEFAULT, 
				onEnterSignalName.c_str(),
				NULL, // Sender.
				NULL); // No additional data.
		}
	}

	virtual void OnExit(float blendTime)
	{
		if (m_onExitSignalName.empty())
		{
			return;
		}

		IAIActor* aiActor = GetAIActor();
		IF_UNLIKELY (aiActor == NULL)
		{
			return;
		}

		aiActor->SetSignal(
			AISIGNAL_DEFAULT, 
			m_onExitSignalName.c_str(),
			NULL,  // Sender.
			NULL); // No additional data.
	}

	virtual void Update(float timePassed) {}


private:

	IAIActor* GetAIActor() const
	{
		IF_UNLIKELY (m_entity == NULL)
		{
			return NULL;
		}
		IAIObject* aiObject = m_entity->GetAI();
		IF_UNLIKELY (aiObject == NULL)
		{
			return NULL;
		}
		return aiObject->CastToIAIActor();
	}

	
	// TODO: Create proper separate signal names in the procedural clip.
	inline void ExtractSignalNames(const char* dataString, CryFixedStringT<64>* onEnterSignalString, CryFixedStringT<64>* onExitSignalString)
	{
		assert(onEnterSignalString != NULL);
		assert(onExitSignalString != NULL);

		// It is allowed to omit any of the signal names in the data string.
		onEnterSignalString->clear();
		onExitSignalString->clear();

		IF_UNLIKELY (dataString == NULL)
		{
			return;
		}

#if !defined(_RELEASE)
		const char *dataStringStart = dataString;
#endif
		char chr;
		while ( (chr = *dataString) != '\0' )
		{
			if (chr == '|')
			{	// Switch to parsing the exit signal name.
				break;
			}
			*onEnterSignalString += chr;
			dataString++;
		} 
#if !defined(_RELEASE)
		assert( (dataString - dataStringStart) < onExitSignalString->MAX_SIZE);
#endif
		if (chr == '\0')
		{	// No exit signal name specified apparently.
			return;
		}

		dataString++; // Skip the separator character.

#if !defined(_RELEASE)
		assert(strlen(dataString) <= onExitSignalString->MAX_SIZE);
#endif

		*onExitSignalString = dataString;
		return;
	}


private:

	CryFixedStringT<64> m_onExitSignalName;
};

CProceduralClipAISignal::CProceduralClipAISignal()
{
}

CProceduralClipAISignal::~CProceduralClipAISignal()
{
}

CRYREGISTER_CLASS(CProceduralClipAISignal)
