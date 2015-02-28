
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   AreaProxy.h
//  Version:     v1.00
//  Created:     27/9/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AreaProxy_h__
#define __AreaProxy_h__
#pragma once

#include "Area.h"

// forward declarations.
struct SEntityEvent;

//////////////////////////////////////////////////////////////////////////
// Description:
//    Handles sounds in the entity.
//////////////////////////////////////////////////////////////////////////
struct CAreaProxy : public IEntityAreaProxy
{
public:
	static void ResetTempState();

public:
	CAreaProxy();
	virtual ~CAreaProxy();
	CEntity* GetEntity() const { return m_pEntity; };

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Initialize( const SComponentInitializer& init );
	virtual	void ProcessEvent( SEntityEvent &event );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual EEntityProxy GetType() { return ENTITY_PROXY_AREA; }
	virtual void Release();
	virtual void Done() {};
	virtual	void Update( SEntityUpdateContext &ctx ) {}
	virtual bool Init( IEntity *pEntity,SEntitySpawnParams &params ) { return true; }
	virtual void Reload( IEntity *pEntity,SEntitySpawnParams &params );
	virtual void SerializeXML( XmlNodeRef &entityNode,bool bLoading );
	virtual void Serialize( TSerialize ser );
	virtual bool NeedSerialize() { return false; };
	virtual bool GetSignature( TSerialize signature );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityAreaProxy interface.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetFlags( int nAreaProxyFlags ) { m_nFlags = nAreaProxyFlags; };
	VIRTUAL int  GetFlags() { return m_nFlags; } 

	VIRTUAL EEntityAreaType GetAreaType() const { return m_pArea->GetAreaType(); };

	VIRTUAL void	SetPoints( const Vec3* const vPoints, const bool* const pabSoundObstructionSegments, int const nPointsCount, float const fHeight );
	VIRTUAL void	SetBox( const Vec3& min,const Vec3& max, const bool* const pabSoundObstructionSides, size_t const nSideCount );
	VIRTUAL void	SetSphere( const Vec3& center,float fRadius );

	VIRTUAL void	BeginSettingSolid( const Matrix34& worldTM );
	VIRTUAL void	AddConvexHullToSolid( const Vec3* verticesOfConvexHull, bool bObstruction, int numberOfVertices );
	VIRTUAL void	EndSettingSolid();
	
	VIRTUAL int   GetPointsCount() { return m_localPoints.size(); };
	VIRTUAL const Vec3* GetPoints();
	VIRTUAL float GetHeight() { return m_pArea->GetHeight(); };
	VIRTUAL void	GetBox( Vec3& min,Vec3& max ) { m_pArea->GetBox(min,max); }
	VIRTUAL void	GetSphere( Vec3& vCenter,float &fRadius ) { m_pArea->GetSphere(vCenter,fRadius); };

	VIRTUAL void SetGravityVolume(const Vec3 * pPoints, int nNumPoints, float fRadius, float fGravity, bool bDontDisableInvisible, float fFalloff, float fDamping);

	VIRTUAL void	SetID( const int id ) { m_pArea->SetID(id); };
	VIRTUAL int		GetID() const  { return m_pArea->GetID(); };
	VIRTUAL void	SetGroup( const int id) { m_pArea->SetGroup(id); };
	VIRTUAL int		GetGroup( ) const { return m_pArea->GetGroup(); };
	VIRTUAL void	SetPriority( const int nPriority) { m_pArea->SetPriority(nPriority); };
	VIRTUAL int		GetPriority( ) const { return m_pArea->GetPriority(); };
	
	VIRTUAL void	SetSoundObstructionOnAreaFace( int unsigned const nFaceIndex, bool const bObstructs ) { m_pArea->SetSoundObstructionOnAreaFace(nFaceIndex, bObstructs); }

	VIRTUAL void	AddEntity( EntityId id ) { m_pArea->AddEntity(id); };
	VIRTUAL void	AddEntity( EntityGUID guid ) { m_pArea->AddEntity(guid); }
	VIRTUAL void	ClearEntities() { m_pArea->ClearEntities(); };

	VIRTUAL void	SetProximity( float prx ) { m_pArea->SetProximity(prx); };
	VIRTUAL float	GetProximity() { return m_pArea->GetProximity(); };

	VIRTUAL float ClosestPointOnHullDistSq(EntityId const nEntityID, Vec3 const& Point3d, Vec3& OnHull3d) { return m_pArea->ClosestPointOnHullDistSq(nEntityID, Point3d, OnHull3d, false); };
	VIRTUAL float CalcPointNearDistSq(EntityId const nEntityID, Vec3 const& Point3d, Vec3& OnHull3d) { return m_pArea->CalcPointNearDistSq(nEntityID, Point3d, OnHull3d, false); };
	VIRTUAL bool	CalcPointWithin(EntityId const nEntityID, Vec3 const& Point3d, bool const bIgnoreHeight = false) const { return m_pArea->CalcPointWithin(nEntityID, Point3d, bIgnoreHeight); }
	
	virtual void GetMemoryUsage(ICrySizer *pSizer )const
	{
		SIZER_COMPONENT_NAME(pSizer,"CAreaProxy");
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddContainer(m_localPoints);		
		pSizer->AddContainer(m_bezierPoints);	
		pSizer->AddContainer(m_bezierPointsTmp);
		if( m_pArea )
			m_pArea->GetMemoryUsage(pSizer);
	}
private:
	void OnMove();
	void Reset();

	void OnEnable(bool bIsEnable, bool bIsCallScript = true);

	void ReadPolygonsForAreaSolid( CCryFile& file, int numberOfPolygons, bool bObstruction );

private:
	static std::vector<Vec3> s_tmpWorldPoints;

private:
	//////////////////////////////////////////////////////////////////////////
	// Private member variables.
	//////////////////////////////////////////////////////////////////////////
	// Host entity.
	CEntity *m_pEntity;

	int m_nFlags;

	typedef std::vector<bool>									tSoundObstruction;
	typedef tSoundObstruction::const_iterator	tSoundObstructionIterConst;

	// Managed area.
	CArea *m_pArea;
	std::vector<Vec3>		m_localPoints;
	tSoundObstruction		m_abObstructSound; // Each bool flag per point defines if segment to next point is obstructed
	Vec3 m_vCenter;
	float m_fRadius;
	float m_fGravity;
	float m_fFalloff;
	float m_fDamping;
	float m_bDontDisableInvisible;

	pe_params_area m_gravityParams;

	std::vector<Vec3> m_bezierPoints;
	std::vector<Vec3> m_bezierPointsTmp;
	SEntityPhysicalizeParams::AreaDefinition m_areaDefinition;
	bool m_bIsEnable;
	bool m_bIsEnableInternal;
	float m_lastFrameTime;
};

#endif //__AreaProxy_h__
