#pragma once
#include "KRMovieDef.h"
#include <atomic>
#include <assert.h>

NS_KRMOVIE_BEGIN

template<typename T> struct IRef
{
	IRef() : m_refs(1) {}
	virtual ~IRef() {}

	IRef(const IRef &) = delete;
	IRef &operator=(const IRef &) = delete;

	virtual T* AddRef()
	{
		m_refs++;
		return (T*)this;
	}

	virtual long Release()
	{
		intptr_t count = --m_refs;
		assert(count >= 0);
		if (count == 0) delete (T*)this;
		return count;
	}
	std::atomic_intptr_t m_refs;
};


NS_KRMOVIE_END