#ifndef _SMART_PTR_H_
#define _SMART_PTR_H_
#include <type_traits>

#ifdef __SPU__
	#define CryFatalError printf
#else
	void CryFatalError(const char *, ...) PRINTF_PARAMS(1, 2);
#endif
#if defined(APPLE)
    #include <cstddef>
#endif
//////////////////////////////////////////////////////////////////
// SMART POINTER
//////////////////////////////////////////////////////////////////
template <class _I> class _smart_ptr 
{
private:
	_I* p;

	template <typename>
	friend class _smart_ptr;

public:
    _smart_ptr() : p(NULL) {}

	_smart_ptr(_I* p_) : p(p_)
	{
		if (p)
			p->AddRef();
	}

	_smart_ptr(const _smart_ptr &p_) : p(p_.p)
	{
		if (p)
			p->AddRef();
	}

	template <typename _Y>
	_smart_ptr(const _smart_ptr<_Y> &p_, typename std::enable_if<std::is_convertible<_Y *, _I *>::value, int>::type dummy = 0) : p(p_.p)
	{
		if (p)
			p->AddRef();
	}

	_smart_ptr(_smart_ptr &&p_) : p(p_.p)
	{
		p_.p = NULL;
	}

	template <typename _Y>
	_smart_ptr(_smart_ptr<_Y> &&p_, typename std::enable_if<std::is_convertible<_Y *, _I *>::value, int>::type dummy = 0) : p(p_.p)
	{
		p_.p = NULL;
	}

	~_smart_ptr()
	{
		if (p)
			p->Release();
	}

	void swap(_smart_ptr& other)
	{
		std::swap(p, other.p);
	}

	void reset()
	{
		if (p)
			p->Release();
		p = NULL;
	}

	void reset(_I* newp)
	{
		if (newp)
			newp->AddRef();
		if (p)
			p->Release();
		p = newp;
	}

	operator _I*() const
	{ 
		return p;
	} 
	
	_I& operator*() const
	{ 
		return *p;
	}
	
	_I* operator->() const
	{ 
		return p;
	}
	
	_I* get() const
	{ 
		return p;
	}

	_smart_ptr& operator=(_I* newp)
	{
		reset(newp);
		return *this;
	}

	_smart_ptr& operator=(const _smart_ptr &newp)
	{
		reset(newp.get());
		return *this;
	}

	template <typename _Y>
	typename std::enable_if<std::is_convertible<_Y *, _I *>::value, _smart_ptr&>::type operator=(const _smart_ptr<_Y> &newp)
	{
		reset(newp.get());
		return *this;
	}

	_smart_ptr& operator=(_smart_ptr &&newp)
	{
		swap(newp);
		newp.reset();
		return *this;
	}

	template <typename _Y>
	typename std::enable_if<std::is_convertible<_Y *, _I *>::value, _smart_ptr&>::type operator=(_smart_ptr<_Y> &&newp)
	{
		swap(newp);
		newp.reset();
		return *this;
	}

	// set our contained pointer to null, but don't call Release()
	// useful for when we want to do that ourselves, or when we know that
	// the contained pointer is invalid
	friend _I* ReleaseOwnership( _smart_ptr<_I>& ptr )
	{
		_I * ret = ptr.p;
		ptr.p = 0;
		return ret;
	}

	AUTO_STRUCT_INFO
};

namespace std
{
	template <typename T>
	ILINE void swap(_smart_ptr<T>& a, _smart_ptr<T>& b)
	{
		a.swap(b);
	}
}

// reference target for smart pointer
// implements AddRef() and Release() strategy using reference counter of the specified type
template <typename Counter> class _reference_target
{
public:
	_reference_target():
		m_nRefCounter (0)
	{
	}

	virtual ~_reference_target()
	{
		//assert (!m_nRefCounter);
	}
	
	void AddRef()
	{
		CHECK_REFCOUNT_CRASH(m_nRefCounter>=0);
		++m_nRefCounter;
	}

	void Release()
	{
		CHECK_REFCOUNT_CRASH(m_nRefCounter>0);
		if (--m_nRefCounter == 0)
		{
			delete this;
		}
		else if (m_nRefCounter < 0)
		{
			assert(0);
			CryFatalError( "Deleting Reference Counted Object Twice" );
		}
	}
	// Warning: use for debugging/statistics purposes only!
	Counter NumRefs()
	{
		return m_nRefCounter;
	}
protected:
	Counter m_nRefCounter;
};

// default implementation is int counter - for better alignment
typedef _reference_target<int> _reference_target_t;


// reference target for smart pointer with configurable destruct function 
// implements AddRef() and Release() strategy using reference counter of the specified type
template <typename T, typename Counter = int> class _cfg_reference_target
{
public:

  typedef void (*DeleteFncPtr)(void*);

	_cfg_reference_target():
		m_nRefCounter (0), 
    m_pDeleteFnc(operator delete)
	{
	}

	explicit _cfg_reference_target(DeleteFncPtr pDeleteFnc):
		m_nRefCounter (0), 
    m_pDeleteFnc(pDeleteFnc)
	{
	}

	virtual ~_cfg_reference_target()
	{
	}
	
	void AddRef()
	{
		CHECK_REFCOUNT_CRASH(m_nRefCounter>=0);
		++m_nRefCounter;
	}

	void Release()
	{
		CHECK_REFCOUNT_CRASH(m_nRefCounter>0);
		if (--m_nRefCounter == 0)
		{
      assert(m_pDeleteFnc);
      static_cast<T*>(this)->~T();
      m_pDeleteFnc(this);
		}
		else if (m_nRefCounter < 0)
		{
			assert(0);
			CryFatalError( "Deleting Reference Counted Object Twice" );
		}
	}

  // Sets the delete function with which this object is supposed to be deleted 
  void SetDeleteFnc(DeleteFncPtr pDeleteFnc) { m_pDeleteFnc = pDeleteFnc; }

	// Warning: use for debugging/statistics purposes only!
	Counter NumRefs()
	{
		return m_nRefCounter;
	}
protected:
	Counter m_nRefCounter;
  DeleteFncPtr m_pDeleteFnc; 
};


// base class for interfaces implementing reference counting
// derive your interface from this class and the descendants won't have to implement
// the reference counting logic
template <typename Counter> class _i_reference_target
{
public:
	_i_reference_target():
		m_nRefCounter (0)
	{
	}

	virtual ~_i_reference_target()
	{
	}

	virtual void AddRef()
	{
		++m_nRefCounter;
	}

	virtual void Release()
	{
		if (--m_nRefCounter == 0)
		{
			delete this;
		}
		else if (m_nRefCounter < 0)
		{
			assert(0);
			CryFatalError( "Deleting Reference Counted Object Twice" );
		}
	}

	// Warning: use for debugging/statistics purposes only!
	Counter NumRefs()	const
	{
		return m_nRefCounter;
	}
protected:
	Counter m_nRefCounter;
};

class CMultiThreadRefCount
{
public:
	CMultiThreadRefCount() : m_cnt(0) {}
	virtual ~CMultiThreadRefCount() {}

	inline int AddRef()
	{
		return CryInterlockedIncrement(&m_cnt);
	}
	inline int Release()
	{
		const int nCount = CryInterlockedDecrement(&m_cnt);
		assert(nCount >= 0);
		if (nCount == 0)
		{
			delete this;
		}
		else if (nCount < 0)
		{
			assert(0);
			CryFatalError( "Deleting Reference Counted Object Twice" );
		}
		return nCount;
	}

	inline int GetRefCount()	const { return m_cnt; }

private:
	volatile int m_cnt;
};

// base class for interfaces implementing reference counting that needs to be thread-safe
// derive your interface from this class and the descendants won't have to implement
// the reference counting logic
template <typename Counter>
class _i_multithread_reference_target
{
public:
	_i_multithread_reference_target() 
		: m_nRefCounter(0) 
	{
	}

	virtual ~_i_multithread_reference_target() 
	{
	}

	virtual void AddRef()
	{
		CryInterlockedIncrement(&m_nRefCounter);
	}

	virtual void Release()
	{
		const int nCount = CryInterlockedDecrement(&m_nRefCounter);
		assert(nCount >= 0);
		if (nCount == 0)
		{
			delete this;
		}
		else if (nCount < 0)
		{
			assert(0);
			CryFatalError( "Deleting Reference Counted Object Twice" );
		}
	}

	Counter NumRefs()	const { return m_nRefCounter; }

protected:

	volatile Counter m_nRefCounter;
};

typedef _i_reference_target<int> _i_reference_target_t;
typedef _i_multithread_reference_target<int> _i_multithread_reference_target_t;

// TYPEDEF_AUTOPTR macro, declares Class_AutoPtr, which is the smart pointer to the given class,
// and Class_AutoArray, which is the array(STL vector) of autopointers
#ifdef ENABLE_NAIIVE_AUTOPTR
// naiive autopointer makes it easier for Visual Assist to parse the declaration and sometimes is easier for debug
#define TYPEDEF_AUTOPTR(T) typedef T* T##_AutoPtr; typedef std::vector<T##_AutoPtr> T##_AutoArray;
#else
#define TYPEDEF_AUTOPTR(T) typedef _smart_ptr<T> T##_AutoPtr; typedef std::vector<T##_AutoPtr> T##_AutoArray;
#endif

#endif //_SMART_PTR_H_
