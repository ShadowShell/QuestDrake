#include "StdAfx.h"
#include "HUDUtils.h"
#include "UI/UIManager.h"
#include "UI/UICVars.h"
#include "UI/Utils/ScreenLayoutManager.h"

#include "Utility/StringUtils.h"
#include "StringUtils.h"
#include "IFlashPlayer.h"

#include "GameRules.h"
#include "GameRulesModules/IGameRulesRoundsModule.h"

#include "Network/Lobby/GameLobby.h"
#include "Network/Squad/SquadManager.h"

#include "Player.h"


static const float MIN_ASPECT_RATIO = (16.0f / 9.0f);

namespace CHUDUtils
{
////////////////////////////////////////////////////////////////////////
static const ColorF s_hUDColor(0.6015625f, 0.83203125f, 0.71484375f, 1.0f);

static const char* g_subtitleCharacters[] =
{
	"PSYCHO",
	"RASCH",
	"PROPHET",
	"CLAIRE",
	"NAX",
	"HIVEMIND",
	"SUIT_VOICE",
};

static const size_t g_subtitleCharactersCount = (sizeof(g_subtitleCharacters) / sizeof(g_subtitleCharacters[0]) );

static const char* g_subtitleColors[] =
{
	"92D050", //PSYCHO
	"00B0F0", //RASCH
	"8F68AC", //PROPHET
	"FFCCFF", //CLAIRE
	"F79646", //NAX
	"FF0000", //HIVEMIND
	"FFFF00", //SUIT_VOICE
};


const size_t GetNumSubtitleCharacters()
{
	return g_subtitleCharactersCount;
}

const char* GetSubtitleCharacter(const size_t index)
{
	assert(index>=0 && index<g_subtitleCharactersCount);
	return g_subtitleCharacters[index];
}

const char* GetSubtitleColor(const size_t index)
{
	assert(index>=0 && index<g_subtitleCharactersCount);
	return g_subtitleColors[index];
}



//////////////////////////////////////////////////////////////////////////
void LocalizeStringW( wstring &out, const char *text, const char *arg1, const char *arg2, const char *arg3, const char *arg4)
{
#if ENABLE_HUD_EXTRA_DEBUG
	const int numberOfWs = g_pGame->GetHUD()->GetCVars()->hud_localize_ws_instead;
	if( numberOfWs>0 )
	{
		static int lastNumberOfWs=0;
		if( lastNumberOfWs!=numberOfWs )
		{
			for(int i=0; i<numberOfWs; i++)
			{
				out.append(L"W");
			}

			lastNumberOfWs = numberOfWs;
		}
		return;
	}
#endif

	if(!text)
	{
		out = L"";
		return;
	}

	wstring localizedString, param1, param2, param3, param4;
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();

	if(text[0]=='@')
		pLocMgr->LocalizeString(text, localizedString);
	else
		StrToWstr(text,localizedString);

	if(arg1)
	{
		if(arg1[0]=='@')
			pLocMgr->LocalizeString(arg1, param1);
		else
			StrToWstr(arg1,param1);
	}

	if(arg2)
	{
		if(arg2[0]=='@')
			pLocMgr->LocalizeString(arg2, param2);
		else
			StrToWstr(arg2,param2);
	}

	if(arg3)
	{
		if(arg3[0]=='@')
			pLocMgr->LocalizeString(arg3, param3);
		else
			StrToWstr(arg3,param3);
	}

	if(arg4)
	{
		if(arg4[0]=='@')
			pLocMgr->LocalizeString(arg4, param4);
		else
			StrToWstr(arg4,param4);
	}

	out.resize(0);
	pLocMgr->FormatStringMessage(out, localizedString, param1.empty()?0:param1.c_str(), param2.empty()?0:param2.c_str(), param3.empty()?0:param3.c_str(), param4.empty()?0:param4.c_str());
}
//////////////////////////////////////////////////////////////////////////
const wchar_t * LocalizeStringW( const char *text, const char *arg1, const char *arg2, const char *arg3, const char *arg4 )
{
	ScopedSwitchToGlobalHeap globalHeap;
	static wstring formatted;
	LocalizeStringW( formatted, text, arg1, arg2, arg3, arg4 );

	return formatted.c_str();
}
//////////////////////////////////////////////////////////////////////////
const char * LocalizeString( const char *text, const char *arg1, const char *arg2, const char *arg3, const char *arg4 )
{
	wstring wcharstr;
	LocalizeStringW( wcharstr, text, arg1, arg2, arg3, arg4 );

	ScopedSwitchToGlobalHeap globalHeap;
	static string charstr;
	CryStringUtils::WStrToUTF8(wcharstr, charstr);

	return charstr.c_str();
}
//////////////////////////////////////////////////////////////////////////
void LocalizeString( string &out, const char *text, const char *arg1, const char *arg2, const char *arg3, const char *arg4 )
{
	wstring wcharstr;
	LocalizeStringW( wcharstr, text, arg1, arg2, arg3, arg4 );

	out.resize(0);
	CryStringUtils::WStrToUTF8(wcharstr, out);
}
//////////////////////////////////////////////////////////////////////////
void LocalizeStringWn( wchar_t* dest, size_t bufferLength, const char *text, const char *arg1 /*= NULL*/, const char *arg2 /*= NULL*/, const char *arg3 /*= NULL*/, const char *arg4 /*= NULL*/ )
{
	cry_wstrncpy( dest, LocalizeStringW(text, arg1, arg2, arg3, arg4), bufferLength );
}
//////////////////////////////////////////////////////////////////////////
void LocalizeStringn( char* dest, size_t bufferLength, const char *text, const char *arg1 /*= NULL*/, const char *arg2 /*= NULL*/, const char *arg3 /*= NULL*/, const char *arg4 /*= NULL*/ )
{
	cry_strncpy( dest, LocalizeString(text, arg1, arg2, arg3, arg4), bufferLength );
}
//////////////////////////////////////////////////////////////////////////
const wchar_t* LocalizeNumberW(const int number)
{
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();

	ScopedSwitchToGlobalHeap globalHeap;
	static wstring formatted;
	pLocMgr->LocalizeNumber(number, formatted);

	return formatted.c_str();

}
//////////////////////////////////////////////////////////////////////////
void LocalizeNumberW(wstring& out, const int number)
{
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();
	pLocMgr->LocalizeNumber(number, out);
}
//////////////////////////////////////////////////////////////////////////
const char* LocalizeNumber(const int number)
{
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();

	static wstring wcharstr;
	pLocMgr->LocalizeNumber(number, wcharstr);

	ScopedSwitchToGlobalHeap globalHeap;
	static string charstr;
	CryStringUtils::WStrToUTF8(wcharstr, charstr);

	return charstr.c_str();
}
//////////////////////////////////////////////////////////////////////////
void LocalizeNumber(string& out, const int number)
{
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();

	static wstring wcharstr;
	pLocMgr->LocalizeNumber(number, wcharstr);
	CryStringUtils::WStrToUTF8(wcharstr, out);
}
//////////////////////////////////////////////////////////////////////////
void LocalizeNumberWn( wchar_t* dest, size_t bufferLength, const int number)
{
	cry_wstrncpy( dest, LocalizeNumberW(number), bufferLength);
}
//////////////////////////////////////////////////////////////////////////
void LocalizeNumbern(char* dest, size_t bufferLength, const int number)
{
	cry_strncpy( dest, LocalizeNumber(number), bufferLength);
}
//////////////////////////////////////////////////////////////////////////
const wchar_t* LocalizeNumberW(const float number, int decimals)
{
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();

	ScopedSwitchToGlobalHeap globalHeap;
	static wstring formatted;
	pLocMgr->LocalizeNumber(number, decimals, formatted);

	return formatted.c_str();

}
//////////////////////////////////////////////////////////////////////////
void LocalizeNumberW(wstring& out, const float number, int decimals)
{
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();
	pLocMgr->LocalizeNumber(number, decimals, out);
}
//////////////////////////////////////////////////////////////////////////
const char* LocalizeNumber(const float number, int decimals)
{
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();

	static wstring wcharstr;
	pLocMgr->LocalizeNumber(number, decimals, wcharstr);

	ScopedSwitchToGlobalHeap globalHeap;
	static string charstr;
	CryStringUtils::WStrToUTF8(wcharstr, charstr);

	return charstr.c_str();
}
//////////////////////////////////////////////////////////////////////////
void LocalizeNumber(string& out, const float number, int decimals)
{
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();

	static wstring wcharstr;
	pLocMgr->LocalizeNumber(number, decimals, wcharstr);
	CryStringUtils::WStrToUTF8(wcharstr, out);
}
//////////////////////////////////////////////////////////////////////////
void LocalizeNumberWn( wchar_t* dest, size_t bufferLength, const float number, int decimals)
{
	cry_wstrncpy( dest, LocalizeNumberW(number, decimals), bufferLength);
}
//////////////////////////////////////////////////////////////////////////
void LocalizeNumbern(char* dest, size_t bufferLength, const float number, int decimals)
{
	cry_strncpy( dest, LocalizeNumber(number, decimals), bufferLength);
}
//////////////////////////////////////////////////////////////////////////

void ConvertSecondsToTimerString( const int s, string* in_out_string, const bool stripZeroElements/*=false*/, bool keepMinutes/*=false*/, const char* const hex_colour/*=NULL*/ )
{
	int hours=0, mins=0, secs=0;
	secs = s;
	hours = (int)floor(((float)secs)*(1/60.0f)*(1/60.0f));
	secs -= hours*60*60;
	mins = (int)floor(((float)secs)*(1/60.0f));
	secs -= mins*60;
	string& l_time = (*in_out_string);
	if (stripZeroElements)
	{
		if (hours > 0)
		{
			l_time.Format( "%.2d:%.2d:%.2d", hours, mins, secs );
		}
		else if (mins > 0 || keepMinutes )
		{
			l_time.Format( "%.2d:%.2d", mins, secs );
		}
		else
		{
			l_time.Format( "%.2d", secs );
		}
	}
	else
	{
		l_time.Format( "%.2d:%.2d:%.2d", hours, mins, secs );
	}

	if (hex_colour)
	{
		string formatted_time;
		formatted_time.Format("<FONT color=\"%s\">%s</FONT>", hex_colour, l_time.c_str());
		l_time = formatted_time;
	}
	
	return;
}

////////////////////////////////////////////////////////////////////////

const char* GetFriendlyStateColour( EFriendState friendState )
{
	if (CUICVars* pHUDCvars = g_pGame->GetUI()->GetCVars())
	{
		switch(friendState)
		{
		case eFS_Friendly:
			return pHUDCvars->hud_colour_friend->GetString();
		case eFS_Enemy:
			return pHUDCvars->hud_colour_enemy->GetString();
		case eFS_LocalPlayer:
			return pHUDCvars->hud_colour_localclient->GetString();
		case eFS_Squaddie:
			return pHUDCvars->hud_colour_squaddie->GetString();
		case eFS_Server:
			return pHUDCvars->hud_colour_localclient->GetString();
		};
	}

	return "FFFFFF";
}

const EFriendState GetFriendlyState( const EntityId entityId, CActor* pLocalActor )
{
	if( pLocalActor )
	{
		if (pLocalActor->GetEntityId()==entityId)
		{
			return eFS_LocalPlayer;
		}
		else if( pLocalActor->IsFriendlyEntity( entityId ) )
		{
			if(CGameLobby* pLobby = g_pGame->GetGameLobby())
			{
				if(CSquadManager* pSM = g_pGame->GetSquadManager())
				{
					IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entityId);
					if(pActor)
					{
						uint16 channelId = pActor->GetChannelId();
						CryUserID userId = pLobby->GetUserIDFromChannelID(channelId);
						const bool isSquadMember = pSM->IsSquadMateByUserId( userId );
						if (isSquadMember)
						{
							return eFS_Squaddie;
						}
					}
				}
			}

			return eFS_Friendly;
		}
		else
		{
			return eFS_Enemy;
		}
	}

	return eFS_Unknown;
}

IFlashPlayer* GetFlashPlayerFromMaterial( IMaterial* pMaterial, bool bGetTemporary )
{
	IFlashPlayer* pRetFlashPlayer(NULL);
	const SShaderItem& shaderItem(pMaterial->GetShaderItem());
	if (shaderItem.m_pShaderResources)
	{
		SEfResTexture* pTex = shaderItem.m_pShaderResources->GetTexture(0);
		if (pTex)
		{
			IDynTextureSource* pDynTexSrc = pTex->m_Sampler.m_pDynTexSource;
			if (pDynTexSrc)
			{
				if (bGetTemporary)
				{
					pRetFlashPlayer = (IFlashPlayer*) pDynTexSrc->GetSourceTemp(IDynTextureSource::DTS_I_FLASHPLAYER);
				}
				else
				{
					pRetFlashPlayer = (IFlashPlayer*) pDynTexSrc->GetSourcePerm(IDynTextureSource::DTS_I_FLASHPLAYER);
				}
			}
		}
	}

	return pRetFlashPlayer;
}

// assume the cgf is loaded at slot 0
// assume that each cgf has only one dynamic flash material
// assume that the dynamic texture is texture 0 on teh shader
IFlashPlayer* GetFlashPlayerFromCgfMaterial( IEntity* pCgfEntity, bool bGetTemporary/*, const char* materialName*/ )
{
	IFlashPlayer* pRetFlashPlayer(NULL);

	// First get the Material from slot 0 on the entity
	assert(pCgfEntity); // should never be NULL

	//IMaterial* pSrcMat = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial( materialName, false );

	IStatObj* pStatObj = pCgfEntity->GetStatObj(0);	

	//const int subObjCount = pStatObj->GetSubObjectCount();

	//for( int subObjId=0; subObjId<subObjCount; subObjId++ )
	{
		//IStatObj::SSubObject* pSubObject = pStatObj->GetSubObject( subObjId );
		//IStatObj* pSubStatObj = pSubObject->pStatObj;

		IMaterial* pMaterial = pStatObj->GetMaterial();
		if (!pMaterial)
		{
			GameWarning( "HUD: Static object '%s' does not have a material!", pStatObj->GetGeoName() );
			return NULL;
		}

		pRetFlashPlayer = CHUDUtils::GetFlashPlayerFromMaterialIncludingSubMaterials(pMaterial, bGetTemporary);
	}
	return pRetFlashPlayer;
}


IFlashPlayer* GetFlashPlayerFromMaterialIncludingSubMaterials( IMaterial* pMaterial, bool bGetTemporary )
{
	IFlashPlayer* pRetFlashPlayer = CHUDUtils::GetFlashPlayerFromMaterial( pMaterial, bGetTemporary );

	const int subMtlCount = pMaterial->GetSubMtlCount();
	for (int i = 0; i != subMtlCount; ++i)
	{
		IMaterial* pSubMat = pMaterial->GetSubMtl(i);
		if (!pSubMat)
		{
			GameWarning( "HUD: Failed to get unified asset 3D submaterial #%d.", i );
			continue;
		}

		IFlashPlayer* pFlashPlayer = CHUDUtils::GetFlashPlayerFromMaterial( pSubMat, bGetTemporary );
		if(pFlashPlayer)
		{
			if(pRetFlashPlayer)
			{
				GameWarning( "HUD: Multiple flash assets in texture!");
			}

			pRetFlashPlayer = pFlashPlayer;
		}
	}

	return pRetFlashPlayer;
}

void UpdateFlashPlayerViewport(IFlashPlayer* pFlashPlayer, int iWidth, int iHeight)
{
	if (pFlashPlayer)
	{
		const float assetWidth = (float)pFlashPlayer->GetWidth();
		const float assetHeight = (float)pFlashPlayer->GetHeight();

		const float renderWidth = (float)iWidth;
		const float renderHeight = (float)iHeight;

		if( (renderWidth * __fres(renderHeight)) > MIN_ASPECT_RATIO )
		{
			const int proposedWidth = int_round( (renderHeight * __fres(assetHeight)) * assetWidth);
			const int offset = int_round(0.5f * (float)(iWidth - proposedWidth));
			pFlashPlayer->SetViewport(offset, 0, proposedWidth, iHeight);
		}
		else
		{
			const int proposedHeight = int_round( (renderWidth / assetWidth) * assetHeight);
			const int offset = int_round(0.5f * (float)(iHeight - proposedHeight));
			pFlashPlayer->SetViewport(0, offset, iWidth, proposedHeight);
		}
	}
}


const ColorF& GetHUDColor()
{
	return s_hUDColor;
}

const float GetIconDepth(const float distance)
{
	// Made into a separate function in case the calculation becomes more complicated at any point
	CUICVars* pCVars = g_pGame->GetUI()->GetCVars();
	return max(distance * pCVars->hud_stereo_icon_depth_multiplier, pCVars->hud_stereo_minDist);
}

int GetBetweenRoundsTimer( int previousTimer )
{
	int roundTime = previousTimer;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		IGameRulesRoundsModule *pRoundsModule = pGameRules->GetRoundsModule();
		if (pRoundsModule)
		{
			if (pRoundsModule->IsRestarting() && (pRoundsModule->GetRoundEndHUDState() == IGameRulesRoundsModule::eREHS_HUDMessage))
			{
				if (pRoundsModule->GetPreviousRoundWinReason() == EGOR_TimeLimitReached)
				{
					// Round finished by the time limit being hit, force the timer to 0 (may not have reached 0 on clients yet due to network lag)
					roundTime = 0;
				}
			}
			else
			{
				roundTime = static_cast<int>(60.0f*pGameRules->GetTimeLimit());
			}
		}
	}

	return roundTime;
}

bool IsSensorPackageActive()
{
	return false;
}

static CHUDUtils::TCenterSortArray m_helperArray;
TCenterSortArray& GetCenterSortHelper()
{
	return m_helperArray;
}

void* GetNearestToCenter()
{
	return GetNearestToCenter(20.0f);
}

void* GetNearestToCenter(const float maxValidDistance)
{
	return GetNearestTo(Vec2(50.0f, 50.0f), maxValidDistance);
}

void* GetNearestTo(const Vec2& center, const float maxValidDistance)
{
	return GetNearestTo(GetCenterSortHelper(), center, maxValidDistance);
}

void* GetNearestTo(const TCenterSortArray& array, const Vec2& center, const float maxValidDistance)
{
	int nearest = -1;
	float nearestDistSq = sqr(maxValidDistance*1.1f);

	Vec2 renderSize = Vec2(800.0f, 600.0f);

	ScreenLayoutManager* pLayoutMgr = g_pGame->GetUI()->GetLayoutManager();
	if(pLayoutMgr)
	{
		renderSize.x = pLayoutMgr->GetRenderWidth();
		renderSize.y = pLayoutMgr->GetRenderHeight();
	}
	else if(gEnv->pRenderer)
	{
		renderSize.x = (float)gEnv->pRenderer->GetWidth();
		renderSize.y = (float)gEnv->pRenderer->GetHeight();
	}

	float xCompression = 1.0f;
	if(renderSize.y>0.0f)
	{
		xCompression = renderSize.x / renderSize.y;
	}

	const int numPoints = array.size();
	for(int i=0; i<numPoints; ++i)
	{
		const SCenterSortPoint& point = array[i];

		Vec2 dir = (point.m_screenPos - center);
		dir.x *= xCompression;

		const float distanceSq = dir.GetLength2();

		if(distanceSq>nearestDistSq)
		{
			continue;
		}

		nearest = i;
		nearestDistSq = distanceSq;
	}

	if(numPoints>0 && nearest<0)
	{
		int a=1;
	}

	if(nearest>=0)
	{
		return array[nearest].m_pData;
	}
	return NULL;
}

}
