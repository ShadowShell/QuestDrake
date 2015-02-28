// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"

#ifdef USE_KINECT

#include "KinectInput.h"
#include <IRenderer.h>
#include "IRenderAuxGeom.h"

#ifdef KINECT_XBOX_CONNECT
#include "KinectXboxSyncThread.h"
#endif

#ifdef WIN32
	// Link to MS Research KinectNUI
	#pragma comment(lib, "MSRKinectNUI.lib")
#endif
#include "Filters\JointFilter.h"

#define NUI_FRAME_END_TIMEOUT_DEFAULT 10
#define MAX_EVENTS 5

#ifdef KINECT_USE_IDENTITY
BOOL NuiIdentityCallBack(PVOID pContect, NUI_IDENTITY_MESSAGE* pMessage)
{
	CKinectInput* pKinInput = (CKinectInput*)pContect;
	return pKinInput->OnIdentityCallBack(pMessage);
}
#endif

namespace
{
	void LogKinectError(HRESULT hr, const char* functionName, DWORD nError)
	{
		switch(hr)
		{
		case E_INVALIDARG:
			CryLog("[KINECT] Invalid arguments passed to: %s", functionName);
			break;
		case E_NUI_ALREADY_INITIALIZED:
			CryLog("[KINECT] There is already a Kinect interface running - failure in function: %s", functionName);
			break;
		case E_NUI_DATABASE_NOT_FOUND:
			CryLog("[KINECT] Couldn't find the Kinect database. Make sure you have the correct files on your xbox - failure in function: %s", functionName);
			break;
		case E_NUI_DATABASE_VERSION_MISMATCH:
			CryLog("[KINECT] Database mismatch for Kinect - failure in function: %s", functionName);
			break;
		case E_OUTOFMEMORY:
			CryLog("[KINECT] out of memory when calling: %s", functionName);
			break;
		case E_NUI_FEATURE_NOT_INITIALIZED:
			CryLog("[KINECT] NuiInitialize was not called with parameters that allow the requested stream type with function: %s", functionName);
			break;
		case E_NUI_IMAGE_STREAM_IN_USE:
			CryLog("[KINECT] The request's stream type is already open. with function: %s", functionName);
			break;
		case E_FAIL:
			CryLog("[KINECT] The skeleton tracking is not enabled - failure when calling %s", functionName);
			break;
#ifdef KINECT_USE_IDENTITY
		case E_NUI_IDENTITY_LOST_TRACK:
			CryLog("[KINECT] The tracking id is no longer being tracked: failure when calling: %s", functionName);
			break;
#endif
		/*case SPERR_UNINITIALIZED:
			CryLog("[KINECT] Speech engine has not been succesfully started: failure when calling: %s", functionName);
			break;*/
		default:
			CryLog("[KINECT] unknown error in function: %s, HRESULT: %i, Error number: %i", functionName, hr, nError);
		}
	}

	void DebugVec3(float fXpos, float fYpos, const char* name, const Vec3& vDebugVec)
	{
		float fColor[] = {1.f, 1.f, 1.f, 1.f};
		gEnv->pRenderer->Draw2dLabel(fXpos, fYpos, 2.f, fColor, false, "%s (%f, %f, %f)", name, vDebugVec.x, vDebugVec.y, vDebugVec.z);
	}
}

CKinectInput::CKinectInput():
  m_hDepth320x240(0)
, m_hDepth80x60(0)
, m_hFrameEndEvent(0)
, m_iDepth320x240(0)
, m_iImage(0)
, m_closestSkeletonIndex(KIN_SKELETON_INVALID_TRACKING_ID)
, m_eventListeners(1)
, m_bUseSeatedST(false)
, m_lastSkeletonPosition(0,0,0,0)
{
	m_enabled = false;

#ifdef KINECT_XBOX_CONNECT
	m_pXboxKinectSyncThread = 0;
#endif

	memset(m_skeletonFilter, 0, sizeof(ISkeletonFilter*) * KIN_SKELETON_COUNT);
}

CKinectInput::~CKinectInput()
{
#ifdef KINECT_XBOX_CONNECT
	delete m_pXboxKinectSyncThread;
#endif

	for(int i = 0; i < KIN_SKELETON_COUNT; ++i)
	{
		delete m_skeletonFilter[i];
	}
}

bool CKinectInput::Init()
{
	// check if kinect is connected
	if(!g_pInputCVars->i_useKinect)
		return false;

	m_pImageFrame = NULL;
	m_pDepthFrame80x60 = NULL;
	m_pDepthFrame320x240 = NULL;

	// initialize the skeleton to non tracked and setup basic values for default skeleton data
	for (int i = 0; i != KIN_SKELETON_COUNT; ++i)
	{		
		m_skeletonFrame.skeletonRawData[i].eTrackingState = KIN_SKELETON_NOT_TRACKED;
		//Zero out alignment data for un-tracked skeletons
		m_skeletonFrame.skeletonDefaultData[i].vPosition = Vec4(0,0,0,0);
		for(int32 iJoint = 0; iJoint < KIN_SKELETON_POSITION_COUNT; ++iJoint)
			m_skeletonFrame.skeletonDefaultData[i].vSkeletonPositions[iJoint] = Vec4(0,0,0,0);
	}

	bool bAllowUseLocalKinect = true;
#ifdef KINECT_XBOX_CONNECT
	if (g_pInputCVars->i_kinectXboxConnect == 2)
		bAllowUseLocalKinect = false;
#endif
	for(int i = 0; i < KIN_SKELETON_COUNT; ++i)
	{
		m_skeletonFilter[i] = new FilterDoubleExponential();
	}


	HRESULT hr = E_FAIL;	
	if (bAllowUseLocalKinect)
	{
		hr = NuiInitialize(
			NUI_INITIALIZE_FLAG_USES_SKELETON | 
			NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | 
#ifdef KINECT_USE_SPEECH
			NUI_INITIALIZE_FLAG_USES_SPEECH |
#endif
			NUI_INITIALIZE_FLAG_USES_COLOR
			);
	}

	if (FAILED(hr))
	{
		NuiShutdown();

#ifdef KINECT_XBOX_CONNECT
		if (g_pInputCVars->i_kinectXboxConnect > 0) 
		{
			SetupXboxKinectThread();
			return m_pXboxKinectSyncThread != 0;
		} else 
#endif
		{
			LogKinectError(hr, "NuiInitialize", 0);
		}
		return false;
	}

	DWORD dwFlags = NUI_SKELETON_TRACKING_FLAG_SUPPRESS_NO_FRAME_DATA;
	hr = NuiSkeletonTrackingEnable( NULL, dwFlags );
	if (FAILED(hr))
	{
		LogKinectError(hr, "NuiSkeletonTrackingEnable", 0);
	}

	// Open image stream
	hr = NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_STREAM_FLAG_SUPPRESS_NO_FRAME_DATA, 1, NULL, &m_hImage );
	if ( FAILED (hr) )
	{
		LogKinectError( hr,"NuiImageStreamOpen", 0);
		return false;
	}


	// Open the depth stream
	hr = NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_320x240, NUI_IMAGE_STREAM_FLAG_SUPPRESS_NO_FRAME_DATA, 1, NULL, &m_hDepth320x240 );
	if ( FAILED (hr) )
	{
		LogKinectError(hr, "NuiImageStreamOpen", 0);
		return false;
	}

#ifdef KINECT_USE_HANDLES
	for (int i = 0; i != KIN_SKELETON_COUNT; ++i)
	{
		hr = NuiHandlesArmsInit(&m_nuiArms[i]);
		if (FAILED(hr))
		{
			LogKinectError(hr, "NuiHandlesArmsInit", 0);
		}
	}
#endif

	m_iDepth320x240 = gEnv->pRenderer->SF_CreateTexture(320, 240, 0, NULL, eTF_A8R8G8B8, FT_NOMIPS|FT_DONT_RESIZE|FT_DONT_RELEASE|FT_DONT_STREAM);
	m_iImage = gEnv->pRenderer->SF_CreateTexture(640, 480, 0, NULL, eTF_X8R8G8B8, FT_NOMIPS|FT_DONT_RESIZE|FT_DONT_RELEASE|FT_DONT_STREAM);

	SpeechEnable();

	m_enabled = true;
	return true;
}

#ifdef KINECT_XBOX_CONNECT
void CKinectInput::SetupXboxKinectThread()
{
	m_pXboxKinectSyncThread = new CKinectXboxSyncThread;
	if (!m_pXboxKinectSyncThread->Initialize())
	{
		delete m_pXboxKinectSyncThread;
		m_pXboxKinectSyncThread = 0;
		return;
	}

	m_pXboxKinectSyncThread->Start();
}
#endif

void CKinectInput::Update()
{
	//Update seated skeleton tracking state
	EnableSeatedSkeletonTracking(g_pInputCVars->i_seatedTracking > 0);

	if(!m_enabled)
	{
#ifdef KINECT_XBOX_CONNECT
		if (m_pXboxKinectSyncThread)
		{
			m_pXboxKinectSyncThread->GetSkeletonFrame(m_skeletonFrame);

			if(g_pInputCVars->i_kinSkeletonSmoothType==1)
			{
				//Smooth only tracked skeletons
				for(int i = 0; i < KIN_SKELETON_COUNT; ++i)
				{
					SKinSkeletonRawData& skeletonData = m_skeletonFrame.skeletonRawData[i];
					if(skeletonData.eTrackingState == NUI_SKELETON_TRACKED)
					{
						if(ISkeletonFilter* pFilter = m_skeletonFilter[i])
						{
							//Update the filter using the new set of data, and then extract the smoothed joints into the current frame
							pFilter->Update(skeletonData, gEnv->pTimer->GetFrameTime());
							memcpy(skeletonData.vSkeletonPositions, pFilter->GetFilteredJoints(), sizeof(Vec4) * KIN_SKELETON_POSITION_COUNT);
						}
					}
				}
			}

			UpdateClosestSkeleton();
			PushRawNativeDataToListeners();

			DebugDraw();
		}
#endif
		return;
	}

	//	CONST NUI_IMAGE_FRAME* pDepthFrame320x240;
	// 	CONST NUI_IMAGE_FRAME* pDepthFrame80x60;

	m_hrImage = E_FAIL;
	m_hr320x240 = E_FAIL;
	m_hr80x60 = E_FAIL;

	m_hrImage = NuiImageStreamGetNextFrame( m_hImage, 0, &m_pImageFrame );
	// Get data from the next camera depth frame
	m_hr320x240 = NuiImageStreamGetNextFrame( m_hDepth320x240, 0, &m_pDepthFrame320x240 );
	// Get data from the next camera depth frame (80x60 resolution)
	m_hr80x60 = NuiImageStreamGetNextFrame(m_hDepth80x60, 0, &m_pDepthFrame80x60 );

	if (SUCCEEDED(m_hr320x240) /*&& SUCCEEDED(m_hr80x60) /*&& SUCCEEDED(m_hrImage)*/)	
	{
		float fNow = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
		HRESULT hr = NuiSkeletonGetNextFrame(0, &m_nuiSkeletonFrame);
		switch(hr)
		{
		case E_NUI_FRAME_NO_DATA:
			CryLog("[KINECT] no frame data");
			break;
		case E_INVALIDARG:
			CryLog("[KINECT] invalid arguments");
			break;
		case E_PENDING:
			CryLog("[KINECT] pending");
			break;
		case E_NUI_DEVICE_NOT_CONNECTED:
			CryLog("[KINECT] device not connected");
			break;
		case E_NUI_STREAM_NOT_ENABLED:
			CryLog("[KINECT] stream not enabled");
			break;
		case S_OK:

			CopyNUISkeletonFrameToNativeFormat(m_nuiSkeletonFrame, m_skeletonFrame);

#ifdef KINECT_USE_HANDLES
			UpdateScreenHands(m_nuiSkeletonFrame, m_skeletonFrame, &(m_nuiArms[0]), m_pDepthFrame320x240, m_pDepthFrame80x60);
#endif

			if(g_pInputCVars->i_kinSkeletonSmoothType==1)
			{
				//Smooth only tracked skeletons
				for(int i = 0; i < KIN_SKELETON_COUNT; ++i)
				{
					SKinSkeletonRawData& skeletonData = m_skeletonFrame.skeletonRawData[i];
					if(skeletonData.eTrackingState == NUI_SKELETON_TRACKED)
					{
						if(ISkeletonFilter* pFilter = m_skeletonFilter[i])
						{
							//Update the filter using the new set of data, and then extract the smoothed joints into the current frame
							pFilter->Update(skeletonData, gEnv->pTimer->GetFrameTime());
							memcpy(skeletonData.vSkeletonPositions, pFilter->GetFilteredJoints(), sizeof(Vec4) * KIN_SKELETON_POSITION_COUNT);
						}
					}
				}
			}
					
			//Morgan - Kind of hacky, but keep calculating closest skeleton
			UpdateClosestSkeleton();

			PushRawNativeDataToListeners();
			UpdateGrips();
			UpdateRails(fNow);
			PushRailData();

			break;
		}

	}

	DebugDraw();	

	if(m_pDepthFrame320x240)
		NuiImageStreamReleaseFrame( m_hDepth320x240, m_pDepthFrame320x240 );
	if(m_pDepthFrame80x60)
		NuiImageStreamReleaseFrame( m_hDepth80x60, m_pDepthFrame80x60 );
	if(m_pImageFrame)
		NuiImageStreamReleaseFrame( m_hImage, m_pImageFrame );

	m_pDepthFrame320x240 = 0;
	m_pDepthFrame80x60 = 0;
	m_pImageFrame = 0;

#ifdef KINECT_USE_SPEECH
	unsigned long nEventsFetched;
	NUI_SPEECH_EVENT events[MAX_EVENTS];  
	HRESULT hr = NuiSpeechGetEvents( MAX_EVENTS, events, &nEventsFetched );

	if( SUCCEEDED( hr ) )
	{
		for (unsigned long i = 0; i < nEventsFetched; ++i)
		{
			// Deal with this event
			OnSpeechEvent(events[i]);
			// Don't forget to "acknowledge" the event
			NuiSpeechDestroyEvent( &events[i] );
		}
	}
#endif
}

void CKinectInput::OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
{
	if (event == ESYSTEM_EVENT_LEVEL_LOAD_END)
	{
		m_closestSkeletonIndex = KIN_SKELETON_INVALID_TRACKING_ID;
		m_lastSkeletonPosition = Vec4(0,0,0,0);
	}
}

void CKinectInput::UpdateClosestSkeleton()
{
	float closestDepth = 0.0f;
	uint32 currClosest = KIN_SKELETON_INVALID_TRACKING_ID;

	for(uint32 iSkeleton = 0; iSkeleton < KIN_SKELETON_COUNT; ++iSkeleton)
	{
		if(m_skeletonFrame.skeletonRawData[iSkeleton].eTrackingState != KIN_SKELETON_TRACKED)
		{
			continue;
		}

		float depth = fabs(m_skeletonFrame.skeletonRawData[iSkeleton].vPosition.x);

		if(depth < closestDepth || currClosest == KIN_SKELETON_INVALID_TRACKING_ID)
		{
			currClosest = iSkeleton;
			closestDepth = depth;
		}
	}

	if(m_closestSkeletonIndex != currClosest)
	{
		for (TKinectEventListeners::Notifier notifier(m_eventListeners); notifier.IsValid(); notifier.Next())
			notifier->OnKinectClosestSkeletonChanged(currClosest);
	}

	m_closestSkeletonIndex = currClosest;

	CheckSkeletonPositions();
}

#ifdef KINECT_USE_HANDLES
void CKinectInput::UpdateScreenHands(const NUI_SKELETON_FRAME& source, SKinSkeletonFrame& sink, NUI_HANDLES_ARMS* arms, CONST NUI_IMAGE_FRAME* pDepthFrame320x240, CONST NUI_IMAGE_FRAME* pDepthFrame80x60)
{
	for (int i = 0; i != KIN_SKELETON_COUNT; ++i)
	{
		NuiHandlesArmsUpdate(&arms[i], i, &source, pDepthFrame320x240, NULL, pDepthFrame80x60, NULL);
		XMVECTOR vLeft = NuiHandlesArmGetScreenSpaceLocation(&m_nuiArms[i], NUI_HANDLES_ARMS_HANDEDNESS_LEFT_ARM);
		XMVECTOR vRight = NuiHandlesArmGetScreenSpaceLocation(&m_nuiArms[i], NUI_HANDLES_ARMS_HANDEDNESS_RIGHT_ARM);

		sink.skeletonRawData[i].screenHands.vLeftHand = Vec3(vLeft.x, vLeft.y, vLeft.z);
		sink.skeletonRawData[i].screenHands.vRightHand = Vec3(vRight.x, vRight.y, vRight.z);
	}
}
#endif

void CKinectInput::CopyNUISkeletonFrameToNativeFormat(const NUI_SKELETON_FRAME& sourceSkeleton, SKinSkeletonFrame& sinkSkeleton)
{
	sinkSkeleton.dwFlags						= sourceSkeleton.dwFlags;
	sinkSkeleton.dwFrameNumber			= sourceSkeleton.dwFrameNumber;
	sinkSkeleton.liTimeStamp				= sourceSkeleton.liTimeStamp.QuadPart;
	sinkSkeleton.vFloorClipPlane		= Vec4(sourceSkeleton.vFloorClipPlane.x, sourceSkeleton.vFloorClipPlane.y, sourceSkeleton.vFloorClipPlane.z, sourceSkeleton.vFloorClipPlane.w) ;
	sinkSkeleton.vNormalToGravity	= Vec4(sourceSkeleton.vNormalToGravity.x, sourceSkeleton.vNormalToGravity.y, sourceSkeleton.vNormalToGravity.z, sourceSkeleton.vNormalToGravity.w) ;
	for (int i = 0; i != KIN_SKELETON_COUNT; ++i)
	{
		if (sourceSkeleton.SkeletonData[i].eTrackingState == KIN_SKELETON_TRACKED)
		{
			sinkSkeleton.skeletonRawData[i].dwEnrollmentIndex = sourceSkeleton.SkeletonData[i].dwEnrollmentIndex;
			sinkSkeleton.skeletonRawData[i].dwQualityFlags = sourceSkeleton.SkeletonData[i].dwQualityFlags;
			sinkSkeleton.skeletonRawData[i].dwTrackingId = sourceSkeleton.SkeletonData[i].dwTrackingID;
			sinkSkeleton.skeletonRawData[i].dwUserIndex = sourceSkeleton.SkeletonData[i].dwUserIndex;
			sinkSkeleton.skeletonRawData[i].eTrackingState = (KIN_SKELETON_TRACKING_STATE)sourceSkeleton.SkeletonData[i].eTrackingState;
			sinkSkeleton.skeletonRawData[i].vPosition = Vec4(sourceSkeleton.SkeletonData[i].Position.x, sourceSkeleton.SkeletonData[i].Position.y, sourceSkeleton.SkeletonData[i].Position.z, sourceSkeleton.SkeletonData[i].Position.w);

			for (int j = 0; j != KIN_SKELETON_POSITION_COUNT - 2; ++j)
			{
				sinkSkeleton.skeletonRawData[i].eSkeletonPositionTrackingState[j] = (KIN_SKELETON_POSITION_TRACKING_STATE)sourceSkeleton.SkeletonData[i].eSkeletonPositionTrackingState[j];
				sinkSkeleton.skeletonRawData[i].vSkeletonPositions[j] = Vec4(sourceSkeleton.SkeletonData[i].SkeletonPositions[j].x, sourceSkeleton.SkeletonData[i].SkeletonPositions[j].y, sourceSkeleton.SkeletonData[i].SkeletonPositions[j].z, sourceSkeleton.SkeletonData[i].SkeletonPositions[j].w);
			}

			sinkSkeleton.skeletonRawData[i].eSkeletonPositionTrackingState[KIN_SKELETON_POSITION_HAND_REFINED_LEFT] = KIN_SKELETON_POSITION_NOT_TRACKED;
			sinkSkeleton.skeletonRawData[i].eSkeletonPositionTrackingState[KIN_SKELETON_POSITION_HAND_REFINED_RIGHT] = KIN_SKELETON_POSITION_NOT_TRACKED;
		}
		else
		{
			sinkSkeleton.skeletonRawData[i].eTrackingState = (KIN_SKELETON_TRACKING_STATE)sourceSkeleton.SkeletonData[i].eTrackingState;
			//Zero out alignment data for un-tracked skeletons
			sinkSkeleton.skeletonDefaultData[i].vPosition = Vec4(0,0,0,0);
			for(int32 iJoint = 0; iJoint < KIN_SKELETON_POSITION_COUNT; ++iJoint)
				sinkSkeleton.skeletonDefaultData[i].vSkeletonPositions[iJoint] = Vec4(0,0,0,0);

			sinkSkeleton.skeletonRawData[i].eSkeletonPositionTrackingState[KIN_SKELETON_POSITION_HAND_REFINED_LEFT] = KIN_SKELETON_POSITION_NOT_TRACKED;
			sinkSkeleton.skeletonRawData[i].eSkeletonPositionTrackingState[KIN_SKELETON_POSITION_HAND_REFINED_RIGHT] = KIN_SKELETON_POSITION_NOT_TRACKED;
		}
	}
}


void CKinectInput::PushRawNativeDataToListeners()
{
	for (TKinectEventListeners::Notifier notifier(m_eventListeners); notifier.IsValid(); notifier.Next())
		notifier->OnKinectRawInputEvent(m_skeletonFrame);
}

bool CKinectInput::RegisterArcRail(int gripId, int railId, const Vec2& vScreenPos, const Vec3& vDir, float fLenght, float fDeadzoneLength, float fToleranceConeAngle)
{
#ifdef KINECT_USE_HANDLES
	SKinGripInternal arcGrip;
	//TODO:
	//check if there is allready a gripid for this

	HRESULT hr = NuiHandlesGripInit(&arcGrip.nuiGrip);
	if (FAILED(hr))
	{
		CryLog("[KINECT]failed to create arc grip: gripId: %i, railId: %i", gripId, railId);
		return false;
	}

	hr = NuiHandlesGripAddRailArc(&arcGrip.nuiGrip, railId, XMVectorSet(vDir.x, vDir.y, vDir.z, 0.f), fLenght, fDeadzoneLength, fToleranceConeAngle);
	if (FAILED(hr))
	{
		switch(hr)
		{
		case NUI_HANDLES_E_INVALID_ARG:
			CryLog("[KINECT]added grip pointer is NULL, this should never happen");
			break;
		case NUI_HANDLES_E_RAIL_ID_INVALID:
			CryLog("[KINECT] invalid rail id: %i", railId);
			break;
		case NUI_HANDLES_E_ILLEGAL_WHILE_ATTACHED:
			CryLog("[KINECT] operation is not allowed while the grip is attached");
			break;
		case NUI_HANDLES_E_MAX_RAILS_EXCEEDED:
			CryLog("[KINECT] exceeded the max number of rails %i", NUI_HANDLES_MAX_RAILS_PER_GRIP);
			break;
		}
		return false;
	}

	NuiHandlesGripSetScreenSpaceLocation(&arcGrip.nuiGrip, XMVectorSet(vScreenPos.x, vScreenPos.y, 0.f, 0.f));

	arcGrip.railIds.push_back(railId);
	m_arcGrips.insert(std::make_pair(gripId, arcGrip));

#endif
	return true;
}

void CKinectInput::UnregisterArcRail(int gripId)
{
#ifdef KINECT_USE_HANDLES
	TKINGrips::iterator it = m_arcGrips.find(gripId);
	if (it != m_arcGrips.end())
	{
		m_arcGrips.erase(it);
	}
#endif
}

bool CKinectInput::RegisterHoverTimeRail(int gripId, int railId, const Vec2& vScreenPos, float fHoverTime, float fTimeTillCommit, SKinGripShape* pGripShape)
{
#ifdef KINECT_USE_HANDLES
	SKinGripInternal timeGrip;

	HRESULT hr = NuiHandlesGripInit(&timeGrip.nuiGrip);
	if (FAILED(hr))
	{
		CryLog("[KINECT]failed to create hover time grip: gripId: %i, railId: %i", gripId, railId);
		return false;
	}

	hr = NuiHandlesGripAddRailHoverTimer(&timeGrip.nuiGrip, railId, DWORD(fHoverTime * 1000.f), DWORD(fTimeTillCommit * 1000.f));
	if (FAILED(hr))
	{
		switch(hr)
		{
		case NUI_HANDLES_E_INVALID_ARG:
			CryLog("[KINECT] hoverTimeRail creation failed - invalid params");
			break;
		}
		return false;
	}

	NuiHandlesGripSetScreenSpaceLocation(&timeGrip.nuiGrip, XMVectorSet(vScreenPos.x, vScreenPos.y, 0.f, 0.f));

	if (pGripShape)
	{
		NUI_HANDLES_GRIPSHAPE gripShape;
		NuiHandlesGripGetShape(&timeGrip.nuiGrip, &gripShape);
		gripShape.fFastAttachRadius = pGripShape->fFastAttachRadius;
		gripShape.fSlowAttachRadius = pGripShape->fSlowAttachRadius;
		gripShape.fApproachRadius = pGripShape->fApproachRadius;
		gripShape.fSlowDetachRadius = pGripShape->fSlowDetachRadius;

		hr = NuiHandlesGripSetShape(&timeGrip.nuiGrip, &gripShape);
		if (FAILED(hr))
		{
			switch(hr)
			{
			case NUI_HANDLES_E_INVALID_ARG:
				CryLog("[KINECT] invalid SKinGripShape params passed to RegisterHoverTimeRail");
			break;
			}
		}
	}

	stl::push_back_unique(timeGrip.railIds, railId);
	m_timeGrips.insert(std::make_pair(gripId, timeGrip));

#endif
	return true;
}

void CKinectInput::UnregisterHoverTimeRail(int gripId)
{
#ifdef KINECT_USE_HANDLES
	TKINGrips::iterator it = m_timeGrips.find(gripId);
	if (it != m_timeGrips.end())
	{
		m_timeGrips.erase(it);
	}
#endif
}

void CKinectInput::UnregisterAllRails()
{
#ifdef KINECT_USE_HANDLES
	m_arcGrips.clear();
	m_timeGrips.clear();
#endif
}

void CKinectInput::UpdateGrips()
{
#ifdef KINECT_USE_HANDLES
	TKINGrips::iterator itArc = m_arcGrips.begin();
	TKINGrips::iterator endArc = m_arcGrips.end();

	HRESULT hr = S_OK;
	for (; itArc != endArc; ++itArc)
	{
		for (int i = 0; i != KIN_SKELETON_COUNT; ++i)
		{
			hr = NuiHandlesGripUpdate(&(*itArc).second.nuiGrip, &m_nuiArms[i]);
			if (FAILED(hr))
			{
				CryLog("[KINECT] grip pointer is 0");
				break;
			}
		}
	}

	TKINGrips::iterator itTime = m_timeGrips.begin();
	TKINGrips::iterator endTime = m_timeGrips.end();

	hr = S_OK;
	for (; itTime != endTime; ++itTime)
	{
		for (int i = 0; i != KIN_SKELETON_COUNT; ++i)
		{
			hr = NuiHandlesGripUpdate(&(*itTime).second.nuiGrip, &m_nuiArms[i]);
			if (FAILED(hr))
			{
				CryLog("[KINECT] grip pointer is 0");
				break;
			}
		}
	}
#endif
}

void CKinectInput::UpdateRails(float fTimeLastGetNextFrame)
{
#ifdef KINECT_USE_HANDLES
	float fNow = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
	
	for (int i = 0; i != KIN_SKELETON_COUNT; ++i)
	{
		NuiHandlesArmsUpdateRailProgress(&m_nuiArms[i], DWORD(gEnv->pTimer->GetFrameTime() * 1000.f));
	}
#endif
}

void CKinectInput::PushRailData()
{
#ifdef KINECT_USE_HANDLES

	for (TKinectEventListeners::Notifier notifier(m_eventListeners); notifier.IsValid(); notifier.Next())
	{
		// ARCS
		TKINGrips::iterator itAcr = m_arcGrips.begin();
		TKINGrips::iterator arcEnd = m_arcGrips.end();
		for (; itAcr != arcEnd; ++itAcr)
		{
			if (NuiHandlesGripGetIsAttached(&(*itAcr).second.nuiGrip))
			{
				NUI_HANDLES_RAILSTATE railState;
				DWORD iRailId;
				NuiHandlesGripGetCommittedRailId( &(*itAcr).second.nuiGrip, &iRailId );
				NuiHandlesGripGetRailState( &(*itAcr).second.nuiGrip, iRailId, &railState );
				SKinRailState kinRailState;
				kinRailState.bAttached = true;
				kinRailState.bCommited = railState.bCommitted;
				kinRailState.arcState.fDenormalizedProgress = railState.Arc.fDenormalizedProgress;
				kinRailState.arcState.fProgress = railState.Arc.fProgress;
				kinRailState.eGripType = KIN_GRIP_ARC;
				kinRailState.nGripId = (*itAcr).first;
				kinRailState.nRailId = (int)railState.dwRailID;
				notifier->OnRailProgress(kinRailState);
			}
			else
			{
				SKinRailState kinRailState;
				kinRailState.bAttached = false;
				kinRailState.bCommited = false;
				kinRailState.arcState.fDenormalizedProgress = 0.f;
				kinRailState.arcState.fProgress = 0.f;
				kinRailState.eGripType = KIN_GRIP_ARC;
				kinRailState.nGripId = (*itAcr).first;
				notifier->OnRailProgress(kinRailState);
			}
		}

		//TIME
		TKINGrips::iterator itTime = m_timeGrips.begin();
		TKINGrips::iterator timeEnd = m_timeGrips.end();
		for (; itTime != timeEnd; ++itTime)
		{
			if (NuiHandlesGripGetIsAttached(&(*itTime).second.nuiGrip))
			{
				NUI_HANDLES_RAILSTATE railState;
				DWORD iRailId;
				NuiHandlesGripGetCommittedRailId( &(*itTime).second.nuiGrip, &iRailId );
				NuiHandlesGripGetRailState( &(*itTime).second.nuiGrip, iRailId, &railState );

				SKinRailState kinRailState;
				kinRailState.bAttached = true;
				kinRailState.bCommited = railState.bCommitted;
				kinRailState.hovertimeState.fPercentageComplete = railState.HoverTimer.fPercentageComplete;
				kinRailState.hovertimeState.fTimePassed = railState.HoverTimer.dbTimePassedInMilliseconds * 0.001f;
				kinRailState.eGripType = KIN_GRIP_HOVERTIME;
				kinRailState.nGripId = (*itTime).first;
				kinRailState.nRailId = (int)iRailId;
				notifier->OnRailProgress(kinRailState);
			}
			else
			{
				SKinRailState kinRailState;
				kinRailState.bAttached = true;
				kinRailState.bCommited = false;
				kinRailState.hovertimeState.fTimePassed = 0.f;
				kinRailState.hovertimeState.fPercentageComplete = 0.f;
				kinRailState.eGripType = KIN_GRIP_HOVERTIME;
				kinRailState.nGripId = (*itTime).first;
				notifier->OnRailProgress(kinRailState);
			}
		}
	}
#endif
}

bool CKinectInput::GetBodySpaceHandles(SKinBodyShapeHandles& bodyShapeHandles)
{
	/*NUI_HANDLES_BODY_SPACE bodySpace;
	HRESULT hr = NuiHandlesArmsGetBodySpace(&m_nuiArms, &bodySpace);
	if (SUCCEEDED(hr))
	{
		bodyShapeHandles.vForward = ConvertXMVectorToVec3(bodySpace.vForward);
		bodyShapeHandles.vLeftShoulder = ConvertXMVectorToVec3(bodySpace.vLeftShoulder);
		bodyShapeHandles.vOrigin = ConvertXMVectorToVec3(bodySpace.vOrigin);
		bodyShapeHandles.vRefinedLeftHand = ConvertXMVectorToVec3(bodySpace.vRefinedLeftHand);
		bodyShapeHandles.vRefinedRightHand = ConvertXMVectorToVec3(bodySpace.vRefinedRightHand);
		bodyShapeHandles.vRight = ConvertXMVectorToVec3( bodySpace.vRight);
		bodyShapeHandles.vRightShoulder = ConvertXMVectorToVec3(bodySpace.vRightShoulder);
		bodyShapeHandles.vUp = ConvertXMVectorToVec3(bodySpace.vUp);

		return true;
	}
	LogKinectError(hr, "NuiHandlesArmsGetBodySpace", 0);*/
	return false;
}

void CKinectInput::EnableSeatedSkeletonTracking(bool bEnableSeatedST)
{
	if (bEnableSeatedST == m_bUseSeatedST)
		return;

#ifdef KINECT_XBOX_CONNECT
	else if (m_pXboxKinectSyncThread)
	{
		m_pXboxKinectSyncThread->SetSeatedST(bEnableSeatedST);
		m_bUseSeatedST = bEnableSeatedST;
	}
#endif
}

bool CKinectInput::GetSkeletonRawData(uint32 iUser, SKinSkeletonRawData &skeletonRawData) const
{
	if((iUser < KIN_SKELETON_COUNT) && (m_skeletonFrame.skeletonRawData[iUser].eTrackingState == KIN_SKELETON_TRACKED))
	{
		skeletonRawData = m_skeletonFrame.skeletonRawData[iUser];

		return true;
	}
	else
	{
		return false;
	}
}

bool CKinectInput::GetSkeletonDefaultData(uint32 iUser, SKinSkeletonDefaultData &skeletonDefaultData) const
{
	if((iUser < KIN_SKELETON_COUNT) && (m_skeletonFrame.skeletonRawData[iUser].eTrackingState == KIN_SKELETON_TRACKED))
	{
		skeletonDefaultData = m_skeletonFrame.skeletonDefaultData[iUser];

		return true;
	}
	else
	{
		return false;
	}
}

void CKinectInput::UpdateSkeletonAlignment(uint32 skeletonIndex)
{
	if((skeletonIndex < KIN_SKELETON_COUNT))
	{
		m_skeletonFrame.skeletonDefaultData[skeletonIndex].vPosition = m_skeletonFrame.skeletonRawData[skeletonIndex].vPosition;
		for(int32 iJoint = 0; iJoint < KIN_SKELETON_POSITION_COUNT; ++iJoint)
			m_skeletonFrame.skeletonDefaultData[skeletonIndex].vSkeletonPositions[iJoint] = m_skeletonFrame.skeletonRawData[skeletonIndex].vSkeletonPositions[iJoint];
	}
}

void CKinectInput::CheckSkeletonPositions()
{
	//Only check the position on the closest tracked skeleton
	if(m_closestSkeletonIndex != KIN_SKELETON_INVALID_TRACKING_ID && m_skeletonFrame.skeletonRawData[m_closestSkeletonIndex].eTrackingState == KIN_SKELETON_TRACKED)
	{
		const Vec4& position = m_skeletonFrame.skeletonRawData[m_closestSkeletonIndex].vPosition;

		float distance = (position - m_lastSkeletonPosition).GetLength();
		//This is in meters
		if(distance > g_pInputCVars->i_kinSkeletonMovedDistance)
		{
			for (TKinectEventListeners::Notifier notifier(m_eventListeners); notifier.IsValid(); notifier.Next())
				notifier->OnKinectSkeletonMoved(m_closestSkeletonIndex, distance);

			m_lastSkeletonPosition = position;
		}
	}
}

/////////////////////////////////////////
// Debug draw

ColorB g_skeletonColors[] = 
{
	ColorB(255, 255, 255, 255),
	ColorB(255, 0, 0, 255),
	ColorB(0, 255, 0, 255),
	ColorB(0, 0, 255, 255),
	ColorB(255, 255, 0, 255),
	ColorB(0, 255, 255, 255),
};

ColorB g_skeletonColorsInferred[] = 
{
	ColorB(128, 128, 128, 128),
	ColorB(128, 0, 0, 128),
	ColorB(0, 128, 0, 128),
	ColorB(0, 0, 128, 128),
	ColorB(128, 128, 0, 128),
	ColorB(0, 128, 128, 128),
};

struct NUI_VISUALIZATION_BONE_JOINTS
{
	NUI_SKELETON_POSITION_INDEX StartJoint;
	NUI_SKELETON_POSITION_INDEX EndJoint;
};

// Define the bones in the skeleton using joint indices
const NUI_VISUALIZATION_BONE_JOINTS g_SkeletonBoneList[] =
{
	// Head
	{ NUI_SKELETON_POSITION_HEAD,            NUI_SKELETON_POSITION_SHOULDER_CENTER },  // Top of head to top of neck

	// Right arm
	{ NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT },   // Neck bottom to right shoulder internal
	{ NUI_SKELETON_POSITION_SHOULDER_RIGHT,  NUI_SKELETON_POSITION_ELBOW_RIGHT },      // Right shoulder internal to right elbow
	{ NUI_SKELETON_POSITION_ELBOW_RIGHT,     NUI_SKELETON_POSITION_WRIST_RIGHT },      // Right elbow to right wrist
	{ NUI_SKELETON_POSITION_WRIST_RIGHT,     NUI_SKELETON_POSITION_HAND_RIGHT },       // Right wrist to right hand

	// Left arm
	{ NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT },    // Neck bottom to left shoulder internal
	{ NUI_SKELETON_POSITION_SHOULDER_LEFT,   NUI_SKELETON_POSITION_ELBOW_LEFT },       // Left shoulder internal to left elbow
	{ NUI_SKELETON_POSITION_ELBOW_LEFT,      NUI_SKELETON_POSITION_WRIST_LEFT },       // Left elbow to left wrist
	{ NUI_SKELETON_POSITION_WRIST_LEFT,      NUI_SKELETON_POSITION_HAND_LEFT },        // Left wrist to left hand

	// Right leg and foot
	{ NUI_SKELETON_POSITION_HIP_RIGHT,       NUI_SKELETON_POSITION_KNEE_RIGHT  },      // Right hip internal to right knee
	{ NUI_SKELETON_POSITION_KNEE_RIGHT,      NUI_SKELETON_POSITION_ANKLE_RIGHT },      // Right knee to right ankle
	{ NUI_SKELETON_POSITION_ANKLE_RIGHT,     NUI_SKELETON_POSITION_FOOT_RIGHT },       // Right ankle to right foot

	// Left leg and foot
	{ NUI_SKELETON_POSITION_HIP_LEFT,        NUI_SKELETON_POSITION_KNEE_LEFT  },       // Left hip internal to left knee
	{ NUI_SKELETON_POSITION_KNEE_LEFT,       NUI_SKELETON_POSITION_ANKLE_LEFT },       // Left knee to left ankle
	{ NUI_SKELETON_POSITION_ANKLE_LEFT,      NUI_SKELETON_POSITION_FOOT_LEFT },        // Left ankle to left foot

	// Spine
	{ NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE },            // Neck bottom to spine
	{ NUI_SKELETON_POSITION_SPINE,           NUI_SKELETON_POSITION_HIP_CENTER },       // Spine to hip center

	// Hips
	{ NUI_SKELETON_POSITION_HIP_RIGHT,       NUI_SKELETON_POSITION_HIP_CENTER },       // Right hip to hip center
	{ NUI_SKELETON_POSITION_HIP_CENTER,      NUI_SKELETON_POSITION_HIP_LEFT }          // Hip center to left hip
};

const UINT g_uSkeletonBoneCount = ARRAYSIZE( g_SkeletonBoneList );


HRESULT CKinectInput::RenderSkeletons( FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight, BOOL bClip, BOOL bRegisterToColor )
{

	for( DWORD dwSkeletonIndex = 0; dwSkeletonIndex < NUI_SKELETON_COUNT; ++ dwSkeletonIndex )
	{
		if( m_skeletonFrame.skeletonRawData[ dwSkeletonIndex ].eTrackingState == NUI_SKELETON_TRACKED )
		{
			RenderSingleSkeleton( dwSkeletonIndex, fX, fY, fWidth, fHeight, bClip, bRegisterToColor );
		}
	}

	return ERROR_SUCCESS;
}


Vec2 g_ScreenSpaceJoints[ NUI_SKELETON_POSITION_COUNT ];

void KIDraw2DLine(float x1, float y1, ColorB color1, float x2, float y2, ColorB color2, float fThickness)
{
	IRenderAuxGeom* pAux = gEnv->pRenderer->GetIRenderAuxGeom();
	x1 /= 800.0f;
	y1 /= 600.0f;
	x2 /= 800.0f;
	y2 /= 600.0f;
	pAux->DrawLine(Vec3(x1, y1, 0.05f), color1, Vec3(x2, y2, 0.05f), color2, fThickness);
}

void KIDrawTriangle(float x1, float y1, ColorB color1, float x2, float y2, ColorB color2, float x3, float y3, ColorB color3)
{
	IRenderAuxGeom* pAux = gEnv->pRenderer->GetIRenderAuxGeom();
	pAux->DrawTriangle(Vec3(x1, y1, 0), color1, Vec3(x2, y2, 0), color2, Vec3(x3, y3, 0), color3);
}

void KIDrawSphere(float x, float y, float radius, ColorB color)
{
	IRenderAuxGeom* pAux = gEnv->pRenderer->GetIRenderAuxGeom();
	pAux->DrawSphere(Vec3(x, y, 0), radius, color, false);
}

HRESULT CKinectInput::RenderSingleSkeleton( DWORD dwSkeletonIndex, FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight, BOOL bClip, BOOL bRegisterToColor )
{

	// There is nothing to draw if the player isn't tracked.
	if ( m_skeletonFrame.skeletonRawData[ dwSkeletonIndex ].eTrackingState != NUI_SKELETON_TRACKED )
	{
		return ERROR_SUCCESS;
	}

	gEnv->pRenderer->SetMaterialColor(1, 1, 1, 1);
	IRenderAuxGeom* pAux = gEnv->pRenderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags flags = pAux->GetRenderFlags();
	flags.SetMode2D3DFlag(e_Mode2D);
	flags.SetDrawInFrontMode(e_DrawInFrontOn);
	flags.SetDepthTestFlag(e_DepthTestOff);
	pAux->SetRenderFlags(flags);

	Vec2 vScreenSpaceJoints[ NUI_SKELETON_POSITION_COUNT ];

	// Pre-compute part of the transformation for speed efficiency.
	FLOAT fHalfWindowWidth = fWidth * 0.5f ;
	FLOAT fHalfWindowHeight = fHeight * 0.5f;    

	FLOAT fXDepthRatioToDisplay = fWidth / 320.0f;
	FLOAT fYDepthRatioToDisplay = fHeight / 240.0f;
	FLOAT fXColorRatioToDisplay = fWidth / 320.0f;
	FLOAT fYColorRatioToDisplay = fWidth / 320.0f;

	// Set up render states

	// Project the world space joints into screen space
	for ( UINT i = 0; i < NUI_SKELETON_POSITION_COUNT; ++ i )
	{
		Vec3 vJointLocation;
		vJointLocation.x = m_skeletonFrame.skeletonRawData[ dwSkeletonIndex ].vSkeletonPositions[ i ].x;
		vJointLocation.y = m_skeletonFrame.skeletonRawData[ dwSkeletonIndex ].vSkeletonPositions[ i ].y;
		vJointLocation.z = m_skeletonFrame.skeletonRawData[ dwSkeletonIndex ].vSkeletonPositions[ i ].z;
		
		// Check for divide by zero
		if ( fabs( vJointLocation.z ) > FLT_EPSILON  )
		{
			// Note:  Without tilt correction, any projection will be off, as the skeleton positions 
			//        are camera-relative, with Up as ( 0, 1, 0), which the axis of the camera may
			//        not be aligned to. You can see this by turning on and off tilt correction
			//        in the sample.

			LONG plDepthX, plDepthY, plColorX, plColorY;
			USHORT usDepthValue;

			Vector4 vPoint;
			vPoint.x = m_skeletonFrame.skeletonRawData[ dwSkeletonIndex ].vSkeletonPositions[ i ].x;
			vPoint.y = m_skeletonFrame.skeletonRawData[ dwSkeletonIndex ].vSkeletonPositions[ i ].y;
			vPoint.z = m_skeletonFrame.skeletonRawData[ dwSkeletonIndex ].vSkeletonPositions[ i ].z;
			vPoint.w = m_skeletonFrame.skeletonRawData[ dwSkeletonIndex ].vSkeletonPositions[ i ].w;

			float fDepthX, fDepthY;
			NuiTransformSkeletonToDepthImageF(vPoint, &fDepthX, &fDepthY, &usDepthValue	);

			plDepthX = LONG(fDepthX * 640);
			plDepthY = LONG(fDepthY * 480);

			NUI_IMAGE_RESOLUTION colorImageResolution = NUI_IMAGE_RESOLUTION_640x480;
			NUI_IMAGE_VIEW_AREA  colorViewArea;

			if ( bRegisterToColor )
			{
				HRESULT hr = NuiImageGetColorPixelCoordinatesFromDepthPixel(
					colorImageResolution,
					&colorViewArea,
					plDepthX,
					plDepthY,
					usDepthValue,
					&plColorX,
					&plColorY);

				if ( SUCCEEDED( hr ) )
				{
					g_ScreenSpaceJoints[i].x = ( (FLOAT) plColorX ) * fXColorRatioToDisplay;
					g_ScreenSpaceJoints[i].y = ( (FLOAT) plColorY ) * fYColorRatioToDisplay;            
				}
				else
				{
					// When a corresponding color value isn't avaible, use the raw depth value instead. 
					g_ScreenSpaceJoints[i].x = ( (FLOAT) plDepthX ) * fXDepthRatioToDisplay;
					g_ScreenSpaceJoints[i].y = ( (FLOAT) plDepthY ) * fYDepthRatioToDisplay;
				}
			}
			else
			{
				g_ScreenSpaceJoints[i].x = ( (FLOAT) plDepthX ) * fXDepthRatioToDisplay;
				g_ScreenSpaceJoints[i].y = ( (FLOAT) plDepthY ) * fYDepthRatioToDisplay;
			}
		}
		else
		{
			// A joint that is so close to the camera that its Z value is 0 can simply be drawn directly 
			// at the center of the 2D plane.
			g_ScreenSpaceJoints[ i ].x = fHalfWindowWidth;
			g_ScreenSpaceJoints[ i ].y = fHalfWindowHeight;
		}
	}

	// Locate the beginning of the confidence array. We'll index into this array in the next loop.
	const KIN_SKELETON_POSITION_TRACKING_STATE *pSkeletonTrackingState = & m_skeletonFrame.skeletonRawData[ dwSkeletonIndex ].eSkeletonPositionTrackingState[ 0 ];

	// Draw each bone in the skeleton using the screen space joints
	for( UINT i = 0; i < g_uSkeletonBoneCount; i++ )
	{
		// Assign a color to each joint based on the confidence level. Don't draw a bone if one of its joints has no confidence.

		ColorB startColor;
		if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].StartJoint ] == NUI_SKELETON_POSITION_NOT_TRACKED )
		{
			// A joint in the bone wasn't tracked during skeleton tracking...
			continue;
		}
		else if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].StartJoint ] == NUI_SKELETON_POSITION_TRACKED )
		{
			startColor = g_skeletonColors[dwSkeletonIndex];
		}
		else if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].StartJoint ] == NUI_SKELETON_POSITION_INFERRED )
		{
			startColor = g_skeletonColorsInferred[dwSkeletonIndex];
		}
		else
		{
			assert( false );
			continue;
		}

		ColorB endColor;
		if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].EndJoint ] == NUI_SKELETON_POSITION_NOT_TRACKED )
		{
			// A joint in the bone wasn't tracked during skeleton tracking...
			continue;
		}
		if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].EndJoint ] == NUI_SKELETON_POSITION_TRACKED )
		{
			endColor = g_skeletonColors[dwSkeletonIndex];
		}
		else if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].EndJoint ] == NUI_SKELETON_POSITION_INFERRED )
		{
			endColor = g_skeletonColorsInferred[dwSkeletonIndex];
		}
		else
		{
			assert( false );
			continue;
		}

		// Vertically center the 1:1 aspect ratio projected skeleton over the destination area.
		// Draw the bone over the depth image
		Vec2 pntArray[ 2 ];
		pntArray[ 0 ].x = g_ScreenSpaceJoints[ g_SkeletonBoneList[ i ].StartJoint ].x + fX;
		pntArray[ 0 ].y = g_ScreenSpaceJoints[ g_SkeletonBoneList[ i ].StartJoint ].y + fY;
		pntArray[ 1 ].x = g_ScreenSpaceJoints[ g_SkeletonBoneList[ i ].EndJoint ].x + fX;
		pntArray[ 1 ].y = g_ScreenSpaceJoints[ g_SkeletonBoneList[ i ].EndJoint ].y + fY;

		KIDraw2DLine(pntArray[ 0 ].x, pntArray[ 0 ].y, startColor, pntArray[ 1 ].x, pntArray[ 1 ].y, endColor, 2.0f);
	}

	return ERROR_SUCCESS;
}

/////////////////////

// Hard code values taken from XDK white paper called: Natural Environs: Understanding and Developing for the Kinect Play Space
const FLOAT SENSOR_H_FOV_HALF_RADIANS = DEG2RAD( NUI_CAMERA_DEPTH_NOMINAL_HORIZONTAL_FOV / 2.0f );
const FLOAT SENSOR_V_FOV_HALF_RADIANS = DEG2RAD( NUI_CAMERA_DEPTH_NOMINAL_VERTICAL_FOV / 2.0f );

const FLOAT PLAYSPACE_MIN_DISTANCE = 0.8f;    // meters
const FLOAT PLAYSPACE_MIN_USABLE_DIST = 1.2f; // meters
const FLOAT PLAYSPACE_MAX_USABLE_DIST = 3.5f; // meters
const FLOAT PLAYSPACE_MAX_DISTANCE = 4.0f;    // meters

const FLOAT PLAYER_RADIUS_BODY = 0.25f;       // Safe distance from edge of usable playspace for a given player in meters
const FLOAT PLAYER_RADIUS = 0.8f;            // Safe distance from edge of usable playspace for a given player in meters

const FLOAT SWEET_SPOT_RADIUS = 0.46f;       // Square of the radius of the "sweet spot" in meters
const FLOAT SWEET_SPOT_Z_DISTANCE = 2.26f;   // Distance of center of sweet spot from sensor array in meters

void CKinectInput::DebugDrawPlaySpace(float originX, float originY, float scale)
{
	gEnv->pRenderer->SetMaterialColor(1, 1, 1, 1);
	IRenderAuxGeom* pAux = gEnv->pRenderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags flags = pAux->GetRenderFlags();
	flags.SetMode2D3DFlag(e_Mode2D);
	flags.SetDrawInFrontMode(e_DrawInFrontOn);
	flags.SetDepthTestFlag(e_DepthTestOff);
	flags.SetCullMode(e_CullModeNone);
	flags.SetDepthWriteFlag(e_DepthWriteOff);
	pAux->SetRenderFlags(flags);

	/*
	// Can't take aspect ratio into account yet, because spheres will still be deformed
	// and then triangle is correct, but spheres not and can give a strange impression
	// would need support to render ellipses or something
	const int screenWidth = gEnv->pRenderer->GetWidth();
	const int screenHeight = gEnv->pRenderer->GetHeight();

	const float fAspect = (float)screenWidth / (float)screenHeight;
	*/

	float fHalfWidthDistUnit = tanf(SENSOR_H_FOV_HALF_RADIANS);

	float fHalfWidthMinDist = fHalfWidthDistUnit * PLAYSPACE_MIN_DISTANCE / PLAYSPACE_MAX_DISTANCE * scale;// / fAspect;
	float fHalfWidthMinUsableDist = fHalfWidthDistUnit * PLAYSPACE_MIN_USABLE_DIST / PLAYSPACE_MAX_DISTANCE * scale;// / fAspect;
	float fHalfWidthMaxUsableDist = fHalfWidthDistUnit * PLAYSPACE_MAX_USABLE_DIST / PLAYSPACE_MAX_DISTANCE * scale;// / fAspect;
	float fHalfWidthMaxDist = fHalfWidthDistUnit * scale;// / fAspect;
	
	float fDistanceMin = PLAYSPACE_MIN_DISTANCE / PLAYSPACE_MAX_DISTANCE * scale;
	float fDistanceMinUsable = PLAYSPACE_MIN_USABLE_DIST / PLAYSPACE_MAX_DISTANCE * scale;
	float fDistanceMaxUsable = PLAYSPACE_MAX_USABLE_DIST / PLAYSPACE_MAX_DISTANCE * scale;
	float fDistanceMax = PLAYSPACE_MAX_DISTANCE / PLAYSPACE_MAX_DISTANCE * scale;
	
	// draw the camera playspace
	KIDrawTriangle(originX, originY, Col_DarkGreen, originX - fHalfWidthMaxDist, originY + fDistanceMax, Col_DarkGreen, originX + fHalfWidthMaxDist, originY + fDistanceMax, Col_DarkGreen);
	KIDrawTriangle(originX, originY, Col_Green, originX - fHalfWidthMaxUsableDist, originY + fDistanceMaxUsable, Col_Green, originX + fHalfWidthMaxUsableDist, originY + fDistanceMaxUsable, Col_Green);
	KIDrawTriangle(originX, originY, Col_DarkGreen, originX - fHalfWidthMinUsableDist, originY + fDistanceMinUsable, Col_DarkGreen, originX + fHalfWidthMinUsableDist, originY + fDistanceMinUsable, Col_DarkGreen);
	KIDrawTriangle(originX, originY, Col_Gray, originX - fHalfWidthMinDist, originY + fDistanceMin, Col_Gray, originX + fHalfWidthMinDist, originY + fDistanceMin, Col_Gray);

	// draw the sweetspot area (in GOLD!!)
	float fSpeetSpotDistance = SWEET_SPOT_Z_DISTANCE / PLAYSPACE_MAX_DISTANCE * scale;
	float fSweetSpotRadius = SWEET_SPOT_RADIUS / PLAYSPACE_MAX_DISTANCE * scale;
	KIDrawSphere(originX, originY + fSpeetSpotDistance, fSweetSpotRadius, Col_Gold);

	float fCharacterRadius = PLAYER_RADIUS_BODY / PLAYSPACE_MAX_DISTANCE * scale;

	// draw the current character positions
	for( DWORD dwSkeletonIndex = 0; dwSkeletonIndex < NUI_SKELETON_COUNT; ++ dwSkeletonIndex )
	{
		if( m_skeletonFrame.skeletonRawData[ dwSkeletonIndex ].eTrackingState == NUI_SKELETON_TRACKED )
		{
			Vec3 vLocation2D = Vec3(m_skeletonFrame.skeletonRawData[dwSkeletonIndex].vPosition.x, 
				m_skeletonFrame.skeletonRawData[dwSkeletonIndex].vPosition.y, 
				m_skeletonFrame.skeletonRawData[dwSkeletonIndex].vPosition.z);	

			float fCharacterDepth = vLocation2D.z / PLAYSPACE_MAX_DISTANCE * scale;
			float fCharacterOffset = vLocation2D.x / PLAYSPACE_MAX_DISTANCE * scale;

			KIDrawSphere(originX + fCharacterOffset, originY + fCharacterDepth, fCharacterRadius, g_skeletonColors[dwSkeletonIndex]);
		}
	}
}

void CKinectInput::DebugDraw()
{
	if(!g_pInputCVars->i_kinectDebug)
		return;

	static const FLOAT drawWidth = 160.0f;
	static const FLOAT drawHeight = 120.0f;
	static const FLOAT drawX = 0.0f;//475.0f;
	static const FLOAT drawY = 45.0f;

	if(g_pInputCVars->i_kinectDebug & 2)
	{
		if (SUCCEEDED(m_hrImage) && m_pImageFrame)
		{
			KINECT_LOCKED_RECT LockedSrc;
			DWORD flags = 0;

			if( SUCCEEDED( m_pImageFrame->pFrameTexture->LockRect( 0, &LockedSrc, NULL, flags ) ) )
			{
				gEnv->pRenderer->UpdateTextureInVideoMemory(m_iImage, (unsigned char*)LockedSrc.pBits, 0,0,640, 480, eTF_X8R8G8B8);
				m_pImageFrame->pFrameTexture->UnlockRect(0);
			}
		}

		gEnv->pRenderer->SetState(GS_BLSRC_ONE | GS_BLDST_ZERO | GS_NODEPTHTEST);
		gEnv->pRenderer->Push2dImage(drawX, drawY, drawWidth, drawHeight, m_iImage, 0.0f, 0.0f, 1.0f, -1.0f,0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.03f, 0.0f);


		if (SUCCEEDED(m_hr320x240) && m_pDepthFrame320x240)
		{
			KINECT_LOCKED_RECT LockedSrc;
			DWORD flags = 0;

			if( SUCCEEDED( m_pDepthFrame320x240->pFrameTexture->LockRect( 0, &LockedSrc, NULL, flags ) ) )
			{

				// Fill in the depthmap data
				std::vector<DWORD> tmp(320*240);

				DWORD* lpBits = ( DWORD* )&tmp[0];
				USHORT* pDepthMapCur = ( USHORT* )LockedSrc.pBits;

				const static FLOAT fMaxDepth = 8000.0f;                           // use 8 meters as max depth to colorize
				const static UINT uNormalize = (UINT) ceil( fMaxDepth / 255.0 );

				for( UINT y = 0; y < 240; ++ y )
				{
					for( UINT x = 0; x < 320; ++ x )
					{
						// To colorize the depth values, normalize the depth value against a maximum depth
						// value to be colorized and lookup into the color table
						UINT uIndex = 255 - min( (int)(( pDepthMapCur[ x ] >> 3 ) / uNormalize), 255 );
						if(uIndex == 255)
							uIndex = 0;

						ColorB col(uIndex, uIndex, uIndex, 255);
						lpBits[x] =  col.pack_argb8888();
					}
					lpBits += 320;
					pDepthMapCur += LockedSrc.Pitch / sizeof( USHORT );
				}
				gEnv->pRenderer->UpdateTextureInVideoMemory(m_iDepth320x240, (unsigned char*)&tmp[0], 0,0,320, 240, eTF_X8R8G8B8);
				m_pDepthFrame320x240->pFrameTexture->UnlockRect(0);
			}
		}
		gEnv->pRenderer->SetState(GS_BLSRC_ONE | GS_BLDST_ZERO | GS_NODEPTHTEST);
		gEnv->pRenderer->Push2dImage(drawX + 165, drawY, drawWidth, drawHeight, m_iDepth320x240, 0.0f, 0.0f, 1.0f, -1.0f,0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.03f, 0.0f);
	}

	if(g_pInputCVars->i_kinectDebug & 1)
		RenderSkeletons(drawX, drawY, drawWidth, drawHeight, false, false);

	if(g_pInputCVars->i_kinectDebug & 4)
		DebugDrawPlaySpace(0.5f, 0.2f, 0.6f);
	else if(g_pInputCVars->i_kinectDebug & 8)
		DebugDrawPlaySpace(0.8f, 0.8f, 0.15f);
}

void CKinectInput::EnableWaveGestureTracking(bool bEnable)
{
#ifdef KINECT_USE_GESTURES
	HRESULT hr = NuiWaveSetEnabled(bEnable);
	if (FAILED(hr))
	{
		LogKinectError(hr, "EnableWaveGestureTracking", 0);
	}
#endif
}
float CKinectInput::GetWaveGestureProgress(DWORD* pTrackingId)
{
#ifdef KINECT_USE_GESTURES
	float fProgress;
	HRESULT hr = NuiWaveGetGestureOwnerProgress(pTrackingId, &fProgress);
	if (FAILED(hr))
	{
		LogKinectError(hr, "GetWaveGestureProgress", 0);
	}
	return fProgress;
#else
	return 0.0f;
#endif
}

bool CKinectInput::IdentityDetectedIntentToPlay(DWORD dwTrackingId)
{
#ifdef KINECT_USE_IDENTITY
	HRESULT hr = NuiIdentityDetectedIntentToPlay(dwTrackingId);
	if (FAILED(hr))
	{
		LogKinectError(hr, "IdentityDetectedIntentToPlay", 0);
		return false;
	}
#endif
	return true;
}

bool CKinectInput::IdentityIdentify(DWORD dwTrackingId, KinIdentifyCallback callbackFunc, void* pData)
{
#ifdef KINECT_USE_IDENTITY
	HRESULT hr = NuiIdentityIdentify(dwTrackingId, 0, NuiIdentityCallBack, this);
	
	switch(hr)
	{
	case E_INVALIDARG:
	case E_ABORT:
	case E_NUI_IDENTITY_LOST_TRACK:
	case E_NUI_IDENTITY_QUALITY_ISSUE:
	case E_NUI_DEVICE_NOT_CONNECTED:
	case E_NUI_IDENTITY_BUSY:
		return false;
	}

	m_callbackFunc = callbackFunc;
	m_pCallbackData = pData;
#endif
	return true;
}

#ifdef KINECT_USE_IDENTITY
bool CKinectInput::OnIdentityCallBack(NUI_IDENTITY_MESSAGE* pMessage)
{
	//Convert from NUI_IDENTITY_MESSAGE to SKinIdentityMessage
	SKinIdentityMessage outMessage;
	switch(pMessage->MessageId)
	{
	case NUI_IDENTITY_MESSAGE_ID_FRAME_PROCESSED:
		outMessage.messageId = E_KinIdentityMessageId_Frame_Processed;
		break;
	case E_KinIdentityMessageId_Id_Complete:
		outMessage.messageId = E_KinIdentityMessageId_Id_Complete;
		break;
	}

	switch(pMessage->OperationId)
	{
	case NUI_IDENTITY_OPERATION_ID_NONE:
		outMessage.operationId = E_KinIdentityOperationId_None;
		break;
	case NUI_IDENTITY_OPERATION_ID_IDENTIFY:
		outMessage.operationId = E_KinIdentityOperationId_Identify;
		break;
	case NUI_IDENTITY_OPERATION_ID_ENROLL:
		outMessage.operationId = E_KinIdentityOperationId_Enroll;
		break;
	case NUI_IDENTITY_OPERATION_ID_TUNER:
		outMessage.operationId = E_KinIdentityOperationId_Tuner;
		break;
	}
	
	outMessage.dwTrackingId = pMessage->dwTrackingID;
	outMessage.dwSkeletonFrameNumber = pMessage->dwSkeletonFrameNumber;
	outMessage.Data.frameProcessed.dwQualityFlags = pMessage->Data.FrameProcessed.dwQualityFlags;
	outMessage.Data.complete.bProfileMatched = pMessage->Data.Complete.bProfileMatched;
	outMessage.Data.complete.dwEnrollementIndex = pMessage->Data.Complete.dwEnrollmentIndex;
	switch(pMessage->Data.Complete.hrResult)
	{
	case S_OK:
			outMessage.Data.complete.eResult = EKinIdentityMessageCompleteResult_Ok;
		break;
	case E_NUI_IDENTITY_BUSY:
		 outMessage.Data.complete.eResult = EKinIdentityMessageCompleteResult_Busy;
		break;
	case E_NUI_IDENTITY_LOST_TRACK:
		outMessage.Data.complete.eResult = EKinIdentityMessageCompleteResult_LostTrack;
		break;
	case E_NUI_IDENTITY_QUALITY_ISSUE:
		outMessage.Data.complete.eResult = EKinIdentityMessageCompleteResult_QualityIssue;
		break;
	case E_ABORT:
		outMessage.Data.complete.eResult = EKinIdentityMessageCompleteResult_Abort;
		break;
	}

	// Post message
	return m_callbackFunc(outMessage, m_pCallbackData);
}
#endif

bool CKinectInput::SpeechEnable()
{
#ifdef KINECT_USE_SPEECH
	NUI_SPEECH_INIT_PROPERTIES initProp;
	initProp.Language = NUI_SPEECH_LANGUAGE_EN_US;// must match the xml:lang in thr grxml file
	initProp.MicrophoneType = NUI_SPEECH_KINECT;

	HRESULT hr = NuiSpeechEnable(&initProp, NUI_INITIALIZE_DEFAULT_HARDWARE_THREAD);
	if (FAILED(hr))
	{
		LogKinectError(hr, "SpeechEnable", 0);
		return false;
	}
#endif
	return true;
}

void CKinectInput::SpeechDisable()
{
#ifdef KINECT_USE_SPEECH
	NuiSpeechDisable();
#endif
}

bool CKinectInput::KinSpeechSetEventInterest(unsigned long ulEvents)
{
#ifdef KINECT_USE_SPEECH
	HRESULT hr = NuiSpeechSetEventInterest(ulEvents);
	if (FAILED(hr))
	{
		LogKinectError(hr, "KinSpeechSetEventInterest", 0);
		return false;
	}
#endif
	return true;
}

bool CKinectInput::KinSpeechLoadDefaultGrammar()
{
#ifdef KINECT_USE_SPEECH
	HRESULT hr2 = NuiSpeechStopRecognition();

	HRESULT hr = NuiSpeechLoadGrammar(L"game:\\underwaterGrammar.cgf", 55, NUI_SPEECH_LOADOPTIONS_DYNAMIC, &m_Grammar);
	if (FAILED(hr))
	{
		LogKinectError(hr, "KinSpeechLoadDefaultGrammar", 0);
		return false;
	}
#endif
	return true;
}

bool CKinectInput::KinSpeechStartRecognition()
{
#ifdef KINECT_USE_SPEECH
	HRESULT hr = NuiSpeechStartRecognition();
	if (FAILED(hr))
	{
		LogKinectError(hr, "KinSpeechStartRecognition", 0);
		return false;
	}
#endif
	return true;
}

void CKinectInput::KinSpeechStopRecognition()
{
#ifdef KINECT_USE_SPEECH
	NuiSpeechStopRecognition();
#endif
}

#ifdef KINECT_USE_SPEECH
void CKinectInput::OnSpeechEvent(NUI_SPEECH_EVENT event)
{
	string outputString;
	if (event.eventId == NUI_SPEECH_EVENT_RECOGNITION && event.pResult)
	{
		if (event.pResult->Phrase.pSemanticProperties)
		{
			for( unsigned long i = 0; i < event.pResult->Phrase.Rule.ulCountOfElements; i++ )
			{
				const NUI_SPEECH_ELEMENT* pElt = &event.pResult->Phrase.pElements[i];
				if (event.pResult->Phrase.pSemanticProperties->fSREngineConfidence > 0.80f)
				{
					char buffer[250];
					size_t convSize;

					wcstombs_s(&convSize, buffer, 250, pElt->pcwszLexicalForm, 250);
					outputString.append(buffer);
				}
			}
			/*string conf;
			conf.Format("property %0.2f, rule %0.2f", event.pResult->Phrase.pSemanticProperties->fSREngineConfidence * 100.f, event.pResult->Phrase.Rule.fSREngineConfidence * 100.f);
			outputString.append(conf.c_str());*/
		}
		else
		{
			for( unsigned long i = 0; i < event.pResult->Phrase.Rule.ulCountOfElements; i++ )
			{
				const NUI_SPEECH_ELEMENT* pElt = &event.pResult->Phrase.pElements[i];
				if (event.pResult->Phrase.Rule.fSREngineConfidence > 0.80f)
				{
					char buffer[250];
					size_t convSize;

					wcstombs_s(&convSize, buffer, 250, pElt->pcwszLexicalForm, 250);
					outputString.append(buffer);
					outputString.append(" ");
				}
			}
		}
	}

	for (TKinectEventListeners::Notifier notifier(m_eventListeners); notifier.IsValid(); notifier.Next())
	{
		notifier->OnVoiceCommand(outputString);
	}
}

#endif

#endif	// USE_KINECT
