#pragma once

#if defined(SERVER_CHECKS)

#ifndef __PLAYERPOSITIONCHECKER_H__
#define __PLAYERPOSITIONCHECKER_H__

class CPlayer;

class CPlayerPositionChecker
{
public:
	CPlayerPositionChecker(void);
	~CPlayerPositionChecker(void);

	void Update(float fFrameTime);

private:
	struct SPlayerRayCast
	{
		int nLastPlayerTested;
		EntityId lastPlayerTested;
		QueuedRayID rayId;
	};

	struct SHeightCheck : public SPlayerRayCast
	{
		float fStartTimeInAir;
		bool	bHighFrequencyChecks;
	} m_HeightCheck;

	void OnHeightCheckRayCastDataReceived( const QueuedRayID& rayID, const RayCastResult& result );
	void CancelRayCastRequest( SPlayerRayCast& rPlayerRayCast );
	void QueueNextPlayerHeightCheckRayCast( );
	void QueueHeightCheckRayCast( EntityId playerId );

	bool ShouldCheckPlayer( CPlayer * pPlayer );
};

#endif //__PLAYERPOSITIONCHECKER_H__

#endif //SERVER_CHECKS