/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 
	Silhouettes manager

-------------------------------------------------------------------------
History:
- 20:07:2007: Created by Julien Darre

*************************************************************************/
#ifndef __HUDSILHOUETTES_H__
#define __HUDSILHOUETTES_H__

//-----------------------------------------------------------------------------------------------------

struct IItem;
struct IVehicle;
struct IActor;

class CHUDSilhouettes
{

private:

	struct SDebugInfiltrationInfo
	{
		SDebugInfiltrationInfo() : bEnabled(false) {};

		bool bEnabled;
		float r, g, b, a;
		float fTime;
	};

	struct SDebugCombatInfo
	{
		SDebugCombatInfo(): bEnabled(false) {};

		bool bEnabled;
		float fTime;
	};

	struct SSilhouette
	{
		EntityId uiEntityId;
		float fTime;
		bool bValid;
		float r, g, b, a;
		SDebugInfiltrationInfo debugInfiltrationInfo;
		SDebugCombatInfo debugCombatInfo;

		SSilhouette() : bValid(false)
		{
		}

		void Serialize(TSerialize &ser)
		{
			ser.Value("uiEntityId", uiEntityId);
			ser.Value("fTime", fTime);
			ser.Value("bValid", bValid);
			ser.Value("r", r);
			ser.Value("g", g);
			ser.Value("b", b);
			ser.Value("a", a);
		}

		void GetMemoryUsage(ICrySizer *pSizer)const {}
	};

	typedef std::vector<SSilhouette> TSilhouettesVector;

public:

	CHUDSilhouettes();
	virtual ~CHUDSilhouettes();

	void Fill();
	void Clear();

	void SetSilhouette(IActor *pActor,		float r,float g,float b,float a,float fDuration,bool bHighlightCurrentItem=true,bool bHighlightAccessories=true);
	void SetSilhouette(IActor *pActor,		float fDuration,bool bHighlightCurrentItem=true,bool bHighlightAccessories=true);

	void SetSilhouette(IItem	*pItem,			float r,float g,float b,float a,float fDuration,bool bHighlightAccessories=true);
	void SetSilhouette(IItem	*pItem,			float fDuration,bool bHighlightAccessories=true);

	void SetSilhouette(IVehicle	*pVehicle,float r,float g,float b,float a,float fDuration);
	void SetSilhouette(IVehicle	*pVehicle,float fDuration);

	void SetSilhouette(IEntity *pEntity,	float r,float g,float b,float a,float fDuration, bool bFlowGraphRequested = false);
	void SetSilhouette(IEntity *pEntity,	float fDuration, bool bFlowGraphRequested = false);

	void SetFlowGraphSilhouette(IEntity *pEntity,	float r,float g,float b,float a,float fDuration);

	void ResetSilhouette(EntityId uiEntityId);
	void ResetFlowGraphSilhouette(EntityId uiEntityId);

	void SetType(int iType);

	void Serialize(TSerialize &ser);

	void Update(float frameTime);

	void GetMemoryUsage(ICrySizer *pSizer) const
	{	
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddContainer(m_silhouettesFGVector);
		pSizer->AddContainer(m_silhouettesVector);		
	}

	void CloakActivated(EntityId uiEntityId);			

private:

	void SetVisionParams(EntityId uiEntityId,float r,float g,float b,float a);
	bool CanUseSilhouettes(bool bFlowGraphRequested = false) const;
	void SetSilhoutteInCombatMode(IEntity* pClientEntity, IEntity* pTargetEntity);
	void SetSilhouetteInInfiltrationMode(IEntity* pClientEntity, IEntity* pTargetEntity);
	void SetSilhouetteInTacticalMode(IEntity* pClientEntity, IEntity* pTargetEntity);
	
	void SetDebugInfiltrationInfo(IEntity *pEntity, const SDebugInfiltrationInfo& debugIconInfo);
	void SetDebugCombatInfo(IEntity *pEntity, const SDebugCombatInfo& debugCombatInfo);

	void DrawDebugInfiltrationInfo(const SSilhouette& silhoutte);
	void DrawDebugCombatInfo(const SSilhouette& silhoutte);

	void HeatVisionActivated();

	std::map<EntityId, Vec3>::iterator GetFGSilhouette(EntityId id);


	TSilhouettesVector m_silhouettesVector;
	std::map<EntityId, Vec3> m_silhouettesFGVector;

	bool	m_heatVisionActive;

	static bool s_hasNotYetWarnedAboutVectorSize;

};

#endif

//-----------------------------------------------------------------------------------------------------
