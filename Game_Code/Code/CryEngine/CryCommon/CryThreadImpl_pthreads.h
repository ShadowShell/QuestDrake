/////////////////////////////////////////////////////////////////////////////
//
// Crytek Source File
// Copyright (C), Crytek Studios, 2001-2006.
//
// History:
// Jun 20, 2006: Created by Sascha Demetrio
//
/////////////////////////////////////////////////////////////////////////////

#include "CryThread_pthreads.h"

#ifndef __SPU__
#if PLATFORM_SUPPORTS_THREADLOCAL
THREADLOCAL CrySimpleThreadSelf
	*CrySimpleThreadSelf::m_Self = NULL;
#else
TLS_DEFINE(CrySimpleThreadSelf*, g_CrySimpleThreadSelf)
#endif
#endif
// vim:ts=2


//////////////////////////////////////////////////////////////////////////
// CryEvent(Timed) implementation
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CryEventTimed::Reset()
{
	m_lockNotify.Lock();
	m_flag = false;
	m_lockNotify.Unlock();
}

//////////////////////////////////////////////////////////////////////////
void CryEventTimed::Set()
{
	m_lockNotify.Lock();
	m_flag = true;
	m_cond.Notify();
	m_lockNotify.Unlock();
}

//////////////////////////////////////////////////////////////////////////
void CryEventTimed::Wait()
{
	m_lockNotify.Lock();
	if (!m_flag)
		m_cond.Wait(m_lockNotify);
	m_flag	=	false;
	m_lockNotify.Unlock();
}

//////////////////////////////////////////////////////////////////////////
bool CryEventTimed::Wait( const uint32 timeoutMillis )
{
	bool bResult = true;
	m_lockNotify.Lock();
	if (!m_flag)
		bResult = m_cond.TimedWait(m_lockNotify,timeoutMillis);
	m_flag	=	false;
	m_lockNotify.Unlock();
	return bResult;
}

///////////////////////////////////////////////////////////////////////////////
// CryCriticalSection implementation
///////////////////////////////////////////////////////////////////////////////
typedef CryLockT<CRYLOCK_RECURSIVE> TCritSecType;

void  CryDeleteCriticalSection( void *cs )
{
	delete ((TCritSecType *)cs);
}

void  CryEnterCriticalSection( void *cs )
{
	((TCritSecType*)cs)->Lock();
}

bool  CryTryCriticalSection( void *cs )
{
	return false;
}

void  CryLeaveCriticalSection( void *cs )
{
	((TCritSecType*)cs)->Unlock();
}

void  CryCreateCriticalSectionInplace(void* pCS)
{
	new (pCS) TCritSecType;
}

void CryDeleteCriticalSectionInplace( void *)
{
}

void* CryCreateCriticalSection()
{
	return (void*) new TCritSecType;
}

#if defined(PS3) && !defined (ORBIS)
#if !defined(__SPU__)
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CryInterlockedPushEntrySList( SLockFreeSingleLinkedListHeader& list,  SLockFreeSingleLinkedListEntry &element )
{
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListEntry) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_ENTRY_HAS_WRONG_SIZE);		

	SLockFreeSingleLinkedListEntry *pCurrentTop = NULL;
	volatile void *pHeader = alias_cast<volatile void*>(&list);
	__lwsync();
	do
	{
		pCurrentTop = (SLockFreeSingleLinkedListEntry*)( __lwarx(pHeader) );
		element.pNext = pCurrentTop;
	}
	while (!__stwcx(pHeader, (long)&element));
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedPopEntrySList(  SLockFreeSingleLinkedListHeader& list )
{
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);

	struct SFreeList{ SFreeList *pNext; };
	volatile SLockFreeSingleLinkedListEntry *pCurrentTop = NULL;
	volatile SLockFreeSingleLinkedListEntry *pNext = NULL;
	volatile void *pHeader = alias_cast<volatile void*>(&list);
	do
	{
		pCurrentTop = (SLockFreeSingleLinkedListEntry*)( __lwarx(pHeader) );
		if(pCurrentTop == NULL)
			return NULL;

		pNext = pCurrentTop->pNext;
	}
	while (!__stwcx(pHeader, (long)pNext));
	__lwsync();
	return (void*)pCurrentTop;
}

//////////////////////////////////////////////////////////////////////////
void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list)
{
#if !defined(__SPU__)
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
	list.pNext = NULL;
#endif 
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list)
{
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);

	struct SFreeList{ SFreeList *pNext; };
	SLockFreeSingleLinkedListEntry *pCurrentTop = NULL;
	SLockFreeSingleLinkedListEntry *pNext = NULL;
	volatile void *pHeader = alias_cast<volatile void*>(&list);
	do
	{
		pCurrentTop = (SLockFreeSingleLinkedListEntry*)( __lwarx(pHeader) );
		if(pCurrentTop == NULL)
			return NULL;		
	}
	while (!__stwcx(pHeader, (long)0));
	__lwsync();
	return pCurrentTop;
}

#else // for SPU forward to driver implemented functions
inline void CryInterlockedPushEntrySList( SLockFreeSingleLinkedListHeader& list, SLockFreeSingleLinkedListEntry &element ) 
{ 
	CryInterlockedPushEntrySListSPU( SPU_MAIN_PTR((void*)&list), SPU_MAIN_PTR((void*)&element)); 
}

inline SPU_DOMAIN_MAIN void* CryInterlockedPopEntrySList(  SLockFreeSingleLinkedListHeader& list )
{ 
	return SPU_MAIN_PTR( CryInterlockedPopEntrySListSPU( SPU_MAIN_PTR((void*)&list) ) ); 
}

inline void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list) 
{ 
	CryInitializeSListHeadSPU( SPU_MAIN_PTR((void*)&list) ); 
}

inline SPU_DOMAIN_MAIN void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list) 
{
	return SPU_MAIN_PTR( CryInterlockedFlushSListSPU( SPU_MAIN_PTR((void*)&list) ) );
}

#endif

#elif defined(_CPU_ARM)

/**
 * (ARMv7) Performs load and tags for conditional store.
 *
 * None of GCC 4.6/4.8 or clang 3.3/3.4 have a builtin intrinsic for ARM's ldrex/strex or dmb
 * instructions.  This is a placeholder until supplied by the toolchain.
 *
 * @param addr Memory location to load and tag for a later condional store with strex instruction.
 * @returns Value loaded from memory at addr
 */
static inline uint64_t __ldrex( uint64_t* addr )
{
	uint64_t result;
#if defined(PLATFORM_64BIT)
	__asm__ __volatile__("ldxr %0, [%1]\n\t"
			: "=&r"(result)
			: "r"(addr));
#else
	__asm__ __volatile__("ldrexd %0, %H0, [%1]\n\t"
			: "=&r"(result)
			: "r"(addr));
#endif
	return result;
}

/**
 * (ARMv7) Performs conditional store.
 *
 * None of GCC 4.6/4.8 or clang 3.3/3.4 have a builtin intrinsic for ARM's ldrex/strex or dmb
 * instructions.  This is a placeholder until supplied by the toolchain.
 *
 * @param addr Memory destination for attempted store.  Must have been previously tagged with ldrex instruction.
 * @param data Value to write/store.
 * @returns True if the write is successful, otherwise false.
 */
static inline bool __strex( uint64_t* addr, uint64_t data )
{
#if defined(PLATFORM_64BIT)
	uint64_t res;
	__asm__ __volatile__("stxr %w0, %2, [%1]\n\t"
		: "=&r"(res)
		: "r"(addr), "r"(data));
#else
#if defined(__clang__)
	// Clang 3.3 complains that "data" is being truncated although the %2/%H2 operand constraints
	// address this.  Verified working with GCC 4.8 and Clang 3.3/3.4.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wasm-operand-widths"
#endif
	uint32_t res;
	__asm__ __volatile__("strexd %0, %2, %H2, [%1]\n\t"
		: "=&r"(res)
		: "r"(addr), "r"(data));
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif // PLATFORM_64BIT
	// If store succeeds, 0 is returned
	return (res == 0);
}

/**
 * (ARMv7) Full memory barriar.
 *
 * None of GCC 4.6/4.8 or clang 3.3/3.4 have a builtin intrinsic for ARM's ldrex/strex or dmb
 * instructions.  This is a placeholder until supplied by the toolchain.
 */
#if defined(IOS)
static inline void  __dmb()
{
	// The linux kernel uses "dmb ish" to only sync with local monitor (arch/arm/include/asm/barrier.h):
	//#define dmb(option) __asm__ __volatile__ ("dmb " #option : : : "memory")
	//#define smp_mb()        dmb(ish)
	// For now let's just do a full dmb with "dmb sy" where the "sy" is the default and optional
#if !defined(PLATFORM_64BIT)
	__asm__ __volatile__ ("dmb" : : : "memory");
#else
	// Need explicit option on ARM 64
	__asm__ __volatile__ ("dmb ish" : : : "memory");
#endif
}
#endif


void CryInterlockedPushEntrySList( SLockFreeSingleLinkedListHeader& list, SLockFreeSingleLinkedListEntry &element )
{
	__dmb(); // Release barrier (as per "ARM Synchronization Primitives" and "Barrier Litmus Tests and Cookbook")
	do
	{
		SLockFreeSingleLinkedListEntry *  volatile oldHead = (SLockFreeSingleLinkedListEntry* volatile)__ldrex((uint64_t*)&list.pNext); 
		element.pNext = oldHead;
	} while (!__strex((uint64_t*)&list.pNext, (uint64_t)&element));
}

void* CryInterlockedPopEntrySList(  SLockFreeSingleLinkedListHeader& list )
{
	volatile SLockFreeSingleLinkedListEntry* oldHead;
	volatile SLockFreeSingleLinkedListEntry* newHead;
	do
	{
		oldHead = (volatile SLockFreeSingleLinkedListEntry*)__ldrex((uint64_t*)&list.pNext);
		if (oldHead == NULL)
			return NULL;
		newHead = oldHead->pNext;
	} while (!__strex((uint64_t*)&list.pNext, (uint64_t)newHead));
	__dmb(); // Acquire barrier (as per "ARM Synchronization Primitives" and "Barrier Litmus Tests and Cookbook")
	return (void*)oldHead;
}

void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list)
{
	list.pNext = NULL;
}

void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list)
{
	volatile SLockFreeSingleLinkedListEntry* oldHead;
	do
	{
		oldHead = (volatile SLockFreeSingleLinkedListEntry*)__ldrex((uint64_t*)&list.pNext);
		if( oldHead == NULL )
			return NULL;
	} while (!__strex((uint64_t*)&list.pNext, NULL));
	__dmb(); // Acquire barrier (as per "ARM Synchronization Primitives" and "Barrier Litmus Tests and Cookbook")
	return (void*)oldHead;
}

//////////////////////////////////////////////////////////////////////////
// LINUX64 Implementation of Lockless Single Linked List
//////////////////////////////////////////////////////////////////////////
#elif defined(LINUX64) || defined(MAC) || defined(IOS_SIMULATOR)
	typedef __uint128_t uint128;

	//////////////////////////////////////////////////////////////////////////
	// Implementation for Linux64 with gcc using __int128_t
	//////////////////////////////////////////////////////////////////////////
	void CryInterlockedPushEntrySList( SLockFreeSingleLinkedListHeader& list,  SLockFreeSingleLinkedListEntry &element )
	{
		uint64 curSetting[2];
		uint64 newSetting[2];
		uint64 newPointer = (uint64)&element;
		do 
		{
			curSetting[0] = (uint64)list.pNext;
			curSetting[1] = list.salt;
			element.pNext = (SLockFreeSingleLinkedListEntry*)curSetting[0];
			newSetting[0] = newPointer; // new pointer
			newSetting[1] = curSetting[1]+1; // new salt
		}
		// while (false == __sync_bool_compare_and_swap( (volatile uint128*)&list.pNext,*(uint128*)&curSetting[0],*(uint128*)&newSetting[0] ));
	   while (0 == _InterlockedCompareExchange128((volatile int64*)&list.pNext, (int64)newSetting[1], (int64)newSetting[0], (int64*)&curSetting[0])); 
	}

	//////////////////////////////////////////////////////////////////////////
	void* CryInterlockedPopEntrySList(  SLockFreeSingleLinkedListHeader& list )
	{
		uint64 curSetting[2];
		uint64 newSetting[2];
		do 
		{
			curSetting[1]=list.salt;
			curSetting[0]=(uint64)list.pNext;
			if (curSetting[0]==0)
				return NULL;
			newSetting[0] = *(uint64*)curSetting[0]; // new pointer
			newSetting[1] = curSetting[1]+1; // new salt
		}
		//while (false == __sync_bool_compare_and_swap( (volatile uint128*)&list.pNext,*(uint128*)&curSetting[0],*(uint128*)&newSetting[0] ));
	   while (0 == _InterlockedCompareExchange128((volatile int64*)&list.pNext, (int64)newSetting[1], (int64)newSetting[0], (int64*)&curSetting[0])); 
		return (void*)curSetting[0];
	}

	//////////////////////////////////////////////////////////////////////////
	void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list)
	{
		list.salt = 0;
		list.pNext = NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list)
	{
		uint64 curSetting[2];
		uint64 newSetting[2];
		uint64 newSalt;
		uint64 newPointer;
		do 
		{
			curSetting[1]=list.salt;
			curSetting[0]=(uint64)list.pNext;
			if (curSetting[0]==0)
				return NULL;
			newSetting[0] = 0;
			newSetting[1] = curSetting[1]+1;
		}
	//	while (false == __sync_bool_compare_and_swap( (volatile uint128*)&list.pNext,*(uint128*)&curSetting[0],*(uint128*)&newSetting[0] ));
	   while (0 == _InterlockedCompareExchange128((volatile int64*)&list.pNext, (int64)newSetting[1], (int64)newSetting[0], (int64*)&curSetting[0])); 
		return (void*)curSetting[0];
	}
	//////////////////////////////////////////////////////////////////////////
#elif defined(LINUX32)
	//////////////////////////////////////////////////////////////////////////
	// Implementation for Linux32 with gcc using uint64
	//////////////////////////////////////////////////////////////////////////
	void CryInterlockedPushEntrySList( SLockFreeSingleLinkedListHeader& list,  SLockFreeSingleLinkedListEntry &element )
	{
		uint32 curSetting[2];
		uint32 newSetting[2];
		uint32 newPointer = (uint32)&element;
		do 
		{
			curSetting[0] = (uint32)list.pNext;
			curSetting[1] = list.salt;
			element.pNext = (SLockFreeSingleLinkedListEntry*)curSetting[0];
			newSetting[0] = newPointer; // new pointer
			newSetting[1] = curSetting[1]+1; // new salt
		}
		while (false == __sync_bool_compare_and_swap( (volatile uint64*)&list.pNext,*(uint64*)&curSetting[0],*(uint64*)&newSetting[0] ));
	}

	//////////////////////////////////////////////////////////////////////////
	void* CryInterlockedPopEntrySList(  SLockFreeSingleLinkedListHeader& list )
	{
		uint32 curSetting[2];
		uint32 newSetting[2];
		do 
		{
			curSetting[1]=list.salt;
			curSetting[0]=(uint32)list.pNext;
			if (curSetting[0]==0)
				return NULL;
			newSetting[0] = *(uint32*)curSetting[0]; // new pointer
			newSetting[1] = curSetting[1]+1; // new salt
		}
		while (false == __sync_bool_compare_and_swap( (volatile uint64*)&list.pNext,*(uint64*)&curSetting[0],*(uint64*)&newSetting[0] ));
		return (void*)curSetting[0];
	}

	//////////////////////////////////////////////////////////////////////////
	void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list)
	{
		list.salt = 0;
		list.pNext = NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list)
	{
		uint32 curSetting[2];
		uint32 newSetting[2];
		uint32 newSalt;
		uint32 newPointer;
		do 
		{
			curSetting[1]=list.salt;
			curSetting[0]=(uint32)list.pNext;
			if (curSetting[0]==0)
				return NULL;
			newSetting[0] = 0;
			newSetting[1] = curSetting[1]+1;
		}
		while (false == __sync_bool_compare_and_swap( (volatile uint64*)&list.pNext,*(uint64*)&curSetting[0],*(uint64*)&newSetting[0] ));
		return (void*)curSetting[0];
	}
	//////////////////////////////////////////////////////////////////////////
#endif
