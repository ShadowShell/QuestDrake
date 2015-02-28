////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   BSPTree.h
//  Version:     v1.00
//  Created:     23/Nov/2011 by Jaesik.
//  Compilers:   Visual Studio 2010
//  Description: 
////////////////////////////////////////////////////////////////////////////

#ifndef __BSPTREE3D_H_
#define __BSPTREE3D_H_

#include "AreaUtil.h"

class BSPTreeNode;

class CBSPTree3D
{
public:
	CBSPTree3D( const AreaUtil::FaceList& faceList );
	~CBSPTree3D();

	bool IsInside(const Vec3& vPos) const;
	void GetMemoryUsage( ICrySizer* pSizer ) const;

private:

	BSPTreeNode* BuildTree( const AreaUtil::FaceList& faceList );	
	static AreaUtil::ESplitResult SplitFaceByPlane( const AreaUtil::CPlaneBase& plane, const AreaUtil::CFace& inFace, AreaUtil::CFace& outPosFace, AreaUtil::CFace& outNegFace );

private:

	BSPTreeNode* m_pBSPTree;

};

#endif // __BSPTREE3D_H_
