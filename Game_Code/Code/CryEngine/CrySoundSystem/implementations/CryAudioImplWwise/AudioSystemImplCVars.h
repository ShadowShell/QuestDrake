// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef AUDIO_WWISE_IMPL_CVARS_H_INCLUDED
#define AUDIO_WWISE_IMPL_CVARS_H_INCLUDED

class CAudioWwiseImplCVars
{
public:

	CAudioWwiseImplCVars();
	~CAudioWwiseImplCVars();

	void	RegisterVariables();
	void	UnregisterVariables();

	int		m_nPrimaryPoolSize;
	int		m_nSecondaryPoolSize;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	int		m_nEnableCommSystem;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

private:

	CAudioWwiseImplCVars(CAudioWwiseImplCVars const&);						// Copy protection
	CAudioWwiseImplCVars& operator=(CAudioWwiseImplCVars const&);	// Copy protection
};

extern CAudioWwiseImplCVars g_AudioWwiseImplCVars;

#endif // AUDIO_WWISE_IMPL_CVARS_H_INCLUDED
