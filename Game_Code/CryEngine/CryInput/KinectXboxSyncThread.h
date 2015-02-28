// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#pragma once

#if !defined(RELEASE) && (defined(WIN32) || defined(WIN64))

//////////////////////////////////////////////////////////////////////////
// Thread that performs syncing with the Xbox to get Kinect Info
//////////////////////////////////////////////////////////////////////////
class CKinectXboxSyncThread : public CrySimpleThread<CKinectXboxSyncThread>
{
public:	
	CKinectXboxSyncThread();
	~CKinectXboxSyncThread();

	bool Initialize();
	void Shutdown();

	void GetSkeletonFrame(SKinSkeletonFrame& skeletonFrame);

	void SetSeatedST(bool bValue);
	bool IsUsingSeatedST() const { return m_bUseSeatedST; }

	//////////////////////////////////////////////////////////////////////////
	// CrySimpleThread
	//////////////////////////////////////////////////////////////////////////
	virtual void Run();
	virtual void Cancel();
	//////////////////////////////////////////////////////////////////////////

private:

	bool ReceiveDataFromSocket(std::vector<uint8>& rData);
	string GetXboxIP();

	SOCKET m_XboxListenSocket;
	SKinSkeletonFrame m_XboxListenSkeletonFrame;
	CryCriticalSection m_XboxListenCS;

	bool m_bUseCompression;

	volatile bool m_bRequestShutdown; 

	volatile bool m_bUseSeatedST;
	volatile bool m_bSeatedSTFlagDirty;
};

#endif