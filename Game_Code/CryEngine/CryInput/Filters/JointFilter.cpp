//--------------------------------------------------------------------------------------
// ADAPTED from Microsoft's provided sample AtgNuiJointFilter.cpp
//
// This file contains various filters for filtering Joints
//
// Microsoft Game Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "StdAfx.h"

#if defined(USE_KINECT)

#include "JointFilter.h"


#include <IConsole.h>

//--------------------------------------------------------------------------------------
// The Blend Update function adds a new frame and then creates an average by summing and dividing the new joints
// This could be done without storing 10 frames of history if memory is a concern.
//--------------------------------------------------------------------------------------
void FilterBlendJoint::Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime )
{
    
    memcpy( &m_pSkeletonsData[ m_iCurrentFrame ], pSkeletonData.vSkeletonPositions, sizeof( FilterSkeletonData ) );
    ++ m_iCurrentFrame;
    m_iCurrentFrame %= m_nFramesToAverage;

    memset( &m_FilteredSkeleton, 0, sizeof( FilterSkeletonData ) );
    for ( int index=0; index < m_nFramesToAverage; ++index )
    {
        for ( int iCount = 0; iCount < KIN_SKELETON_POSITION_COUNT; ++iCount )
        {
            m_FilteredSkeleton.m_Positions[iCount] += m_pSkeletonsData[index].m_Positions[iCount];
        }
    }
    for ( int iCount = 0; iCount < KIN_SKELETON_POSITION_COUNT; ++iCount )
    {
        m_FilteredSkeleton.m_Positions[iCount] /= (float)m_nFramesToAverage;
    }
    

}
//--------------------------------------------------------------------------------------
// Return a pointer to the filtered joints
//--------------------------------------------------------------------------------------

Vec4* FilterBlendJoint::GetFilteredJoints( )
{
    return &m_FilteredSkeleton.m_Positions[0];
}

//--------------------------------------------------------------------------------------
// The VelDamp function estimates velocity as part of the smoothing function
// 
//--------------------------------------------------------------------------------------
void FilterVelDamp::Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime )
{
    if ( m_iPreviousFrame == -1 )
    {
        memcpy( &m_SkeletonData[0], pSkeletonData.vSkeletonPositions, sizeof( FilterSkeletonData ) );
        m_iPreviousFrame = 1;

	// Redundancy to ensure both previous and current frames are checked
	if ( m_iCurrentFrame == -1 )
	{
		memcpy( &m_SkeletonData[1], pSkeletonData.vSkeletonPositions, sizeof( FilterSkeletonData ) );
		m_iCurrentFrame = 0;
	}
    }
    else if ( m_iCurrentFrame == -1 )
    {
        memcpy( &m_SkeletonData[1], pSkeletonData.vSkeletonPositions, sizeof( FilterSkeletonData ) );
        m_iCurrentFrame = 0;    
    }
    else 
    {
        ++m_iPreviousFrame;
        m_iPreviousFrame%=2;
        ++m_iCurrentFrame;
        m_iCurrentFrame%=2;
        memcpy( &m_SkeletonData[m_iCurrentFrame], pSkeletonData.vSkeletonPositions, sizeof( FilterSkeletonData ) );
        
        static const float  fAlphaCoeff = 0.4f;
        static const float  fBetaCoeff = ( fAlphaCoeff * fAlphaCoeff ) / ( 2.0f - fAlphaCoeff );
        
        FilterSkeletonData* pPreviousSkeleton = &m_SkeletonData[m_iPreviousFrame];
        FilterSkeletonData* pCurrentSkeleton = &m_SkeletonData[m_iCurrentFrame];
        FilterVelocitySingleEstimate* pPreviousEstimate = &m_VelocityEstimate[m_iPreviousFrame];
        FilterVelocitySingleEstimate* pCurrentEstimate = &m_VelocityEstimate[m_iCurrentFrame];

        Vec4            vPredicted;
        Vec4            vError;

        for ( int joint = 0; joint < KIN_SKELETON_POSITION_COUNT ; ++joint )
        {
            // Calculate vPredicted position using last frames corrected position and velocity.
            vPredicted = pPreviousSkeleton->m_Positions[ joint ] + pPreviousEstimate->m_vEstVel[ joint ] ;

            // Calculate vError between vPredicted and measured position.
            vError = pCurrentSkeleton->m_Positions[ joint ] - vPredicted;

            // Calculate corrected position.
            pCurrentSkeleton->m_Positions[ joint ] = vPredicted + (vError * fAlphaCoeff );

            // Calculate corrected velocity.
            pCurrentEstimate->m_vEstVel[ joint ]= pPreviousEstimate->m_vEstVel[ joint ] + (vError * fBetaCoeff );
        }

    }
}
//--------------------------------------------------------------------------------------
// Return the filtered joints.
//--------------------------------------------------------------------------------------

Vec4* FilterVelDamp::GetFilteredJoints( )
{
    return &m_SkeletonData[m_iCurrentFrame].m_Positions[0];
}

//--------------------------------------------------------------------------------------
// if joint is 0 it is not valid.
//--------------------------------------------------------------------------------------
inline BOOL JointPositionIsValid(Vec4 vJointPosition)
{
    return (vJointPosition.x != 0.0f ||
            vJointPosition.y != 0.0f ||
            vJointPosition.z != 0.0f);
}



// Description of connected joints
typedef struct Bone
{
    KIN_SKELETON_POSITION_INDEX startJoint;
    KIN_SKELETON_POSITION_INDEX endJoint;
} Bone;


// Define the bones in the skeleton using joint indices
const Bone g_Bones[] =
{
    // Spine
    {  KIN_SKELETON_POSITION_HIP_CENTER, KIN_SKELETON_POSITION_SPINE  },                // Spine to hip center
    {  KIN_SKELETON_POSITION_SPINE, KIN_SKELETON_POSITION_SHOULDER_CENTER },            // Neck bottom to spine

    // Head
    { KIN_SKELETON_POSITION_SHOULDER_CENTER, KIN_SKELETON_POSITION_HEAD  },             // Top of head to top of neck

    // Right arm
    { KIN_SKELETON_POSITION_SHOULDER_CENTER, KIN_SKELETON_POSITION_SHOULDER_RIGHT },    // Neck bottom to right shoulder internal
    { KIN_SKELETON_POSITION_SHOULDER_RIGHT, KIN_SKELETON_POSITION_ELBOW_RIGHT },        // Right shoulder internal to right elbow
    { KIN_SKELETON_POSITION_ELBOW_RIGHT, KIN_SKELETON_POSITION_WRIST_RIGHT },           // Right elbow to right wrist
    { KIN_SKELETON_POSITION_WRIST_RIGHT, KIN_SKELETON_POSITION_HAND_REFINED_RIGHT },            // Right wrist to right hand

    // Left arm
    { KIN_SKELETON_POSITION_SHOULDER_CENTER, KIN_SKELETON_POSITION_SHOULDER_LEFT },     // Neck bottom to left shoulder internal
    { KIN_SKELETON_POSITION_SHOULDER_LEFT, KIN_SKELETON_POSITION_ELBOW_LEFT },          // Left shoulder internal to left elbow
    { KIN_SKELETON_POSITION_ELBOW_LEFT, KIN_SKELETON_POSITION_WRIST_LEFT },             // Left elbow to left wrist
    { KIN_SKELETON_POSITION_WRIST_LEFT, KIN_SKELETON_POSITION_HAND_LEFT },              // Left wrist to left hand
    { KIN_SKELETON_POSITION_WRIST_LEFT, KIN_SKELETON_POSITION_HAND_REFINED_LEFT },              // Left wrist to left hand

    // Right leg and foot
    { KIN_SKELETON_POSITION_HIP_RIGHT, KIN_SKELETON_POSITION_KNEE_RIGHT },              // Right hip internal to right knee
    { KIN_SKELETON_POSITION_KNEE_RIGHT, KIN_SKELETON_POSITION_ANKLE_RIGHT },            // Right knee to right ankle

    { KIN_SKELETON_POSITION_ANKLE_RIGHT, KIN_SKELETON_POSITION_FOOT_RIGHT },            // Left ankle to left foot 

    // Left leg and foot
    { KIN_SKELETON_POSITION_HIP_LEFT, KIN_SKELETON_POSITION_KNEE_LEFT },                // Left hip internal to left knee
    { KIN_SKELETON_POSITION_KNEE_LEFT, KIN_SKELETON_POSITION_ANKLE_LEFT },              // Left knee to left ankle

    { KIN_SKELETON_POSITION_ANKLE_LEFT, KIN_SKELETON_POSITION_FOOT_LEFT },              // Left ankle to left foot 

    // Hips
    { KIN_SKELETON_POSITION_HIP_CENTER, KIN_SKELETON_POSITION_HIP_RIGHT },              // Right hip to hip center
    { KIN_SKELETON_POSITION_HIP_CENTER, KIN_SKELETON_POSITION_HIP_LEFT }                // Hip center to left hip

};

const uint32 g_numBones = ARRAYSIZE( g_Bones );

//--------------------------------------------------------------------------------------
// Name: CombiJointFilter()
// Desc: A filter for the positional data.  This filter uses a combination of velocity 
//       position history to filter the joint positions.
//--------------------------------------------------------------------------------------
void FilterCombination::Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime )
{
    // Process each joint
    for ( uint32 nJoint = 0; nJoint < KIN_SKELETON_POSITION_COUNT; ++nJoint )
    {
        // Remember where the camera thinks this joint should be
        m_History[ nJoint ].m_vWantedPos = pSkeletonData.vSkeletonPositions[ nJoint ];

        Vec4 vDelta;
        vDelta = m_History[ nJoint ].m_vWantedPos - m_History[ nJoint ].m_vLastWantedPos;
        {
            Vec4 vBlended;

            // Calculate the vBlended value - could optimize this by remembering the running total and
            // subtracting the oldest value and then adding the newest. Saves adding them all up on each frame.
            vBlended = Vec4(0,0,0,0);
            for( uint32 k = 0; k < m_nUseTaps; ++k)
            {
                vBlended = vBlended + m_History[ nJoint ].m_vPrevDeltas[k];
            }
            vBlended = vBlended / ((float)m_nUseTaps);

						vBlended.w = 0.0f;
						vDelta.w = 0.0f;

            float fDeltaLength = vDelta.GetLength();
            float fBlendedLength = vBlended.GetLength();
            m_History[ nJoint ].m_fWantedLocalBlendRate = m_fDefaultApplyRate;
            m_History[ nJoint ].m_bActive[0] = false;
            m_History[ nJoint ].m_bActive[1] = false;
            m_History[ nJoint ].m_bActive[2] = false;

            // Does the current velocity and history have a reasonable magnitude?
            if( fDeltaLength   >= m_fDeltaLengthThreshold &&
                fBlendedLength >= m_fBlendedLengthThreshold )
            {
                float fDotProd;
                float fConfidence;

                if( m_bDotProdNormalize )
                {
                    Vec4 vDeltaOne = vDelta;
										vDeltaOne.Normalize();
                    Vec4 vBlendedOne = vBlended;
										vBlendedOne.Normalize();

                    fDotProd = vDeltaOne.Dot( vBlendedOne );
                }
                else
                {
										fDotProd = vDelta.Dot(vBlended);
                }

                // Is the current frame aligned to the recent history?
                if( fDotProd >= m_fDotProdThreshold )
                {
                    fConfidence = fDotProd;
                    m_History[ nJoint ].m_fWantedLocalBlendRate = min( fConfidence, 1.0f );
                    m_History[ nJoint ].m_bActive[0] = true;
                }
            }

            assert( m_History[ nJoint ].m_fWantedLocalBlendRate <= 1.0f );
        }

        // Push the previous deltas down the history
        for( int j = m_nUseTaps-2; j >= 0; --j )
        {
            m_History[ nJoint ].m_vPrevDeltas[j+1] = m_History[ nJoint ].m_vPrevDeltas[j];
        }

        // Store the current history
        m_History[ nJoint ].m_vPrevDeltas[0] = vDelta;	

        // Remember where the camera thought this joint was on the this frame
        m_History[ nJoint ].m_vLastWantedPos = m_History[ nJoint ].m_vWantedPos;
    }

    // Secondary and tertiary blending
    for ( uint32 pass = 0; pass < 2; ++pass )
    {
        for ( uint32 bone = 0; bone < g_numBones; ++bone )
        {
            float fRate1;
            float fRate2;

            fRate1 = m_History[ g_Bones[bone].startJoint ].m_fWantedLocalBlendRate;
            fRate2 = m_History[ g_Bones[bone].endJoint ].m_fWantedLocalBlendRate;

            // Blend down? Start to end
            if( (fRate1 * m_fDownBlendRate) > fRate2)
            {
                // Yes, apply
                m_History[ g_Bones[bone].endJoint ].m_fWantedLocalBlendRate = ( fRate1 * m_fDownBlendRate );

                // Flag
                m_History[ g_Bones[bone].endJoint ].m_bActive[pass+1] = true;
            }
            // Blend down? End to start
            if( ( fRate2 * m_fDownBlendRate ) > fRate1)
            {
                // Yes, apply
                m_History[ g_Bones[bone].startJoint ].m_fWantedLocalBlendRate = ( fRate2 * m_fDownBlendRate );

                // Flag
                m_History[ g_Bones[bone].startJoint ].m_bActive[pass+1] = true;
            }
        }
    }

    // Apply
    for ( uint32 joint = 0; joint < KIN_SKELETON_POSITION_COUNT; ++joint )
    {
        // Blend the blend rate
        m_History[ joint ].m_fActualLocalBlendRate = 
                Lerp(m_History[ joint ].m_fActualLocalBlendRate,
                     m_History[ joint ].m_fWantedLocalBlendRate,
                     m_fBlendBlendRate);

        // Blend the actual position towards the wanted positon
        m_History[ joint ].m_vPos = 
                Lerp(m_History[ joint ].m_vPos,
                             m_History[ joint ].m_vWantedPos,
                             m_History[ joint ].m_fActualLocalBlendRate);
        m_FilteredJoints[ joint ] = m_History[ joint ].m_vPos;
    }
}


//--------------------------------------------------------------------------------------
// Implementation of a Holt Double Exponential Smoothing filter. The double exponential
// smooths the curve and predicts.  There is also noise jitter removal. And maximum
// prediction bounds.  The paramaters are commented in the init function.
//--------------------------------------------------------------------------------------
void FilterDoubleExponential::Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime )
{
		//If parameters have not been manually set, then apply cvar based global ones
		if(!m_paramsSet)
		{
			if(gEnv->pConsole)
			{
				m_params = KIN_TRANSFORM_SMOOTH_PARAMETERS(gEnv->pConsole->GetCVar("i_kinGlobalExpSmoothFactor")->GetFVal(),
					gEnv->pConsole->GetCVar("i_kinGlobalExpCorrectionFactor")->GetFVal(),
					gEnv->pConsole->GetCVar("i_kinGlobalExpPredictionFactor")->GetFVal(),
					gEnv->pConsole->GetCVar("i_kinGlobalExpJitterRadius")->GetFVal(),
					gEnv->pConsole->GetCVar("i_kinGlobalExpDeviationRadius")->GetFVal());
			}
		}

    for (uint32 i = 0; i < KIN_SKELETON_POSITION_COUNT; i++)
    {
		    KIN_TRANSFORM_SMOOTH_PARAMETERS SmoothingParams = m_params;

        // If inferred, we smooth a bit more by using a bigger jitter radius
        if (pSkeletonData.eSkeletonPositionTrackingState[i] == KIN_SKELETON_POSITION_INFERRED)
        {
            SmoothingParams.m_fJitterRadius       *= 2.0f;
            SmoothingParams.m_fMaxDeviationRadius *= 2.0f;
        }

        Update( pSkeletonData, i, SmoothingParams );
    }
}

void FilterDoubleExponential::Update( const SKinSkeletonRawData& pSkeletonData, uint32 i, const KIN_TRANSFORM_SMOOTH_PARAMETERS& smoothingParams )
{
    Vec4 vPrevRawPosition;
    Vec4 vPrevFilteredPosition;
    Vec4 vPrevTrend;
    Vec4 vRawPosition;
    Vec4 vFilteredPosition;
    Vec4 vPredictedPosition;
    Vec4 vDiff;
    Vec4 vTrend;
    Vec4 vLength;
    float fDiff;
    BOOL bJointIsValid;

    const Vec4* __restrict pJointPositions = pSkeletonData.vSkeletonPositions;

    vRawPosition            = pJointPositions[i];
    vPrevFilteredPosition   = m_History[i].m_vFilteredPosition;
    vPrevTrend              = m_History[i].m_vTrend;
    vPrevRawPosition        = m_History[i].m_vRawPosition;
    bJointIsValid           = JointPositionIsValid(vRawPosition);

    // If joint is invalid, reset the filter
    if (!bJointIsValid)
    {
        m_History[i].m_dwFrameCount = 0;
    }

    // Initial start values
    if (m_History[i].m_dwFrameCount == 0)
    {
        vFilteredPosition = vRawPosition;
        vTrend = Vec4(0,0,0,0);
        m_History[i].m_dwFrameCount++;
    }
    else if (m_History[i].m_dwFrameCount == 1)
    {
        vFilteredPosition = (vRawPosition + vPrevRawPosition) * 0.5f;
        vDiff = vFilteredPosition - vPrevFilteredPosition;
        vTrend = (vDiff * smoothingParams.m_fCorrection) + (vPrevTrend *  (1.0f - smoothingParams.m_fCorrection));
        m_History[i].m_dwFrameCount++;
    }
    else
    {              
        // First apply jitter filter
        vDiff = vRawPosition - vPrevFilteredPosition;
        fDiff = fabs(vDiff.GetLength());

        if (fDiff <= smoothingParams.m_fJitterRadius)
        {
            vFilteredPosition = vRawPosition * (fDiff/smoothingParams.m_fJitterRadius) + vPrevFilteredPosition * (1.0f - fDiff/smoothingParams.m_fJitterRadius);
        }
        else
        {
            vFilteredPosition = vRawPosition;
        }

        // Now the double exponential smoothing filter
        vFilteredPosition = vFilteredPosition * (1.0f - smoothingParams.m_fSmoothing) + (vPrevFilteredPosition + vPrevTrend) * smoothingParams.m_fSmoothing;


        vDiff = vFilteredPosition - vPrevFilteredPosition;
        vTrend = vDiff * smoothingParams.m_fCorrection + vPrevTrend * (1.0f - smoothingParams.m_fCorrection); 
    }      

    // Predict into the future to reduce latency
    vPredictedPosition = vFilteredPosition + (vTrend * smoothingParams.m_fPrediction);

    // Check that we are not too far away from raw data
    vDiff = vPredictedPosition - vRawPosition;
    fDiff = fabs(vDiff.GetLength());

    if (fDiff > smoothingParams.m_fMaxDeviationRadius)
    {
        vPredictedPosition = vPredictedPosition * smoothingParams.m_fMaxDeviationRadius/fDiff + vRawPosition * (1.0f - smoothingParams.m_fMaxDeviationRadius/fDiff);
    }

    // Save the data from this frame
    m_History[i].m_vRawPosition      = vRawPosition;
    m_History[i].m_vFilteredPosition = vFilteredPosition;
    m_History[i].m_vTrend            = vTrend;
    
    // Output the data
    m_FilteredJoints[i] = vPredictedPosition;
    m_FilteredJoints[i].w = 1.0f;
}


//--------------------------------------------------------------------------------------
// The Taylor Series smooths and removes jitter based on a taylor series expansion
//--------------------------------------------------------------------------------------
void FilterTaylorSeries::Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime )
{
    const float fJitterRadius = 0.05f;
    const float fAlphaCoef  = 1.0f - m_fSmoothing;
    const float fBetaCoeff  = (fAlphaCoef * fAlphaCoef ) / ( 2 - fAlphaCoef );

    Vec4 vRawPos;
    // Velocity, acceleration and Jolt are 1st, 2nd and 3rd degree derivatives of position respectively. 
    Vec4 vCurFilteredPos, vEstVelocity, vEstAccelaration, vEstJolt;
    Vec4 vPrevFilteredPos, vPrevEstVelocity, vPrevEstAccelaration, vPrevEstJolt;
    Vec4 vDiff;
    float fDiff;

    Vec4 vPredicted, vError;
    Vec4 vConstants(0.0f, 1.0f, 0.5f, 0.1667f);

    for (int i = 0; i < KIN_SKELETON_POSITION_COUNT; i++)
    {
        vRawPos             = pSkeletonData.vSkeletonPositions[i];
        vPrevFilteredPos    = m_History[i].vPos;
        vPrevEstVelocity    = m_History[i].vEstVelocity;
        vPrevEstAccelaration = m_History[i].vEstAccelaration;
        vPrevEstJolt        = m_History[i].vEstJolt;

        if (!JointPositionIsValid(vPrevFilteredPos))
        {
            vCurFilteredPos = vRawPos;
            vEstVelocity      = Vec4(0,0,0,0);
            vEstAccelaration     = Vec4(0,0,0,0);
            vEstJolt     = Vec4(0,0,0,0);
        }
        else if (!JointPositionIsValid(vRawPos))
        {
            vCurFilteredPos = vPrevFilteredPos;
            vEstVelocity = vPrevEstVelocity;
            vEstAccelaration = vPrevEstAccelaration;
            vEstJolt = vPrevEstJolt;
        }
        else
        {
            // If the current and previous frames have valid data, perform interpolation

            vDiff = vPrevFilteredPos - vRawPos;
            fDiff = fabs(vDiff.GetLength());

            if (fDiff <= fJitterRadius)
            {
                vCurFilteredPos = vRawPos * fDiff/fJitterRadius + vPrevFilteredPos * (1.0f - fDiff/fJitterRadius);
            }
            else
            {
                vCurFilteredPos = vRawPos;
            }

            vPredicted  = vPrevFilteredPos + vPrevEstVelocity;
            vPredicted  = vPredicted + vPrevEstAccelaration * (vConstants.y * vConstants.y * vConstants.z);
            vPredicted  = vPredicted + vPrevEstJolt * (vConstants.y * vConstants.y * vConstants.y * vConstants.w);
            vError      = vCurFilteredPos - vPredicted;

            vCurFilteredPos = vPredicted + vError * fAlphaCoef;
            vEstVelocity = vPrevEstVelocity + vError * fBetaCoeff;
            vEstAccelaration = vEstVelocity - vPrevEstVelocity;
            vEstJolt = vEstAccelaration - vPrevEstAccelaration;
        }

        // Update the state
        m_History[i].vPos = vCurFilteredPos;
        m_History[i].vEstVelocity = vEstVelocity;
        m_History[i].vEstAccelaration = vEstAccelaration;
        m_History[i].vEstJolt = vEstJolt;
      
        // Output the data
        m_FilteredJoints[i]     = vCurFilteredPos;
        m_FilteredJoints[i].w   = 1.0f;
    }
}

//--------------------------------------------------------------------------------------
// Name: FilterAdaptiveDoubleExponential::UpdateSmoothingParameters()
// Desc: Updates the smoothing parameters based on the smoothing filter's trend
//--------------------------------------------------------------------------------------
void FilterAdaptiveDoubleExponential::Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime  )
{
    for (uint32 i = 0; i < KIN_SKELETON_POSITION_COUNT; i++)
    {
        Vec4 vPreviousPosition  = m_DoubleExponentialFilter.m_History[ i ].m_vRawPosition;
        Vec4 vCurrentPosition   = pSkeletonData.vSkeletonPositions[ i ];
        Vec4 vVelocity          = ( vCurrentPosition - vPreviousPosition ) / fDeltaTime;
        float fVelocity             = fabsf( vVelocity.GetLength() );

        UpdateSmoothingParameters( i, fVelocity, pSkeletonData.eSkeletonPositionTrackingState[i] );

        m_DoubleExponentialFilter.Update( pSkeletonData, i, m_SmoothingParams[ i ] );
    }

    // Copy filtered data to output data
    memcpy( m_FilteredJoints, m_DoubleExponentialFilter.GetFilteredJoints(), sizeof( m_FilteredJoints ) );

}


//--------------------------------------------------------------------------------------
// Name: FilterAdaptiveDoubleExponential::UpdateSmoothingParameters()
// Desc: Updates the smoothing parameters of a joint based on he current joint velocity
//--------------------------------------------------------------------------------------
void FilterAdaptiveDoubleExponential::UpdateSmoothingParameters( uint32 i, float fVelocity, KIN_SKELETON_POSITION_TRACKING_STATE eTrackingState )
{
    assert( i < KIN_SKELETON_POSITION_COUNT );

    KIN_TRANSFORM_SMOOTH_PARAMETERS LerpedParams;

    static const float fBigJitterThreshold      = 5.0f;     // meters per second
    static const float fLowLatencyThreshold     = 1.5f;     // meters per second
    static const float fHighLatencyThreshold    = 0.25f;    // meters per second

    BOOL bDetectBigJitter = false;

    // Parameters for hands should should be the same as wrists, otherwise they look unnatural
    switch( i )
    {
    case KIN_SKELETON_POSITION_HAND_LEFT:
        memcpy( &m_SmoothingParams[ KIN_SKELETON_POSITION_HAND_LEFT ], &m_SmoothingParams[ KIN_SKELETON_POSITION_WRIST_LEFT ], sizeof( KIN_TRANSFORM_SMOOTH_PARAMETERS ) );
        break;

		case KIN_SKELETON_POSITION_HAND_REFINED_LEFT:
			memcpy( &m_SmoothingParams[ KIN_SKELETON_POSITION_HAND_REFINED_LEFT ], &m_SmoothingParams[ KIN_SKELETON_POSITION_WRIST_LEFT ], sizeof( KIN_TRANSFORM_SMOOTH_PARAMETERS ) );
			break;

    case KIN_SKELETON_POSITION_HAND_RIGHT:
        memcpy( &m_SmoothingParams[ KIN_SKELETON_POSITION_HAND_RIGHT ], &m_SmoothingParams[ KIN_SKELETON_POSITION_WRIST_RIGHT ], sizeof( KIN_TRANSFORM_SMOOTH_PARAMETERS ) );
        break;

		case KIN_SKELETON_POSITION_HAND_REFINED_RIGHT:
			memcpy( &m_SmoothingParams[ KIN_SKELETON_POSITION_HAND_REFINED_RIGHT ], &m_SmoothingParams[ KIN_SKELETON_POSITION_WRIST_LEFT ], sizeof( KIN_TRANSFORM_SMOOTH_PARAMETERS ) );
			break;

    case KIN_SKELETON_POSITION_FOOT_LEFT:
        memcpy( &m_SmoothingParams[ KIN_SKELETON_POSITION_FOOT_LEFT ], &m_SmoothingParams[ KIN_SKELETON_POSITION_ANKLE_LEFT ], sizeof( KIN_TRANSFORM_SMOOTH_PARAMETERS ) );
        break;

    case KIN_SKELETON_POSITION_FOOT_RIGHT:
        memcpy( &m_SmoothingParams[ KIN_SKELETON_POSITION_FOOT_RIGHT ], &m_SmoothingParams[ KIN_SKELETON_POSITION_ANKLE_RIGHT ], sizeof( KIN_TRANSFORM_SMOOTH_PARAMETERS ) );
        break;

    default:
        {
            // Lerp between high and low latency params
            float fLerp = max( 0.0f, min( 1.0f, ( fVelocity - fHighLatencyThreshold ) / ( fLowLatencyThreshold - fHighLatencyThreshold ) ) );

            // Detected a big jitter
            if ( fVelocity > fBigJitterThreshold ||
                 fLerp - m_fPreviousLerp[ i ] > 0.999f )
            {
                bDetectBigJitter = true;
                fLerp = ( fLerp * 0.25f ) + ( m_fPreviousLerp[ i ] * 0.75f );
            }

            m_fPreviousLerp[ i ] = fLerp;

            LerpedParams.m_fSmoothing = m_HighLatencySmoothingParams.m_fSmoothing + fLerp * ( m_LowLatencySmoothingParams.m_fSmoothing - m_HighLatencySmoothingParams.m_fSmoothing );
            LerpedParams.m_fCorrection = m_HighLatencySmoothingParams.m_fCorrection + fLerp * ( m_LowLatencySmoothingParams.m_fCorrection - m_HighLatencySmoothingParams.m_fCorrection );
            LerpedParams.m_fPrediction = m_HighLatencySmoothingParams.m_fPrediction + fLerp * ( m_LowLatencySmoothingParams.m_fPrediction - m_HighLatencySmoothingParams.m_fPrediction );
            LerpedParams.m_fJitterRadius = m_HighLatencySmoothingParams.m_fJitterRadius + fLerp * ( m_LowLatencySmoothingParams.m_fJitterRadius - m_HighLatencySmoothingParams.m_fJitterRadius );
            LerpedParams.m_fMaxDeviationRadius = m_HighLatencySmoothingParams.m_fMaxDeviationRadius + fLerp * ( m_LowLatencySmoothingParams.m_fMaxDeviationRadius - m_HighLatencySmoothingParams.m_fMaxDeviationRadius );

            memcpy( &m_SmoothingParams[ i ], &LerpedParams, sizeof( KIN_TRANSFORM_SMOOTH_PARAMETERS ) );               
        }
    }
    
    if ( bDetectBigJitter )
    {
        m_SmoothingParams[ i ].m_fJitterRadius *= 2.0f;
        m_SmoothingParams[ i ].m_fMaxDeviationRadius *= 2.0f;
        m_SmoothingParams[ i ].m_fPrediction /= 2.0f;
    }
    else if ( eTrackingState == KIN_SKELETON_POSITION_INFERRED )
    {
        // if the joint is not tracked, but inferred, we make sure it gets smoothed more by doubling the jitter radius
        m_SmoothingParams[ i ].m_fJitterRadius *= 2.0f;
        m_SmoothingParams[ i ].m_fMaxDeviationRadius *= 2.0f;
    }      
}

#endif
