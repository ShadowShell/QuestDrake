/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements the Player swim state

-------------------------------------------------------------------------
History:
- 19.10.10: Created by Stephen M. North
- Initial creation does not derive from CPlayerState, created only to encapsulate
-   the swim code.
- 20.10.10: Creation of explicit Entry conditions.
- Still not a full player state.
- 25.10.10: Changed to a full player state.

*************************************************************************/
#ifndef __PlayerStateSwim_h__
#define __PlayerStateSwim_h__

#include "Audio/AudioSignalPlayer.h"

struct SActorFrameMovementParams;
class CPlayer;
class CPlayerStateSwim_WaterTestProxy;
class CPlayerStateSwim
{
public:
	static void SetParamsFromXml(const IItemParamsNode* pParams) 
	{
		ScopedSwitchToGlobalHeap useGlobalHeap;
		GetSwimParams().SetParamsFromXml( pParams );
	}

	CPlayerStateSwim();

	void OnEnter( CPlayer& player );
	bool OnPrePhysicsUpdate( CPlayer& player, const SActorFrameMovementParams& movement, float frameTime );
	void OnUpdate( CPlayer& player, float frameTime );
	void OnExit( CPlayer& player );

	static void UpdateSoundListener( CPlayer &player );

	bool DetectJump(CPlayer& player, const SActorFrameMovementParams& movement, float frameTime, float* pVerticalSpeedModifier) const;

private:
	
	CAudioSignalPlayer m_enduranceSpeedSwimSound;

	Vec3 m_gravity;
	float m_lastWaterLevel;
	float m_lastWaterLevelTime;
	float m_verticalVelDueToSurfaceMovement; // e.g. waves.
	float m_headUnderWaterTimer;
	bool m_onSurface;
	bool m_enduranceSwimSoundPlaying;

	void PlayEnduranceSound( const EntityId ownerId );
	void StopEnduranceSound( const EntityId ownerId );

	struct CSwimmingParams
	{
		CSwimmingParams() 
			: m_swimSpeedSprintSpeedMul(2.5f)
			, m_swimUpSprintSpeedMul(2.0f)
			, m_swimSprintSpeedMul(1.4f)
			, m_stateSwim_animCameraFactor(0.25f)
			, m_swimDolphinJumpDepth(0.1f)
			,	m_swimDolphinJumpThresholdSpeed(3.0f)
			, m_swimDolphinJumpSpeedModification(0.0f)
		{}
		void SetParamsFromXml(const IItemParamsNode* pParams);

		float m_swimSpeedSprintSpeedMul;
		float m_swimUpSprintSpeedMul;
		float m_swimSprintSpeedMul;
		float m_stateSwim_animCameraFactor;

		float m_swimDolphinJumpDepth;
		float m_swimDolphinJumpThresholdSpeed;
		float m_swimDolphinJumpSpeedModification;
	};

	static CSwimmingParams s_swimParams;
	static CSwimmingParams& GetSwimParams() { return s_swimParams; }

	// DO NOT IMPLEMENT!
	CPlayerStateSwim( const CPlayerStateSwim& );
	CPlayerStateSwim& operator=( const CPlayerStateSwim& );
};

#endif // __PlayerStateSwim_h__
