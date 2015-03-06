#ifndef _LETTER_BOX_HUD_EVENT_LISTENER_
#define _LETTER_BOX_HUD_EVENT_LISTENER_

#pragma once

// Includes
#include "UI/HUD/HUDEventDispatcher.h"

//==================================================================================================
// Name: SLetterBoxParams
// Desc: Parameters for CLetterBoxHudEventListener
// Author: James Chilvers
//==================================================================================================
struct SLetterBoxParams
{
	SLetterBoxParams()
	{
		color.set(0.0f,0.0f,0.0f,1.0f);
		scale = 0.11f;
	}

	ColorF color;
	float scale;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: CLetterBoxHudEventListener
// Desc: Renders Letter box bars
// Author: James Chilvers
//==================================================================================================
class CLetterBoxHudEventListener : public IHUDEventListener
{
public:
	CLetterBoxHudEventListener() {}
	SC_API virtual ~CLetterBoxHudEventListener();

	SC_API void Initialise(const SLetterBoxParams* params);

	SC_API void Register();
	SC_API void UnRegister();

	SC_API virtual void OnHUDEvent(const SHUDEvent& event);

private:

	void Draw();

	SLetterBoxParams	m_params;

};//------------------------------------------------------------------------------------------------

#endif // _LETTER_BOX_HUD_EVENT_LISTENER_
