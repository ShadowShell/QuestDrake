// ScreenEffects - Allows for simultaneous and queued blending of effects
//  John Newfield
// 
//	23-1-2008: Refactored by Benito G.R.

#include "StdAfx.h"
#include "Actor.h"
#include "Player.h"
#include "IViewSystem.h"
#include "ScreenEffects.h"
#include "GameCVars.h"

#include "BlendTypes.h"
#include "BlendedEffect.h"
#include "BlendNode.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

//---------------------------------
CScreenEffects::CScreenEffects() : 
m_curUniqueID(0), 
m_enableBlends(true), 
m_updatecoords(false)
{

}

//---------------------------------
CScreenEffects::~CScreenEffects(void)
{
	ClearAllBlendGroups(false);
}

//---------------------------------
void CScreenEffects::Reset()
{
	ClearAllBlendGroups(false);
	ResetGameEffectPools();

	stl::free_container(m_coordsXname);
	stl::free_container(m_coordsYname);
}
	
//---------------------------------
void CScreenEffects::Update(float frameTime)
{
	if (!m_enableBlends)
		return;
	std::map<int, CBlendGroup*>::iterator it = m_blends.begin();
	while (it != m_blends.end())
	{
		CBlendGroup *curGroup = (CBlendGroup *)it->second;
		bool found = m_enabledGroups.find(it->first) != m_enabledGroups.end();
		if (curGroup && (!found || (found && m_enabledGroups[it->first])))
		{
			if (curGroup->HasJobs())
			{
				curGroup->Update(frameTime);
			}
		}
		++it;
	}

}

void CScreenEffects::PostUpdate(float frameTime)
{
	if (m_updatecoords)
	{
		Vec3 screenspace;
		if (gEnv->pRenderer->ProjectToScreen(m_coords3d.x, m_coords3d.y, m_coords3d.z, &screenspace.x, &screenspace.y, &screenspace.z))
		{
			gEnv->p3DEngine->SetPostEffectParam(m_coordsXname, screenspace.x/100.0f);
			gEnv->p3DEngine->SetPostEffectParam(m_coordsYname, screenspace.y/100.0f);
		}
	}
}

//---------------------------------
int CScreenEffects::GetUniqueID()
{
	return (eSFX_GID_Last + (m_curUniqueID++));
}

//---------------------------------
void CScreenEffects::StartBlend(IBlendedEffect *effect, IBlendType *blendType, float speed, int blendGroup)
{
	if(!effect || !blendType)
		return;

	bool found = m_enabledGroups.find(blendGroup) != m_enabledGroups.end();
	if (!m_enableBlends || (found && !m_enabledGroups[blendGroup]))
	{
		if (effect)
			effect->Release();
		if (blendType)
			blendType->Release();
		return;
	}

	CBlendGroup *group = 0;
	if (m_blends.count(blendGroup) == 0)
	{
		group = new CBlendGroup();
		m_blends[blendGroup] = group;
	}
	else
	{
		group = m_blends[blendGroup];
	}

	if (group)
		group->AddJob(blendType,effect,speed);

}

//---------------------------------
void CScreenEffects::CamShake(Vec3 rotateShake, Vec3 shiftShake, float freq, float shakeTime, float randomness, int shakeID)
{
	if (g_pGameCVars && g_pGameCVars->g_detachCamera==0)
	{
		IView *view = gEnv->pGame->GetIGameFramework()->GetIViewSystem()->GetActiveView();
		if (view)
		{
			CCCPOINT(Camera_DoShake);
			view->SetViewShake(Ang3(rotateShake), shiftShake, shakeTime, freq, randomness, shakeID, false);
		}
	}
}

//---------------------------------
bool CScreenEffects::HasJobs(int blendGroup)
{
	if (m_blends.count(blendGroup) == 0)
	{
		return false;
	}
	else
	{
		CBlendGroup *group = m_blends[blendGroup];
		if (group)
			return (group->HasJobs());
	}
	return false;
	
}

//---------------------------------
void CScreenEffects::ClearBlendGroup(int blendGroup, bool resetScreen)
{
	if (m_blends.count(blendGroup) != 0)
	{
		CBlendGroup *group = m_blends[blendGroup];
		if (group)
		{
			delete group;
			m_blends.erase(m_blends.find(blendGroup));
		}
	}
	if (resetScreen)
		ResetScreen();

}

//---------------------------------
void CScreenEffects::ClearAllBlendGroups(bool resetScreen)
{
	std::map<int, CBlendGroup* >::iterator it = m_blends.begin();
	while (it != m_blends.end())
	{
		CBlendGroup *cur = (CBlendGroup *)it->second;
		if (cur)
		{
			delete cur;
		}
		++it;
	}
	m_blends.clear();
	m_enabledGroups.clear();

	if (resetScreen)
		ResetScreen();
}

//---------------------------------
void CScreenEffects::ResetBlendGroup(int blendGroup, bool resetScreen)
{
	if (m_blends.count(blendGroup) != 0)
	{
		CBlendGroup *group = m_blends[blendGroup];
		if (group)
		{
			group->Reset();
		}
	}
	if (resetScreen)
		ResetScreen();
}

//---------------------------------
void CScreenEffects::ResetAllBlendGroups(bool resetScreen)
{
	std::map<int, CBlendGroup* >::iterator it = m_blends.begin();
	while (it != m_blends.end())
	{
		CBlendGroup *cur = (CBlendGroup *)it->second;
		if (cur)
		{
			cur->Reset();
		}
		++it;
	}

	if (resetScreen)
		ResetScreen();
}

//---------------------------------
void CScreenEffects::ResetScreen()
{
	
	gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Amount", 0.0f);
	gEnv->p3DEngine->SetPostEffectParam("Global_Saturation", 1.0f);
	gEnv->p3DEngine->SetPostEffectParam("Global_Brightness", 1.0f);
	gEnv->p3DEngine->SetPostEffectParam("Global_Contrast", 1.0f);
	gEnv->p3DEngine->SetPostEffectParam("Global_ColorC", 0.0f);
	gEnv->p3DEngine->SetPostEffectParam("Global_ColorM", 0.0f);
	gEnv->p3DEngine->SetPostEffectParam("Global_ColorY", 0.0f);
	gEnv->p3DEngine->SetPostEffectParam("Global_ColorK", 0.0f);
	gEnv->p3DEngine->SetPostEffectParam("BloodSplats_Active", false);
	gEnv->p3DEngine->SetPostEffectParam("BloodSplats_Type", false);
	gEnv->p3DEngine->SetPostEffectParam("BloodSplats_Amount", 0.0f);
	gEnv->p3DEngine->SetPostEffectParam("MotionBlur_VectorsScale", g_pGameCVars->cl_motionBlurVectorScale);
	gEnv->p3DEngine->SetPostEffectParam("FilterChromaShift_User_Amount", 0.0f);
	gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("FilterSharpening_Amount", 1.0f);
	gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("FilterSharpening_Type", 0.0f);
}

void CScreenEffects::SetUpdateCoords(const char *coordsXname, const char *coordsYname, Vec3 pos)
{
	m_coordsXname = coordsXname;
	m_coordsYname = coordsYname;
	m_coords3d = pos;
	m_updatecoords = true;
}

void CScreenEffects::EnableBlends(bool enable, int blendGroup)
{
	m_enabledGroups[blendGroup] = enable;
}

//--------------------------------------------------------------
void CScreenEffects::GetMemoryStatistics(ICrySizer * s)
{
	s->AddContainer(m_blends);
	s->AddContainer(m_enabledGroups);	
}



