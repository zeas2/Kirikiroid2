

#ifndef __ALIGNED_ALLOCATOR_H__
#define __ALIGNED_ALLOCATOR_H__

#include <malloc.h>		// _aligned_malloc and _aligned_free
#include <memory>		// std::allocator

// STL allocator
template< class T, int TAlign=4 >
struct aligned_allocator : public std::allocator<T>
{
	static const int ALIGN_SIZE = TAlign;
	typedef T* pointer;
	template <class U> struct rebind    { typedef aligned_allocator<U,TAlign> other; };
	aligned_allocator() throw() {}
	aligned_allocator(const aligned_allocator&) throw () {}
	template <class U> aligned_allocator(const aligned_allocator<U>&) throw() {}
	template <class U> aligned_allocator& operator=(const aligned_allocator<U>&) throw()  {}
	// allocate
	pointer allocate(TJS::tjs_uint c, const void* hint = 0) {
		return static_cast<pointer>( TJS::TJSAlignedAlloc( sizeof(T)*c, TAlign ) );
	}
	// deallocate
	void deallocate(pointer p, TJS::tjs_uint n) {
		TJS::TJSAlignedDealloc(p);
	}
};

#endif // __ALIGNED_ALLOCATOR_H__


