/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: UI draw functions

-------------------------------------------------------------------------
History:
- 07:11:2005: Created by Julien Darre

*************************************************************************/
#include "StdAfx.h"
#include "UIDraw.h"

//-----------------------------------------------------------------------------------------------------

CUIDraw::CUIDraw()
{
	m_pRenderer = gEnv->pRenderer;
}

//-----------------------------------------------------------------------------------------------------

CUIDraw::~CUIDraw()
{
	for(TTexturesMap::iterator iter=m_texturesMap.begin(); iter!=m_texturesMap.end(); ++iter)
	{
		SAFE_RELEASE((*iter).second);
	}
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::Release()
{
	delete this;
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::PreRender()
{
 	m_pRenderer->SetCullMode(R_CULL_DISABLE);
	m_pRenderer->Set2DMode(true,m_pRenderer->GetNativeWidth(),m_pRenderer->GetNativeHeight());
	m_pRenderer->SetColorOp(eCO_MODULATE,eCO_MODULATE,DEF_TEXARG0,DEF_TEXARG0);
	m_pRenderer->SetState(GS_BLSRC_SRCALPHA|GS_BLDST_ONEMINUSSRCALPHA|GS_NODEPTHTEST);
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::PostRender()
{
	m_pRenderer->Set2DMode(false,0,0);
}

//-----------------------------------------------------------------------------------------------------

uint32 CUIDraw::GetColorARGB(uint8 ucAlpha,uint8 ucRed,uint8 ucGreen,uint8 ucBlue)
{
	int iRGB = (m_pRenderer->GetFeatures() & RFT_RGBA);
	return (iRGB ? RGBA8(ucRed,ucGreen,ucBlue,ucAlpha) : RGBA8(ucBlue,ucGreen,ucRed,ucAlpha));
}

//-----------------------------------------------------------------------------------------------------

int CUIDraw::CreateTexture(const char *strName, bool dontRelease)
{
	for(TTexturesMap::iterator iter=m_texturesMap.begin(); iter!=m_texturesMap.end(); ++iter)
	{
		if(0 == strcmpi((*iter).second->GetName(),strName))
		{
			return (*iter).first;
		}
	}
	uint32 flags = FT_NOMIPS|FT_DONT_RESIZE|FT_DONT_STREAM|FT_STATE_CLAMP;
	if (dontRelease)
	{
		GameWarning("Are you sure you want to permanently keep this UI texture '%s'?!", strName);
	}
	
	flags |= dontRelease ? FT_DONT_RELEASE : 0;
	ITexture *pTexture = m_pRenderer->EF_LoadTexture(strName,flags);
	pTexture->SetClamp(true);
	int iTextureID = pTexture->GetTextureID();
	m_texturesMap.insert(std::make_pair(iTextureID,pTexture));
	return iTextureID;
}

//-----------------------------------------------------------------------------------------------------

bool CUIDraw::DeleteTexture(int iTextureID)
{
	TTexturesMap::iterator it = m_texturesMap.find(iTextureID);
	if(it != m_texturesMap.end())
	{
		m_texturesMap.erase(it);
		gEnv->pRenderer->RemoveTexture(iTextureID);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::GetTextureSize(int iTextureID,float &rfSizeX,float &rfSizeY)
{
	TTexturesMap::iterator Iter = m_texturesMap.find(iTextureID);
	if(Iter != m_texturesMap.end())
	{
		ITexture *pTexture = (*Iter).second;
		rfSizeX = (float) pTexture->GetWidth	();
		rfSizeY = (float) pTexture->GetHeight	();
	}
	else
	{
		// Unknow texture !
		CRY_ASSERT(0);
		rfSizeX = 0.0f;
		rfSizeY = 0.0f;
	}
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::DrawLine(float fX1, float fY1, float fX2, float fY2, uint32 uiDiffuse)
{
	SVF_P3F_C4B_T2F aVertices[2];

	const float fOff = -0.5f;

	aVertices[0].color.dcolor = uiDiffuse;
	aVertices[0].xyz = Vec3(fX1+fOff, fY1+fOff, 0.0f);
	aVertices[0].st = Vec2(0, 0);

	aVertices[1].color.dcolor = uiDiffuse;
	aVertices[1].xyz = Vec3(fX2+fOff, fY2+fOff, 0.0f);
	aVertices[1].st = Vec2(1, 1);

/*
	if(iTextureID)
	{
		m_pRenderer->EnableTMU(true);  
		m_pRenderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
		m_pRenderer->SetTexture(iTextureID);
	}
	else
*/	
	{
		//m_pRenderer->EnableTMU(false);
		// m_pRenderer->SetWhiteTexture();
	}

	uint16 ausIndices[] = {0,1};

	m_pRenderer->DrawDynVB(aVertices,ausIndices,2,2,prtLineList);
	
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::DrawTriangle(float fX0,float fY0,float fX1,float fY1,float fX2,float fY2,uint32 uiColor)
{
	SVF_P3F_C4B_T2F aVertices[3];

	const float fOff = -0.5f;

	aVertices[0].color.dcolor = uiColor;
	aVertices[0].xyz = Vec3(fX0+fOff, fY0+fOff, 0.0f);
	aVertices[0].st = Vec2(0, 0);

	aVertices[1].color.dcolor = uiColor;
	aVertices[1].xyz = Vec3(fX1+fOff, fY1+fOff, 0.0f);
	aVertices[1].st = Vec2(0, 0);

	aVertices[2].color.dcolor = uiColor;
	aVertices[2].xyz = Vec3(fX2+fOff, fY2+fOff, 0.0f);
	aVertices[2].st = Vec2(0, 0);

	uint16 ausIndices[] = {0,1,2};

	m_pRenderer->SetWhiteTexture();
	m_pRenderer->DrawDynVB(aVertices,ausIndices,3,sizeof(ausIndices)/sizeof(ausIndices[0]),prtTriangleList);
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::DrawQuad(	float fX,
												float fY,
												float fSizeX,
												float fSizeY,
												uint32 uiDiffuse,
												uint32 uiDiffuseTL,
												uint32 uiDiffuseTR,
												uint32 uiDiffuseDL,
												uint32 uiDiffuseDR,
												int iTextureID,
												float fUTexCoordsTL,
												float fVTexCoordsTL,
												float fUTexCoordsTR,
												float fVTexCoordsTR,
												float fUTexCoordsDL,
												float fVTexCoordsDL,
												float fUTexCoordsDR,
												float fVTexCoordsDR,
												bool bUse169)
{
	SVF_P3F_C4B_T2F aVertices[4];

	if(bUse169)
	{
		float fWidth43 = m_pRenderer->GetHeight() * 4.0f / 3.0f;
		float fScale = fWidth43 / (float) m_pRenderer->GetWidth();
		float fOffset = (fSizeX - fSizeX * fScale);
		fX += 0.5f * fOffset;
		fSizeX -= fOffset;
	}

	const float fOff = -0.5f;

	aVertices[0].color.dcolor = uiDiffuse ? uiDiffuse : uiDiffuseTL;
	aVertices[0].xyz = Vec3(m_pRenderer->ScaleCoordX(fX)+fOff, m_pRenderer->ScaleCoordY(fY)+fOff, 0.0f);
	aVertices[0].st = Vec2(fUTexCoordsTL, fVTexCoordsTL);

	aVertices[1].color.dcolor = uiDiffuse ? uiDiffuse : uiDiffuseTR;
	aVertices[1].xyz = Vec3(m_pRenderer->ScaleCoordX(fX+fSizeX)+fOff, m_pRenderer->ScaleCoordY(fY)+fOff, 0.0f);
	aVertices[1].st = Vec2(fUTexCoordsTR, fVTexCoordsTR);

	aVertices[2].color.dcolor = uiDiffuse ? uiDiffuse : uiDiffuseDL;
	aVertices[2].xyz = Vec3(m_pRenderer->ScaleCoordX(fX)+fOff, m_pRenderer->ScaleCoordY(fY+fSizeY)+fOff, 0.0f);
	aVertices[2].st = Vec2(fUTexCoordsDL, fVTexCoordsDL);

	aVertices[3].color.dcolor = uiDiffuse ? uiDiffuse : uiDiffuseDR;
	aVertices[3].xyz = Vec3(m_pRenderer->ScaleCoordX(fX+fSizeX)+fOff, m_pRenderer->ScaleCoordY(fY+fSizeY)+fOff, 0.0f);
	aVertices[3].st = Vec2(fUTexCoordsDR, fVTexCoordsDR);

	if(iTextureID >= 0)
	{
		m_pRenderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
		m_pRenderer->SetTexture(iTextureID);
	}
	else
	{
		//m_pRenderer->EnableTMU(false);
		// m_pRenderer->SetWhiteTexture();
	}

	uint16 ausIndices[] = {0,1,2,3};

	m_pRenderer->DrawDynVB(aVertices,ausIndices,4,4,prtTriangleStrip);
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::DrawQuadSimple(	float fX,
															float fY,
															float fSizeX,
															float fSizeY,
															uint32 uiDiffuse,
															int iTextureID)
{
	SVF_P3F_C4B_T2F aVertices[4];

	const float fOff = -0.5f;

	aVertices[0].color.dcolor = uiDiffuse;
	aVertices[0].xyz = Vec3(fX+fOff, fY+fOff, 0.0f);
	aVertices[0].st = Vec2(0, 0);

	aVertices[1].color.dcolor = uiDiffuse;
	aVertices[1].xyz = Vec3(fX+fSizeX+fOff, fY+fOff, 0.0f);
	aVertices[1].st = Vec2(1, 0);

	aVertices[2].color.dcolor = uiDiffuse;
	aVertices[2].xyz = Vec3(fX+fOff, fY+fSizeY+fOff, 0.0f);
	aVertices[2].st = Vec2(0, 1);

	aVertices[3].color.dcolor = uiDiffuse;
	aVertices[3].xyz = Vec3(fX+fSizeX+fOff, fY+fSizeY+fOff, 0.0f);
	aVertices[3].st = Vec2(1, 1);

	if(iTextureID)
	{
		m_pRenderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
		m_pRenderer->SetTexture(iTextureID);
	}
	else
	{
		//m_pRenderer->EnableTMU(false);
		// m_pRenderer->SetWhiteTexture();
	}

	uint16 ausIndices[] = {0,1,2,3};

	m_pRenderer->DrawDynVB(aVertices,ausIndices,4,4,prtTriangleStrip);
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::DrawImage(int iTextureID,	float fX,
																				float fY,
																				float fSizeX,
																				float fSizeY,
																				float fAngleInDegrees,
																				float fRed,
																				float fGreen,
																				float fBlue,
																				float fAlpha,
																				float fS0,
																				float fT0,
																				float fS1,
																				float fT1)
{
	float fWidth43 = m_pRenderer->GetHeight() * 4.0f / 3.0f;
	float fScale = fWidth43 / (float) m_pRenderer->GetWidth();
	float fOffset = (fSizeX - fSizeX * fScale);
	fX += 0.5f * fOffset;
	fSizeX -= fOffset;

	m_pRenderer->Draw2dImage(	fX,
														fY+fSizeY,
														fSizeX,
														-fSizeY,
														iTextureID,
														fS0,fT0,fS1,fT1,
														fAngleInDegrees,
														fRed,
														fGreen,
														fBlue,
														fAlpha);
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::DrawImageCentered(int iTextureID,	float fX,
																								float fY,
																								float fSizeX,
																								float fSizeY,
																								float fAngleInDegrees,
																								float fRed,
																								float fGreen,
																								float fBlue,
																								float fAlpha)
{
	float fImageX = fX - 0.5f * fSizeX;
	float fImageY = fY - 0.5f * fSizeY;

	DrawImage(iTextureID,fImageX,fImageY,fSizeX,fSizeY,fAngleInDegrees,fRed,fGreen,fBlue,fAlpha);
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::DrawTextSimple(IFFont *pFont,
															float fX, float fY,
															float fSizeX, float fSizeY,
															const char *strText, ColorF color,
															EUIDRAWHORIZONTAL	eUIDrawHorizontal, EUIDRAWVERTICAL		eUIDrawVertical)
{
	if (!pFont)
		return;

	// Note: First ScaleCoordY is not a mistake
	if (fSizeX <= 0.0f) fSizeX = 15.0f;
	if (fSizeY <= 0.0f) fSizeY = 15.0f;

	STextDrawContext cxt;
	cxt.SetSizeIn800x600(false);
	cxt.SetSize(vector2f(fSizeX, fSizeY));
	cxt.SetColor(color);

/*
	if (UIDRAWHORIZONTAL_CENTER == eUIDrawHorizontalDocking)
		fX += m_pRenderer->GetWidth() * 0.5f;
	else if (UIDRAWHORIZONTAL_RIGHT == eUIDrawHorizontalDocking)
		fX += m_pRenderer->GetWidth();

	if (UIDRAWVERTICAL_CENTER == eUIDrawVerticalDocking)
		fY += m_pRenderer->GetHeight() * 0.5f;
	else if (UIDRAWVERTICAL_BOTTOM == eUIDrawVerticalDocking)
		fY += m_pRenderer->GetHeight();
*/

	Vec2 vDim = pFont->GetTextSize(strText, true, cxt);

	int flags = 0;
	if (UIDRAWHORIZONTAL_CENTER == eUIDrawHorizontal)
	{
		fX -= vDim.x * 0.5f;
		flags |= eDrawText_Center;
	}
	else if (UIDRAWHORIZONTAL_RIGHT == eUIDrawHorizontal)
	{
		fX -= vDim.x;
		flags |= eDrawText_Right;
	}

	if (UIDRAWVERTICAL_CENTER == eUIDrawVertical)
	{
		fY -= vDim.y * 0.5f;
		flags |= eDrawText_CenterV;
	}
	else if (UIDRAWVERTICAL_BOTTOM == eUIDrawVertical)
	{
		fY -= vDim.y;
		flags |= eDrawText_Bottom;
	}

	cxt.SetFlags(flags);

	pFont->DrawString(fX,fY,strText, true, cxt);
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::DrawText(	IFFont *pFont,
												float fX,
												float fY,
												float fSizeX,
												float fSizeY,
												const char *strText,
												float fAlpha,
												float fRed,
												float fGreen,
												float fBlue,
												EUIDRAWHORIZONTAL	eUIDrawHorizontalDocking,
												EUIDRAWVERTICAL		eUIDrawVerticalDocking,
												EUIDRAWHORIZONTAL	eUIDrawHorizontal,
												EUIDRAWVERTICAL		eUIDrawVertical)
{
	if(NULL == pFont)
	{
		return;
	}
	// Note: First ScaleCoordY is not a mistake
	if (fSizeX<=0.0f) fSizeX=15.0f;
	if (fSizeY<=0.0f) fSizeY=15.0f;

	fSizeX = m_pRenderer->ScaleCoordY(fSizeX);
	fSizeY = m_pRenderer->ScaleCoordY(fSizeY);

	// Note: First ScaleCoordY is not a mistake
	float fTextX = m_pRenderer->ScaleCoordY(fX);
	float fTextY = m_pRenderer->ScaleCoordY(fY);

	if(UIDRAWHORIZONTAL_CENTER == eUIDrawHorizontalDocking)
	{
		fTextX += m_pRenderer->GetWidth() * 0.5f;
	}
	else if(UIDRAWHORIZONTAL_RIGHT == eUIDrawHorizontalDocking)
	{
		fTextX += m_pRenderer->GetWidth();
	}

	if(UIDRAWVERTICAL_CENTER == eUIDrawVerticalDocking)
	{
		fTextY += m_pRenderer->GetHeight() * 0.5f;
	}
	else if(UIDRAWVERTICAL_BOTTOM == eUIDrawVerticalDocking)
	{
		fTextY += m_pRenderer->GetHeight();
	}

	STextDrawContext ctx;
	vector2f vDim = pFont->GetTextSize(strText, true, ctx);

	if(UIDRAWHORIZONTAL_CENTER == eUIDrawHorizontal)
	{
		fTextX -= vDim.x * 0.5f;
	}
	else if(UIDRAWHORIZONTAL_RIGHT == eUIDrawHorizontal)
	{
		fTextX -= vDim.x;
	}

	if(UIDRAWVERTICAL_CENTER == eUIDrawVertical)
	{
		fTextY -= vDim.y * 0.5f;
	}
	else if(UIDRAWVERTICAL_BOTTOM == eUIDrawVertical)
	{
		fTextY -= vDim.y;
	}

	SDrawTextInfo info;
	info.color[0] = fRed;
	info.color[1] = fGreen;
	info.color[2] = fBlue;
	info.color[3] = fAlpha;

 	// Adjusting the scale values for the Draw2dText call
	info.xscale = fSizeX/UIDRAW_TEXTSIZEFACTOR;
	info.yscale = fSizeY/UIDRAW_TEXTSIZEFACTOR;

	info.flags = eDrawText_2D | eDrawText_FixedSize ;

	DrawText(fTextX, fTextY, info, "%s", strText);
}

void CUIDraw::DrawText( float x, float y, SDrawTextInfo& info, const char * format, ...) {
	va_list args;
	va_start(args, format);
	m_pRenderer->DrawTextQueued(Vec3(x, y, 0.0f), info, "%s", args);
	va_end(args);
}
//-----------------------------------------------------------------------------------------------------
void CUIDraw::GetTextDim(	IFFont *pFont,
													float *fWidth,
													float *fHeight,
													float fSizeX,
													float fSizeY,
													const char *strText)
{
	if(NULL == pFont)
	{
		return;
	}

	//	fSizeX = m_pRenderer->ScaleCoordX(fSizeX);
	//	fSizeY = m_pRenderer->ScaleCoordY(fSizeY);

	// Note: First ScaleCoordY is not a mistake
	if (fSizeX<=0.0f) fSizeX=15.0f;
	if (fSizeY<=0.0f) fSizeY=15.0f;

	fSizeX = m_pRenderer->ScaleCoordY(fSizeX);
	fSizeY = m_pRenderer->ScaleCoordY(fSizeY);

	STextDrawContext ctx;
	ctx.SetSizeIn800x600(false);
	ctx.SetSize(vector2f(fSizeX,fSizeY));

	Vec2 dim=pFont->GetTextSize(strText, true, ctx);
	
	float fScaleBack=1.0f/m_pRenderer->ScaleCoordY(1.0f);
	if (fWidth)
		*fWidth=dim.x*fScaleBack;
	if (fHeight)
		*fHeight=dim.y*fScaleBack;
}


//-----------------------------------------------------------------------------------------------------

void CUIDraw::DrawTextW(IFFont *pFont,
												float fX,
												float fY,
												float fSizeX,
												float fSizeY,
												const wchar_t *strText,
												float fAlpha,
												float fRed,
												float fGreen,
												float fBlue,
												EUIDRAWHORIZONTAL	eUIDrawHorizontalDocking,
												EUIDRAWVERTICAL		eUIDrawVerticalDocking,
												EUIDRAWHORIZONTAL	eUIDrawHorizontal,
												EUIDRAWVERTICAL		eUIDrawVertical)
{
	InternalDrawTextW(pFont, fX, fY, 0.0f,
		fSizeX, fSizeY, 
		strText, 
		fAlpha, fRed, fGreen, fBlue, 
		eUIDrawHorizontalDocking, eUIDrawVerticalDocking, eUIDrawHorizontal, eUIDrawVertical);
}

void CUIDraw::DrawWrappedTextW(	IFFont *pFont,
																float fX,
																float fY,
																float fMaxWidth,
																float fSizeX,
																float fSizeY,
																const wchar_t *strText,
																float fAlpha,
																float fRed,
																float fGreen,
																float fBlue,
																EUIDRAWHORIZONTAL	eUIDrawHorizontalDocking,
																EUIDRAWVERTICAL		eUIDrawVerticalDocking,
																EUIDRAWHORIZONTAL	eUIDrawHorizontal,
																EUIDRAWVERTICAL		eUIDrawVertical
												)
{
	InternalDrawTextW(pFont, fX, fY, fMaxWidth,
		fSizeX, fSizeY, 
		strText, 
		fAlpha, fRed, fGreen, fBlue, 
		eUIDrawHorizontalDocking, eUIDrawVerticalDocking, eUIDrawHorizontal, eUIDrawVertical);
}


//-----------------------------------------------------------------------------------------------------

void CUIDraw::InternalDrawTextW(IFFont *pFont,
																float fX,
																float fY,
																float fMaxWidth,
																float fSizeX,
																float fSizeY,
																const wchar_t *strText,
																float fAlpha,
																float fRed,
																float fGreen,
																float fBlue,
																EUIDRAWHORIZONTAL	eUIDrawHorizontalDocking,
																EUIDRAWVERTICAL		eUIDrawVerticalDocking,
																EUIDRAWHORIZONTAL	eUIDrawHorizontal,
																EUIDRAWVERTICAL		eUIDrawVertical
																)
{
	if(NULL == pFont)
	{
		return;
	}

	const bool bWrapText = fMaxWidth > 0.0f;
	if (bWrapText)
		fMaxWidth = m_pRenderer->ScaleCoordX(fMaxWidth);

	//	fSizeX = m_pRenderer->ScaleCoordX(fSizeX);
	//	fSizeY = m_pRenderer->ScaleCoordY(fSizeY);

	// Note: First ScaleCoordY is not a mistake
	if (fSizeX<=0.0f) fSizeX=15.0f;
	if (fSizeY<=0.0f) fSizeY=15.0f;

	fSizeX = m_pRenderer->ScaleCoordY(fSizeX);
	fSizeY = m_pRenderer->ScaleCoordY(fSizeY);

	STextDrawContext ctx;
	ctx.SetSizeIn800x600(false);
	ctx.SetSize(vector2f(fSizeX,fSizeY));
	ctx.SetColor(ColorF(fRed,fGreen,fBlue,fAlpha));

	// Note: First ScaleCoordY is not a mistake

	float fTextX = m_pRenderer->ScaleCoordY(fX);
	float fTextY = m_pRenderer->ScaleCoordY(fY);

	if(UIDRAWHORIZONTAL_CENTER == eUIDrawHorizontalDocking)
	{
		fTextX += m_pRenderer->GetWidth() * 0.5f;
	}
	else if(UIDRAWHORIZONTAL_RIGHT == eUIDrawHorizontalDocking)
	{
		fTextX += m_pRenderer->GetWidth();
	}

	if(UIDRAWVERTICAL_CENTER == eUIDrawVerticalDocking)
	{
		fTextY += m_pRenderer->GetHeight() * 0.5f;
	}
	else if(UIDRAWVERTICAL_BOTTOM == eUIDrawVerticalDocking)
	{
		fTextY += m_pRenderer->GetHeight();
	}

	wstring wrappedStr;
	if (bWrapText)
	{
		pFont->WrapText(wrappedStr, fMaxWidth, strText, ctx);
		strText = wrappedStr.c_str();
	}

	Vec2 vDim = pFont->GetTextSizeW(strText, true, ctx);
	int flags = 0;

	if(UIDRAWHORIZONTAL_CENTER == eUIDrawHorizontal)
	{
		fTextX -= vDim.x * 0.5f;
		flags |= eDrawText_Center;
	}
	else if(UIDRAWHORIZONTAL_RIGHT == eUIDrawHorizontal)
	{
		fTextX -= vDim.x;
		flags |= eDrawText_Right;
	}

	if(UIDRAWVERTICAL_CENTER == eUIDrawVertical)
	{
		fTextY -= vDim.y * 0.5f;
		flags |= eDrawText_CenterV;
	}
	else if(UIDRAWVERTICAL_BOTTOM == eUIDrawVertical)
	{
		fTextY -= vDim.y;
		flags |= eDrawText_Bottom;
	}

	ctx.SetFlags(flags);

	pFont->DrawStringW(fTextX, fTextY, strText, true, ctx);
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::InternalGetTextDimW(IFFont *pFont,
													float *fWidth,
													float *fHeight,
													float fMaxWidth,
													float fSizeX,
													float fSizeY,
													const wchar_t *strText)
{
	if(NULL == pFont)
	{
		return;
	}

	const bool bWrapText = fMaxWidth > 0.0f;
	if (bWrapText)
		fMaxWidth = m_pRenderer->ScaleCoordX(fMaxWidth);

	//	fSizeX = m_pRenderer->ScaleCoordX(fSizeX);
	//	fSizeY = m_pRenderer->ScaleCoordY(fSizeY);

	// Note: First ScaleCoordY is not a mistake
	if (fSizeX<=0.0f) fSizeX=15.0f;
	if (fSizeY<=0.0f) fSizeY=15.0f;

	fSizeX = m_pRenderer->ScaleCoordY(fSizeX);
	fSizeY = m_pRenderer->ScaleCoordY(fSizeY);

	STextDrawContext ctx;
	ctx.SetSizeIn800x600(false);
	ctx.SetSize(Vec2(fSizeX, fSizeY));

	wstring wrappedStr;
	if (bWrapText)
	{
		pFont->WrapText(wrappedStr, fMaxWidth, strText, ctx);
		strText = wrappedStr.c_str();
	}

	Vec2 dim = pFont->GetTextSizeW(strText, true, ctx);

	float fScaleBack=1.0f/m_pRenderer->ScaleCoordY(1.0f);
	if (fWidth)
		*fWidth=dim.x*fScaleBack;
	if (fHeight)
		*fHeight=dim.y*fScaleBack;
}

//-----------------------------------------------------------------------------------------------------

void CUIDraw::GetTextDimW(IFFont *pFont,
																 float *fWidth,
																 float *fHeight,
																 float fSizeX,
																 float fSizeY,
																 const wchar_t *strText)
{
	InternalGetTextDimW(pFont, fWidth, fHeight, 0.0f, fSizeX, fSizeY, strText);
}

void CUIDraw::GetWrappedTextDimW(IFFont *pFont,
																 float *fWidth,
																 float *fHeight,
																 float fMaxWidth,
																 float fSizeX,
																 float fSizeY,
																 const wchar_t *strText)
{
	InternalGetTextDimW(pFont, fWidth, fHeight, fMaxWidth, fSizeX, fSizeY, strText);
}


//-----------------------------------------------------------------------------------------------------

void CUIDraw::GetMemoryStatistics(ICrySizer * s)
{
	SIZER_SUBCOMPONENT_NAME(s, "UIDraw");
	s->Add(*this);
	s->AddContainer(m_texturesMap);
}

//-----------------------------------------------------------------------------------------------------
#include UNIQUE_VIRTUAL_WRAPPER(IUIDraw)