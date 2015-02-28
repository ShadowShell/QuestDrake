// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef COMMON_WWISE_H_INCLUDED
#define COMMON_WWISE_H_INCLUDED

#include "AK/SoundEngine/Common/AkTypes.h"
#include "AK/AkWwiseSDKVersion.h"
#include <IAudioSystem.h>

#define WWISE_IMPL_BASE_PATH "/sounds/wwise/"
#define WWISE_IMPL_BANK_PATH "" // No further sub folders necessary.
#define WWISE_IMPL_BANK_FULL_PATH WWISE_IMPL_BASE_PATH WWISE_IMPL_BANK_PATH
#define WWISE_IMPL_INFO_STRING "Wwise " AK_WWISESDK_VERSIONNAME

#define ASSERT_WWISE_OK(x) (assert(x == AK_Success))
#define IS_WWISE_OK(x) (x == AK_Success)

// several wwise-specific helper functions
//////////////////////////////////////////////////////////////////////////
inline void FillAKVector(Vec3 const& vCryVector, AkVector& vAKVector)
{
	vAKVector.X = vCryVector.x;
	vAKVector.Y	= vCryVector.z;
	vAKVector.Z = vCryVector.y;
}

///////////////////////////////////////////////////////////////////////////
inline void FillAKObjectPosition(SATLWorldPosition const& oATLPosition, AkSoundPosition& oAKPosition)
{
	FillAKVector(oATLPosition.mPosition.GetColumn3(), oAKPosition.Position);
	FillAKVector(oATLPosition.mPosition.GetColumn1(), oAKPosition.Orientation);
}

///////////////////////////////////////////////////////////////////////////
inline void FillAKListenerPosition(SATLWorldPosition const& oATLPosition, AkListenerPosition& oAkPosition)
{
	FillAKVector(oATLPosition.mPosition.GetColumn3(), oAkPosition.Position);
	FillAKVector(oATLPosition.mPosition.GetColumn1(), oAkPosition.OrientationFront);
	FillAKVector(oATLPosition.mPosition.GetColumn2(), oAkPosition.OrientationTop);
}

#endif // COMMON_WWISE_H_INCLUDED
