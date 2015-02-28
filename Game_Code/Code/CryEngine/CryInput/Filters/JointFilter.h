//--------------------------------------------------------------------------------------
// ADAPTED from Microsoft's provided sample AtgNuiJointFilter.h
//
// This file contains various filters for filtering Joints
//
// Microsoft Game Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#pragma once
#ifndef JOINT_FILTER_H
#define JOINT_FILTER_H


typedef struct _KIN_TRANSFORM_SMOOTH_PARAMETERS {

	_KIN_TRANSFORM_SMOOTH_PARAMETERS(float fSmoothing = 0.0f, 
		float fCorrection = 0.0f, float fPrediction = 0.0f, float fJitterRadius = 0.0f, float fMaxDeviationRadius = 0.0f):
		m_fSmoothing(fSmoothing), 
		m_fCorrection(fCorrection), 
		m_fPrediction(fPrediction), 
		m_fJitterRadius(fJitterRadius), 
		m_fMaxDeviationRadius(fMaxDeviationRadius) 
		{     
			// Check for divide by zero. Use an epsilon of a 10th of a millimeter
			m_fJitterRadius = max(0.0001f, m_fJitterRadius);
		}

	float m_fSmoothing; // How much smoothing will occur.  Will lag when too high
	float m_fCorrection; // How much to correct back from prediction.  Can make things springy
	float m_fPrediction; // Amount of prediction into the future to use. Can over shoot when too high
	float m_fJitterRadius; // Size of the radius where jitter is removed. Can do too much smoothing when too high
	float m_fMaxDeviationRadius; // Size of the max prediction radius Can snap back to noisy data when too high
} KIN_TRANSFORM_SMOOTH_PARAMETERS;

class FilterTaylorSeries : public ISkeletonFilter
{

public:
	FilterTaylorSeries( ): m_fSmoothing(0.5f) {Reset();}

		virtual ~FilterTaylorSeries(){};

    // Set smoothing parameters
    void SetParameters( float fSmoothing )
    {
        m_fSmoothing = fSmoothing;
    };

    void Reset()
    {
        memset( m_FilteredJoints, 0, sizeof( m_FilteredJoints )  );
        memset( m_History, 0, sizeof( m_History ) );
    }

    void Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime );

    Vec4* GetFilteredJoints() {return &m_FilteredJoints[0];}

private:
    struct FilterTaylorSeriesData
    {
        Vec4 vPos;                  // Position
        Vec4 vEstVelocity;          // First  degree derivative of position
        Vec4 vEstAccelaration;      // Second degree derivative of position
        Vec4 vEstJolt;              // Third  degree derivative of position
    };

    Vec4 m_FilteredJoints[KIN_SKELETON_POSITION_COUNT];
    FilterTaylorSeriesData m_History[KIN_SKELETON_POSITION_COUNT];
    float m_fSmoothing;

};

class FilterBlendJoint : public ISkeletonFilter 
{
public:

    FilterBlendJoint() : m_pSkeletonsData( NULL ), m_nFramesToAverage( 10 ), m_iCurrentFrame( 0 )
    {
        Reset();
    };

		virtual ~FilterBlendJoint(){};

    void SetParameters( int nFramesToAverage )
    {
        m_nFramesToAverage = nFramesToAverage;
				m_pSkeletonsData.reserve(m_nFramesToAverage);
    };

    void Reset()
    {
        memset( &m_FilteredSkeleton, 0, sizeof( FilterSkeletonData ) );
        m_iCurrentFrame = 0;    
    }

    void Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime );

    Vec4* GetFilteredJoints( );    

private:
    struct FilterSkeletonData
    {
        Vec4 m_Positions[KIN_SKELETON_POSITION_COUNT];
    };

    FilterSkeletonData m_FilteredSkeleton;
		std::vector<FilterSkeletonData> m_pSkeletonsData;
    INT m_nFramesToAverage;
    INT m_iCurrentFrame;
};

class FilterVelDamp : public ISkeletonFilter
{
public:
    FilterVelDamp()
    {
        Reset();
		};

		virtual ~FilterVelDamp(){};

    void Reset()
    {
        memset( m_SkeletonData, 0, sizeof(FilterSkeletonData) * 2 );
        memset( m_VelocityEstimate, 0, sizeof(FilterVelocitySingleEstimate) * 2 );
        m_iCurrentFrame = -1;
        m_iPreviousFrame = -1;    
    }

    void Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime );

    Vec4* GetFilteredJoints( );    

private:
    struct FilterSkeletonData
    {
        Vec4 m_Positions[KIN_SKELETON_POSITION_COUNT];
    };
    
    struct FilterVelocitySingleEstimate
    {
        Vec4 m_vEstVel[KIN_SKELETON_POSITION_COUNT];
    };
    
    FilterSkeletonData m_SkeletonData[2];
    FilterVelocitySingleEstimate m_VelocityEstimate[2];
    INT m_iCurrentFrame;
    INT m_iPreviousFrame;
};


// Holt Double Exponential Smoothing filter
class FilterDoubleExponentialData
{
public:
    Vec4 m_vRawPosition;
    Vec4 m_vFilteredPosition;
    Vec4 m_vTrend;
    DWORD    m_dwFrameCount;
};

class FilterDoubleExponential : public ISkeletonFilter
{
public:
		
		FilterDoubleExponential():m_paramsSet(false){Reset();}

		virtual ~FilterDoubleExponential(){};

		void SetParameters(const KIN_TRANSFORM_SMOOTH_PARAMETERS& smoothingParams)
		{
				m_params = smoothingParams;	
				m_paramsSet = true;
		}

    void Reset()
    {
        memset(m_FilteredJoints, 0, sizeof(m_FilteredJoints));
        memset(m_History, 0, sizeof(m_History));
    }

    void Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime );

    Vec4* GetFilteredJoints() {return &m_FilteredJoints[0];}

    // we need to be able to dynamically change the smoothing parameters for the filter
    friend class FilterAdaptiveDoubleExponential;

private:

    Vec4 m_FilteredJoints[KIN_SKELETON_POSITION_COUNT];
    FilterDoubleExponentialData m_History[KIN_SKELETON_POSITION_COUNT];

    void Update( const SKinSkeletonRawData& pSkeletonData, uint32 i, const KIN_TRANSFORM_SMOOTH_PARAMETERS&);

		KIN_TRANSFORM_SMOOTH_PARAMETERS m_params;

		bool m_paramsSet;

};

class FilterCombinationData
{
public:
    static const DWORD MAX_COMBINATION_FILTER_TAPS = 10;
    Vec4	m_vPrevDeltas[ MAX_COMBINATION_FILTER_TAPS ];	
    Vec4	m_vWantedPos;					
    Vec4	m_vLastWantedPos;				
    Vec4	m_vPos;						
    float	    m_fWantedLocalBlendRate;
    float	    m_fActualLocalBlendRate;
    bool	    m_bActive[3];					// Primary/secondary/tertiary

};

class FilterCombination : public ISkeletonFilter
{
public:
		FilterCombination():m_fDefaultApplyRate(0.055f), m_fDotProdThreshold(0.20f), m_fDeltaLengthThreshold(0.002f),
			m_fBlendedLengthThreshold(0.005f), m_bDotProdNormalize(true), m_nUseTaps(4), m_fDownBlendRate(0.5f), m_fBlendBlendRate(0.1f)
		{
			Reset();
		}

		virtual ~FilterCombination(){};

    void SetParameters( float fDefaultApplyRate, float fDotProdThreshold, float fDeltaLengthThreshold, float fBlendedLengthThreshold,
               bool bDotProdNormalize, int nUseTaps, float fDownBlendRate, float fBlendBlendRate)
    {
        m_fDefaultApplyRate = fDefaultApplyRate;
        m_fDotProdThreshold = fDotProdThreshold;
        m_fDeltaLengthThreshold = fDeltaLengthThreshold;
        m_fBlendedLengthThreshold = fBlendedLengthThreshold;
        m_bDotProdNormalize = bDotProdNormalize;
        m_nUseTaps = nUseTaps;
        m_fDownBlendRate = fDownBlendRate;
        m_fBlendBlendRate = fBlendBlendRate;
        Reset();
    }

    void Reset()
    {
        memset(m_FilteredJoints, 0, sizeof(m_FilteredJoints));
        memset(m_History, 0, sizeof(m_History));
    }

    void Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime );

    Vec4* GetFilteredJoints() {return &m_FilteredJoints[0];}

private:
    Vec4 m_FilteredJoints[KIN_SKELETON_POSITION_COUNT];
    FilterCombinationData m_History[KIN_SKELETON_POSITION_COUNT];
      
    float m_fDefaultApplyRate;
    float m_fDotProdThreshold;
    float m_fDeltaLengthThreshold;
    float m_fBlendedLengthThreshold;
    bool  m_bDotProdNormalize;
    uint32  m_nUseTaps;
    float m_fDownBlendRate;
    float m_fBlendBlendRate;

};

// Dynamically adjust the smoothing parameters for each joint using a double exponential smoothing filter
// The velocity of each joint is evaluated which means that for a joint moving slowly, it will
// be smoother with higher latency, but for a joint moving faster it will be less smooth by with
// lower latency
class FilterAdaptiveDoubleExponential : public ISkeletonFilter
{
public:

		FilterAdaptiveDoubleExponential()
		{ 
			Reset(); 
		}

		virtual ~FilterAdaptiveDoubleExponential(){};

		void SetParameters(const KIN_TRANSFORM_SMOOTH_PARAMETERS& highLatencyParams, const KIN_TRANSFORM_SMOOTH_PARAMETERS& lowLatencyParams)
		{
			m_HighLatencySmoothingParams = highLatencyParams;
			m_LowLatencySmoothingParams = lowLatencyParams;
		}

    void Reset()
    {
        m_DoubleExponentialFilter.Reset();
        memset( m_FilteredJoints, 0, sizeof( m_FilteredJoints ) );            
        for ( uint32 i = 0; i < KIN_SKELETON_POSITION_COUNT; i++ )
        {
            memcpy( &m_SmoothingParams[ i ], &m_HighLatencySmoothingParams, sizeof( m_HighLatencySmoothingParams ) );
            m_fPreviousLerp[ i ] = 0.0f;
        }
    }

    void Update( const SKinSkeletonRawData& pSkeletonData, const float fDeltaTime );

    Vec4* GetFilteredJoints() {return &m_FilteredJoints[0];}

private:
    Vec4 m_FilteredJoints[KIN_SKELETON_POSITION_COUNT];

    KIN_TRANSFORM_SMOOTH_PARAMETERS m_HighLatencySmoothingParams;
    KIN_TRANSFORM_SMOOTH_PARAMETERS m_LowLatencySmoothingParams;
    KIN_TRANSFORM_SMOOTH_PARAMETERS m_SmoothingParams[KIN_SKELETON_POSITION_COUNT];
    float m_fPreviousLerp[KIN_SKELETON_POSITION_COUNT];

    FilterDoubleExponential m_DoubleExponentialFilter;

    void UpdateSmoothingParameters( uint32 i, float fVelocity, KIN_SKELETON_POSITION_TRACKING_STATE eTrackingState );
    
};

#endif // #define JOINT_FILTER_H
