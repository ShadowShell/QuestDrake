////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   BSPTree3D.cpp
//  Version:     v1.00
//  Created:     23/Nov/2011 by Jaesik.
//  Compilers:   Visual Studio 2010
//  Description: 
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BSPTree3D.h"

class BSPTreeNode
{
public:
	BSPTreeNode() :
	m_PosChild(NULL),
	m_NegChild(NULL)
	{
	}
	~BSPTreeNode()
	{
		if( m_PosChild )
			delete m_PosChild;
		if( m_NegChild )
			delete m_NegChild;
	}

	AreaUtil::EPointPosEnum IsPointIn(  const Vec3& vPos ) const
	{
		float distance(m_Plane.Distance(vPos));

		if( distance > AreaUtil::EPSILON )
		{
			if( m_PosChild )
				return m_PosChild->IsPointIn(vPos);
			else
				return AreaUtil::ePP_OUTSIDE;
		}
		else if( distance < -AreaUtil::EPSILON )
		{
			if( m_NegChild )
				return m_NegChild->IsPointIn(vPos);
			else
				return AreaUtil::ePP_INSIDE;
		}
		else
		{
			if( m_PosChild || m_NegChild )
			{
				if( m_PosChild && m_PosChild->IsPointIn(vPos) != AreaUtil::ePP_OUTSIDE || 
					m_NegChild && m_NegChild->IsPointIn(vPos) != AreaUtil::ePP_OUTSIDE )
				{
					return AreaUtil::ePP_BORDER;
				}
				return AreaUtil::ePP_OUTSIDE;
			}
			return AreaUtil::ePP_BORDER;
		}
	}

	void GetMemoryUsage( ICrySizer* pSizer ) const
	{
		SIZER_COMPONENT_NAME(pSizer,"BSPTreeNode");
		pSizer->AddObject(this,sizeof(*this));
		if( m_PosChild )
			m_PosChild->GetMemoryUsage(pSizer);
		if( m_NegChild )
			m_NegChild->GetMemoryUsage(pSizer);
	}

	AreaUtil::CPlaneBase m_Plane;
	BSPTreeNode* m_PosChild;
	BSPTreeNode* m_NegChild;
};

CBSPTree3D::CBSPTree3D( const AreaUtil::FaceList& faceList )
{
	m_pBSPTree = BuildTree(faceList);
}

CBSPTree3D::~CBSPTree3D()
{
	if( m_pBSPTree )
		delete m_pBSPTree;
}

bool CBSPTree3D::IsInside( const Vec3& vPos ) const
{
	if( m_pBSPTree == NULL )
		return false;
	
	return m_pBSPTree->IsPointIn(vPos) == AreaUtil::ePP_INSIDE;
}

BSPTreeNode* CBSPTree3D::BuildTree( const AreaUtil::FaceList& faceList )
{
	assert( !faceList.empty() );

	BSPTreeNode* pTree = new BSPTreeNode;

	pTree->m_Plane = AreaUtil::CPlaneBase( faceList[0][0], faceList[0][1], faceList[0][2] );

	AreaUtil::FaceList posList, negList;

	for( int i = 0, faceSize(faceList.size()); i < faceSize; ++i )
	{
		AreaUtil::CFace posFace;
		AreaUtil::CFace negFace;

		AreaUtil::ESplitResult type = SplitFaceByPlane( pTree->m_Plane, faceList[i], posFace, negFace );

		if( type == AreaUtil::eSR_CROSS )
		{
			posList.push_back(posFace);
			negList.push_back(negFace);
		}
		else if( type == AreaUtil::eSR_POSITIVE )
		{
			posList.push_back(faceList[i]);
		}
		else if( type == AreaUtil::eSR_NEGATIVE )
		{
			negList.push_back(faceList[i]);
		}
	}

	if( !posList.empty() )
		pTree->m_PosChild = BuildTree(posList);

	if( !negList.empty() )
		pTree->m_NegChild = BuildTree(negList);

	return pTree;
}

AreaUtil::ESplitResult CBSPTree3D::SplitFaceByPlane( const AreaUtil::CPlaneBase& plane, const AreaUtil::CFace& inFace, AreaUtil::CFace& outPosFace, AreaUtil::CFace& outNegFace )
{
	std::vector<char> signInfoList;
	signInfoList.resize(inFace.size());
	bool bPosPt = false;
	bool bNegPt = false;	

	for( int i = 0, iPtSize(inFace.size()); i < iPtSize; ++i )
	{
		const Vec3& p(inFace[i]);
		float d(plane.Distance(p));
		if( d < -AreaUtil::EPSILON )
		{
			bNegPt = true;
			signInfoList[i] = -1;
		}
		else if( d > AreaUtil::EPSILON )
		{
			bPosPt = true;
			signInfoList[i] = 1;
		}
		else
		{
			signInfoList[i] = 0;
		}
	}

	if( bPosPt && bNegPt )
	{
		int nIndexFromPlusToMinus = -1;
		int nIndexFromMinusToPlus = -1;
		Vec3 vFromPlusToMinus;
		Vec3 vFromMinusToPlus;

		for( int i = 0, iPtSize(signInfoList.size()); i < iPtSize; )
		{
			int next_i = i;
			while( signInfoList[(++next_i)%iPtSize] == 0 );
			int i2(next_i);
			next_i %= iPtSize;
			if( signInfoList[i]==1 && signInfoList[next_i]==-1 )
			{
				nIndexFromPlusToMinus = next_i;
				float t(0);
				if( i2-i == 1 )
					plane.HitTest( inFace[i], inFace[next_i], &t, &(vFromPlusToMinus) );
				else if( i2-i > 1 )
					vFromPlusToMinus = inFace[(i+1)%iPtSize];
			}
			else if( signInfoList[i] == -1 && signInfoList[next_i]==1 )
			{
				nIndexFromMinusToPlus = next_i;
				float t(0);
				if( i2-i == 1 )
					plane.HitTest( inFace[i], inFace[next_i], &t, &(vFromMinusToPlus) );
				else if( i2-i > 1 )
					vFromMinusToPlus = inFace[(i+1)%iPtSize];
			}
			i = i2;
		}

		assert( nIndexFromPlusToMinus != -1 && nIndexFromMinusToPlus != -1 );

		for( int i = 0, iSize(signInfoList.size()); i < iSize; ++i )
		{
			int nIndex = (i+nIndexFromMinusToPlus)%iSize;
			if( nIndex == nIndexFromPlusToMinus )
				break;
			if( signInfoList[nIndex] == 0 )
				continue;
			outPosFace.push_back(inFace[nIndex]);
		}

		outPosFace.push_back(vFromPlusToMinus);
		outPosFace.push_back(vFromMinusToPlus);		

		for( int i = 0, iSize(signInfoList.size()); i < iSize; ++i )
		{
			int nIndex = (i+nIndexFromPlusToMinus)%iSize;
			if( nIndex == nIndexFromMinusToPlus )
				break;
			if( signInfoList[nIndex] == 0 )
				continue;
			outNegFace.push_back(inFace[nIndex]);
		}

		outNegFace.push_back(vFromMinusToPlus);		
		outNegFace.push_back(vFromPlusToMinus);

		return AreaUtil::eSR_CROSS;
	}
	else if( bPosPt && !bNegPt )
	{
		return AreaUtil::eSR_POSITIVE;
	}
	else if( !bPosPt && bNegPt )
	{
		return AreaUtil::eSR_NEGATIVE;
	}
	else
	{
		return AreaUtil::eSR_COINCIDENCE;
	}
}

void CBSPTree3D::GetMemoryUsage( ICrySizer* pSizer ) const
{
	SIZER_COMPONENT_NAME(pSizer,"CBSPTree3D");
	if( m_pBSPTree )
		m_pBSPTree->GetMemoryUsage(pSizer);
	pSizer->AddObject(this,sizeof(*this));
}
