// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef SOUNDALLOCATOR_H_INCLUDED
#define SOUNDALLOCATOR_H_INCLUDED

#include <CryMemoryManager.h>
#include <BucketAllocatorImpl.h>
#include <FrameProfiler.h>
#include <IAudioInterfacesCommonData.h>

class IGeneralMemoryHeap;

// These two Macros can be used to allocate memory in the module's memory pool.
// The first one creates a PTR that stores a new instance of a CLASS, the second one uses a pre-created PTR to save the new instance
// Both will only execute the constructor if the allocation succeeded
#define POOL_NEW_CREATE(CLASS, PTR) CLASS* PTR = static_cast<CLASS*>(AUDIO_ALLOCATOR_MEMORY_POOL.Allocate<void*>(sizeof(CLASS), AUDIO_MEMORY_ALIGNMENT, #CLASS)); !PTR ? 0 : new(PTR) CLASS
#define POOL_NEW(CLASS, PTR) PTR = static_cast<CLASS*>(AUDIO_ALLOCATOR_MEMORY_POOL.Allocate<void*>(sizeof(CLASS), AUDIO_MEMORY_ALIGNMENT, #CLASS)); !PTR ? 0 : new(PTR) CLASS

// These two macros are used to destroy the objects allocated in the module's memory pool and free their memory
// The second one is designed to be used for const pointers
#define POOL_FREE(PTR) { stl::destruct(PTR); bool const bFreed = AUDIO_ALLOCATOR_MEMORY_POOL.Free(PTR); assert(bFreed); }
#define POOL_FREE_CONST(PTR) { stl::destruct(PTR); bool const bFreed = AUDIO_ALLOCATOR_MEMORY_POOL.Free((void*) PTR); assert(bFreed); }

//////////////////////////////////////////////////////////////////////////
// <title CSoundAllocator>
// Summary:
//			A wrapper class for all pool allocations. Small allocations are redirected to the global bucket allocator,
//			the larger ones are kept in the module's memory pool.
//////////////////////////////////////////////////////////////////////////
class CSoundAllocator
{
public:

	//DOC-IGNORE-BEGIN	
	CSoundAllocator()
		: m_mspaceBase(NPTR)
		, m_mspaceSize(0)
		, m_mspace(NPTR)
		, m_mspaceUsed(0)
		, nSmallAllocations(0)
		, nAllocations(0)
		, nSmallAllocationsSize(0)
		, nAllocationsSize(0)
	{
#if CAPTURE_REPLAY_LOG && defined(USE_GLOBAL_BUCKET_ALLOCATOR)
		m_smallAllocator.ReplayRegisterAddressRange("Sound Buckets");
#endif
	}

	~CSoundAllocator()
	{
		if (m_mspace)
			m_mspace->Release();
	}
	//DOC-IGNORE-END

	void* AllocateRaw(size_t const nSize, size_t const nAlign, char const* const sUsage)
	{
		// Redirect small allocations through a bucket allocator
		if (nSize <= nMaxSmallSize)
		{			
			void* const ret = m_smallAllocator.allocate(nSize);
			TrackSmallAlloc(ret, m_smallAllocator.getSizeEx(ret));			
			return ret;
		}

		FRAME_PROFILER("CSoundAllocator::Allocate", GetISystem(), PROFILE_AUDIO);

		void* const ret = m_mspace->Memalign(nAlign, nSize, sUsage);

		if (ret != NPTR)
		{
			CryInterlockedAdd(&m_mspaceUsed, m_mspace->UsableSize(ret));
		}

		TrackGeneralAlloc(ret, nSize);		

		return ret;
	}

	template<typename T>
	T Allocate(size_t const nSize, size_t const nAlign, char const* const sUsage)
	{
		return reinterpret_cast<T>(AllocateRaw(nSize, nAlign, sUsage));
	}

	bool Free(void* ptr)
	{
		if (ptr && m_smallAllocator.IsInAddressRange(ptr))
		{
			m_smallAllocator.deallocate(ptr);
			TrackSmallDealloc(ptr);

			return true;
		}
		else
		{
			size_t sz = m_mspace->Free(ptr);
			if (sz)
			{
				TrackGeneralDealloc(sz);
				CryInterlockedAdd(&m_mspaceUsed, -static_cast<int>(sz));

				return true;
			}
		}

		return false;
	}

	size_t Size(void* ptr)
	{			
		if (m_smallAllocator.IsInAddressRange(ptr))
		{
			return m_smallAllocator.getSizeEx(ptr);
		}
		else
		{
			return m_mspace->UsableSize(ptr);
		}
	}

	ILINE uint8* Data() const { return reinterpret_cast<uint8*>(m_mspaceBase); }
	ILINE size_t MemSize() const { return m_mspaceSize; }
	ILINE size_t MemFree() const { return m_mspaceSize - static_cast<size_t>(m_mspaceUsed); }
	ILINE size_t const FragmentCount()				const { return static_cast<size_t>(nAllocations); }	
	ILINE size_t const GetSmallAllocsSize()		const { return static_cast<size_t>(nSmallAllocationsSize); }
	ILINE size_t const GetSmallAllocsCount()	const { return static_cast<size_t>(nSmallAllocations); }

	void InitMem(size_t const nSize, void* ptr, char const* const sDescription)
	{
		m_mspace = CryGetIMemoryManager()->CreateGeneralMemoryHeap(ptr, nSize, sDescription);
		m_mspaceBase = ptr;
		m_mspaceSize = nSize;
	}

	void UnInitMem()
	{
		if (m_mspace)
			m_mspace->Release();

		m_mspaceBase					= NPTR;
		m_mspaceSize					= 0;
		m_mspace							= NPTR;
		m_mspaceUsed					= 0;
		nSmallAllocations			= 0;
		nAllocations					= 0;
		nSmallAllocationsSize	= 0;
		nAllocationsSize			= 0;
	}

	void Cleanup()
	{
#if defined(USE_GLOBAL_BUCKET_ALLOCATOR)
		m_smallAllocator.cleanup();
#endif
	}

private:

#ifdef USE_GLOBAL_BUCKET_ALLOCATOR
	typedef BucketAllocator<BucketAllocatorDetail::DefaultTraits<10 * 1024 * 1024, BucketAllocatorDetail::SyncPolicyLocked, false> > SoundBuckets;
#else
	typedef node_alloc<eCryDefaultMalloc, true, 512 * 1024> SoundBuckets;
#endif

	CSoundAllocator(const CSoundAllocator&);
	CSoundAllocator& operator = (const CSoundAllocator&);

	void TrackSmallAlloc( void *ptr, size_t nSize )
	{		
#ifdef INCLUDE_AUDIO_PRODUCTION_CODE
		if(ptr)
		{
			CryInterlockedIncrement(&nSmallAllocations);
			CryInterlockedAdd(&nSmallAllocationsSize, nSize);
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}

	void TrackGeneralAlloc( void *ptr, size_t nSize )
	{
#ifdef INCLUDE_AUDIO_PRODUCTION_CODE
		if(ptr)
		{
			CryInterlockedIncrement(&nAllocations);
			CryInterlockedAdd(&nAllocationsSize, nSize);
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}

	void TrackSmallDealloc( void *ptr )
	{
#ifdef INCLUDE_AUDIO_PRODUCTION_CODE
		CryInterlockedDecrement(&nSmallAllocations);
		CryInterlockedAdd(&nSmallAllocationsSize, -static_cast<int>(Size(ptr)));
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}

	void TrackGeneralDealloc( size_t sz )
	{
#ifdef INCLUDE_AUDIO_PRODUCTION_CODE
		CryInterlockedDecrement(&nAllocations);
		CryInterlockedAdd(&nAllocationsSize, -static_cast<int>(sz));
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}

	// all allocations below nMaxSmallSize go into the system-wide allocator(and thus the system wide small allocation allocator)
	enum {nMaxSmallSize = 512};

	void* m_mspaceBase;
	size_t m_mspaceSize;
	IGeneralMemoryHeap* m_mspace;
	volatile int m_mspaceUsed;

	SoundBuckets m_smallAllocator;

	volatile int nSmallAllocations;
	volatile int nAllocations;

	volatile int nSmallAllocationsSize;
	volatile int nAllocationsSize;
};

#endif // SOUNDALLOCATOR_H_INCLUDED
