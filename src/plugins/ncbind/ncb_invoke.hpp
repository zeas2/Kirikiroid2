#ifndef _ncb_invoke_hpp_
#define _ncb_invoke_hpp_

struct MethodCaller {
	//--------------------------------------
	// template prototypes

	template   <typename MethodT                       > struct tMethodTraits; // has ResultType, ClassType, ArgsType typedefs / HasResult IsStatic IsConst enums
	//template <typename ResultT, typename ClassT, ... > struct tMethodType;   // has (method-typed) Type typedef
	template   <typename ResultT, typename ClassT, typename ArgsT> struct tMethodResolver;  // ArgsT = tMethodTraits::ArgsType / has MethodType (or Type) typedef

	// method parameter type-tag (use userdefined Functor)
	template <typename TypeT>                            struct tTypeTag { typedef TypeT Type; };
	template <int N>                                     struct tNumTag  { enum { Number = N }; };

	// method args helper template
	template <template <typename> class Templ, class ArgsT, int N> struct tArgsExtract; // { typedef Templ<ArgsT::Arg[n]Type>::Type T[n];  T[n] t[n]; ... (n=1-N) }
	template <class ArgsExtT, int N>                               struct tArgsSelect;  // { static inline ArgsExtT::T[N] &Get(ArgsExtT &arg) { return arg.t[N]; } };

#if 0 // userdefined Functor sample
	struct      ResultAndParamsHolderExample {
		// get params operator method
		template <int N, typename T> T operator ()(tNumTag<N> index, tTypeTag<T> dummy) const;
		// set result operator method
		template <typename T> bool operator ()(T result, tTypeTag<T> dummy);
		/*                  */bool operator ()(); // if result is void
	};
#endif

/**
	Usage:

	class Target {
	public:
		typedef int    ParamT1;
		typedef double ParamT2;
		int Method(ParamT1, ParamT2, ...);
		static void StaticMethod();
	};
	typedef class ResultAndParamsHolderFunctor {...} FunctT; // like ResultAndParamsHolderExample.

	Target t;
	FunctT p;

	MethodCaller::Invoke(p, &Target::Method, t);    // as: return p(t.Method(p(1, tTypeTag<int>()), p(2, tTypeTag<double>()), ...), tTypeTag<int>());
	MethodCaller::Invoke(p, &Target::StaticMethod); // as: return Target::StaticMethod(), p();

 */


//--------------------------------------
// inner helper templates prototype
private:
	template <typename ResultT, typename ClassT, typename ArgsT, class FncT> struct tMethodCallerImpl;
	template <                  typename ClassT, typename ArgsT, class FncT> struct tInstanceFactoryImpl;

	template <typename T> struct tMethodHasResult;

public:
#define FOREACH_INCLUDE "ncb_foreach.h"
#undef  FOREACH_START
#define FOREACH_START FOREACH_MAX
#undef  FOREACH_END
#define FOREACH_END   FOREACH_MAX

#define MARGS_DEF_EXT(n) typename T ## n = void
#define MARGS_PRM_EXT(n) typename T ## n
#define MARGS_ARG_EXT(n)          T ## n
#define MARGS_REF_EXT(n) typedef  T ## n Arg ## n ## Type;
#undef  FOREACH
#define FOREACH \
	template <                                   FOREACH_COMMA_EXT(MARGS_DEF_EXT)> struct tMethodArgs { FOREACH_SPACE_EXT(MARGS_REF_EXT) }; \
	template <typename ResultT, typename ClassT, FOREACH_COMMA_EXT(MARGS_DEF_EXT) > \
	struct tMethodType { typedef typename tMethodResolver<ResultT, ClassT, tMethodArgs<FOREACH_COMMA_EXT(MARGS_ARG_EXT)> >::MethodType Type; };
#include FOREACH_INCLUDE
#undef  MARGS_DEF_EXT
#undef  MARGS_PRM_EXT
#undef  MARGS_ARG_EXT
#undef  MARGS_REF_EXT

//--------------------------------------
// main template

#define MCIMPL_TEMPL tMethodCallerImpl< \
	typename tMethodTraits<MethodT>::ResultType, \
	typename tMethodTraits<MethodT>::ClassWithConstType, \
	typename tMethodTraits<MethodT>::ArgsType, \
	FncT>

	template <class FncT, typename MethodT>
	static bool Invoke(FncT io, MethodT const &m) {
		return MCIMPL_TEMPL::Invoke(io, m);
	}
private:
//	template <class FncT, typename MethodT>               static inline bool invokeSelect(FncT io, MethodT const &m, void*)        { return Invoke(io, m); } // for static method
	template <class FncT, typename MethodT>               static inline bool invokeSelect(FncT io, MethodT const &m, void*&)       { return Invoke(io, m); } // for static method
	template <class FncT, typename MethodT, class ClassT> static inline bool invokeSelect(FncT io, MethodT const &m, ClassT *inst) { return (inst != 0) && MCIMPL_TEMPL::Invoke(io, m, inst); } // for class method
public:
	template <class FncT, typename MethodT> 
	static bool Invoke(FncT io, MethodT const &m, typename tMethodTraits<MethodT>::ClassWithConstType *inst) {
		return invokeSelect(io, m, inst);
	}
	template <class FncT, typename MethodT>
	static bool Invoke(FncT io, MethodT const &m, typename tMethodTraits<MethodT>::ClassWithConstType &inst) {
		return MCIMPL_TEMPL::Invoke(io, m, &inst);
	}
	template <class FncT, typename MethodT>
	static typename tMethodTraits<MethodT>::ClassType * Factory(FncT io, tTypeTag<MethodT>) {
		return (tInstanceFactoryImpl<
				typename tMethodTraits<MethodT>::ClassType *, 
				typename tMethodTraits<MethodT>::ArgsType,
				FncT>::Factory(io));
	}
	template <class FncT, typename MethodT>
	static typename tMethodTraits<MethodT>::ResultType Factory(FncT io, MethodT const &m) {
		return (tInstanceFactoryImpl<
				typename tMethodTraits<MethodT>::ResultType, 
				typename tMethodTraits<MethodT>::ArgsType,
				FncT>::Factory(io, m));
	}
	template <class FncT, typename ClassT, typename ArgsT>
	static ClassT* Factory(FncT io, tTypeTag<ClassT>, tTypeTag<ArgsT>) {
		return (tInstanceFactoryImpl<ClassT*, ArgsT, FncT>::Factory(io));
	}
};

#undef MCIMPL_TEMPL
#define MC MethodCaller

template <typename T> struct MC::tMethodHasResult       { enum { HasResult = true  }; };
template <>           struct MC::tMethodHasResult<void> { enum { HasResult = false }; };

//--------------------------------------
// method {invoker-implement, type-resolver, traits} template specializations (by foreach extraction :D)

#define METHODCALLER_IMPL(res, cls, cnst) \
	template <class FncT         res ## _DEF /**/          cls ## _DEF /**/ FOREACH_COMMA /**/ FOREACH_COMMA_EXT(MCIMPL_TMPL_ARGS_EXT) >                 \
		struct MC::tMethodCallerImpl<res ## _REF, cnst ## _REF cls ## _REF2, MC::tMethodArgs < FOREACH_COMMA_EXT(MCIMPL_METH_ARGS_EXT) >, FncT > {       \
			typedef              res ## _REF              (cls ## _REF           *MethodType)( FOREACH_COMMA_EXT(MCIMPL_METH_ARGS_EXT)) cnst ## _REF;    \
			static inline bool Invoke(FncT io, MethodType const &mptr /**/ cls ## _COMMA /**/ cnst ## _REF /**/ cls ## _INST(inst)) {                    \
				/**/ return      res ## _RESULT(io)      ((cls ## _CALL(inst,mptr)        )(   FOREACH_COMMA_EXT(MCIMPL_READ_ARGS_EXT))) res ## _CLOSE(io); } \
		}
#define INSTANCEFACTORY_IMPL \
	template <class FncT                                 , class ClassT/**/ FOREACH_COMMA /**/ FOREACH_COMMA_EXT(MCIMPL_TMPL_ARGS_EXT) >                 \
		struct MC::tInstanceFactoryImpl<                         ClassT*,    MC::tMethodArgs < FOREACH_COMMA_EXT(MCIMPL_METH_ARGS_EXT) >, FncT > {       \
			typedef       ClassT*                                               (*MethodType)( FOREACH_COMMA_EXT(MCIMPL_METH_ARGS_EXT));                 \
			static inline ClassT* Factory(FncT io) { (void)io; return new ClassT           (   FOREACH_COMMA_EXT(MCIMPL_READ_ARGS_EXT)); }               \
			static inline ClassT* Factory(FncT io, MethodType const &mptr) { return (*mptr)(   FOREACH_COMMA_EXT(MCIMPL_READ_ARGS_EXT)); }               \
		}
#define METHODRESOLVER_SPECIALIZATION(cls, cnst) \
	template <         typename ResultT /**/               cls ## _DEF /**/ FOREACH_COMMA /**/ FOREACH_COMMA_EXT(MCIMPL_TMPL_ARGS_EXT) >                     \
		struct MC::tMethodResolver< ResultT,      cnst ## _REF cls ## _REF2, MC::tMethodArgs < FOREACH_COMMA_EXT(MCIMPL_METH_ARGS_EXT) > > {                 \
			typedef             ResultT                   (cls ## _REF           *MethodType)( FOREACH_COMMA_EXT(MCIMPL_METH_ARGS_EXT)) cnst ## _REF;        \
			typedef MethodType Type; };                                                                                                                      \
	template <         typename ResultT /**/               cls ## _DEF /**/ FOREACH_COMMA /**/ FOREACH_COMMA_EXT(MCIMPL_TMPL_ARGS_EXT) >                     \
	struct MC::tMethodTraits<   ResultT                   (cls ## _REF           *          )( FOREACH_COMMA_EXT(MCIMPL_METH_ARGS_EXT)) cnst ## _REF> {      \
			typedef             ResultT                   (cls ## _REF           *MethodType)( FOREACH_COMMA_EXT(MCIMPL_METH_ARGS_EXT)) cnst ## _REF;        \
			typedef             ResultT                                                                                                  ResultType;         \
			typedef                                        cls ## _REF2                                                                  ClassType;          \
			typedef                           cnst ## _REF cls ## _REF2                                                                  ClassWithConstType; \
			typedef                                                        tMethodArgs<        FOREACH_COMMA_EXT(MCIMPL_METH_ARGS_EXT) > ArgsType;           \
			enum { HasResult = tMethodHasResult<ResultType>::HasResult, IsStatic = cls ## _BOOL, IsConst = cnst ## _BOOL, ArgsCount = FOREACH_COUNT };       \
		}

#define MCIMPL_TMPL_ARGS_EXT(n) typename T ## n
#define MCIMPL_METH_ARGS_EXT(n) T ## n
#define MCIMPL_READ_ARGS_EXT(n) io(tNumTag<n>(), tTypeTag<T ## n>())

#define MCIMPL_VOID_DEF
#define MCIMPL_VOID_REF /*                 */void
#define MCIMPL_VOID_RESULT(io) /*          */(
#define MCIMPL_VOID_CLOSE(io) /*           */, io())
#define MCIMPL_NONVOID_DEF /*              */,typename ResultT
#define MCIMPL_NONVOID_REF /*              */ResultT
#define MCIMPL_NONVOID_RESULT(io) /*       */io(
#define MCIMPL_NONVOID_CLOSE(io) /*        */, tTypeTag<ResultT>())

#define MCIMPL_STATIC_DEF
#define MCIMPL_STATIC_REF
#define MCIMPL_STATIC_REF2 /*              */void
#define MCIMPL_STATIC_BOOL /*              */true
#define MCIMPL_STATIC_COMMA
#define MCIMPL_STATIC_INST(inst)
#define MCIMPL_STATIC_CALL(inst, method) /**/*method
#define MCIMPL_CLASS_DEF /*                */,class ClassT
#define MCIMPL_CLASS_REF /*                */ClassT::
#define MCIMPL_CLASS_REF2 /*               */ClassT
#define MCIMPL_CLASS_BOOL /*               */false
#define MCIMPL_CLASS_COMMA /*              */,
#define MCIMPL_CLASS_INST(inst) /*         */ClassT *inst
#define MCIMPL_CLASS_CALL(inst,method) /*  */inst->*method

#define MCIMPL_NONCONST_REF
#define MCIMPL_NONCONST_BOOL /*            */false
#define MCIMPL_CONST_REF /*                */const
#define MCIMPL_CONST_BOOL /*               */true


#define MAHELP_CONCAT0(a,b) a ## b
#define MAHELP_CONCAT(a,b) MAHELP_CONCAT0(a,b)
#define MAHELP_EXT(n) /* templ type */ typedef typename Templ<typename ArgsT::Arg ## n ## Type>::Type T ## n; /* instance */ T ## n  t ## n;
#define METHODARGSHELPER_IMPL \
	template <template <typename> class Templ, class ArgsT> struct MC::tArgsExtract<Templ, ArgsT, FOREACH_COUNT> { FOREACH_SPACE_EXT(MAHELP_EXT) }; \
	template <class ArgsT>                                  struct MC::tArgsSelect<ArgsT, FOREACH_COUNT> \
	{ static inline typename MAHELP_CONCAT(ArgsT::T,FOREACH_COUNT) &Get(ArgsT &arg) { return MAHELP_CONCAT(arg.t,FOREACH_COUNT); } }

#undef  FOREACH_START
#define FOREACH_START 0
#undef  FOREACH_END
#define FOREACH_END   FOREACH_MAX

#undef  FOREACH
#define FOREACH \
	METHODCALLER_IMPL(MCIMPL_VOID,    MCIMPL_STATIC, MCIMPL_NONCONST); \
	METHODCALLER_IMPL(MCIMPL_NONVOID, MCIMPL_STATIC, MCIMPL_NONCONST); \
	METHODCALLER_IMPL(MCIMPL_VOID,    MCIMPL_CLASS,  MCIMPL_NONCONST); \
	METHODCALLER_IMPL(MCIMPL_NONVOID, MCIMPL_CLASS,  MCIMPL_NONCONST); \
	METHODCALLER_IMPL(MCIMPL_VOID,    MCIMPL_CLASS,  MCIMPL_CONST);    \
	METHODCALLER_IMPL(MCIMPL_NONVOID, MCIMPL_CLASS,  MCIMPL_CONST);    \
	METHODRESOLVER_SPECIALIZATION(    MCIMPL_STATIC, MCIMPL_NONCONST); \
	METHODRESOLVER_SPECIALIZATION(    MCIMPL_CLASS,  MCIMPL_NONCONST); \
	METHODRESOLVER_SPECIALIZATION(    MCIMPL_CLASS,  MCIMPL_CONST);    \
	INSTANCEFACTORY_IMPL; \
	METHODARGSHELPER_IMPL;
#include FOREACH_INCLUDE

#undef MC
#undef METHODCALLER_IMPL
#undef INSTANCEFACTORY_IMPL
#undef METHODRESOLVER_SPECIALIZATION

#undef METHODARGSHELPER_IMPL
#undef MAHELP_EXT

#undef MCIMPL_TMPL_ARGS_EXT
#undef MCIMPL_METH_ARGS_EXT
#undef MCIMPL_READ_ARGS_EXT

#undef MCIMPL_VOID_DEF
#undef MCIMPL_VOID_REF
#undef MCIMPL_VOID_RESULT
#undef MCIMPL_VOID_CLOSE
#undef MCIMPL_NONVOID_DEF
#undef MCIMPL_NONVOID_REF
#undef MCIMPL_NONVOID_RESULT
#undef MCIMPL_NONVOID_CLOSE

#undef MCIMPL_STATIC_DEF
#undef MCIMPL_STATIC_REF
#undef MCIMPL_STATIC_BOOL
#undef MCIMPL_STATIC_CALL
#undef MCIMPL_CLASS_DEF
#undef MCIMPL_CLASS_REF
#undef MCIMPL_CLASS_BOOL
#undef MCIMPL_CLASS_CALL

#undef MCIMPL_NONCONST_REF
#undef MCIMPL_NONCONST_BOOL
#undef MCIMPL_CONST_REF
#undef MCIMPL_CONST_BOOL


#define MCCAST_20(MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20>::Type>(MM)
#define MCCAST_19(MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19     ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19     >::Type>(MM)
#define MCCAST_18(MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18          ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18          >::Type>(MM)
#define MCCAST_17(MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17               ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17               >::Type>(MM)
#define MCCAST_16(MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16                    ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16                    >::Type>(MM)
#define MCCAST_15(MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15                         ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15                         >::Type>(MM)
#define MCCAST_14(MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14                              ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14                              >::Type>(MM)
#define MCCAST_13(MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13                                   ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13                                   >::Type>(MM)
#define MCCAST_12(MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12                                        ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12                                        >::Type>(MM)
#define MCCAST_11(MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11                                             ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11                                             >::Type>(MM)
#define MCCAST_10(MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10                                                  ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10                                                  >::Type>(MM)
#define MCCAST_9( MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9                                                       ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8, T9                                                       >::Type>(MM)
#define MCCAST_8( MM, MR, MC, T1, T2, T3, T4, T5, T6, T7, T8                                                           ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7, T8                                                           >::Type>(MM)
#define MCCAST_7( MM, MR, MC, T1, T2, T3, T4, T5, T6, T7                                                               ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6, T7                                                               >::Type>(MM)
#define MCCAST_6( MM, MR, MC, T1, T2, T3, T4, T5, T6                                                                   ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5, T6                                                                   >::Type>(MM)
#define MCCAST_5( MM, MR, MC, T1, T2, T3, T4, T5                                                                       ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4, T5                                                                       >::Type>(MM)
#define MCCAST_4( MM, MR, MC, T1, T2, T3, T4                                                                           ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3, T4                                                                           >::Type>(MM)
#define MCCAST_3( MM, MR, MC, T1, T2, T3                                                                               ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2, T3                                                                               >::Type>(MM)
#define MCCAST_2( MM, MR, MC, T1, T2                                                                                   ) static_cast<MethodCaller::tMethodType<MR, MC, T1, T2                                                                                   >::Type>(MM)
#define MCCAST_1( MM, MR, MC, T1                                                                                       ) static_cast<MethodCaller::tMethodType<MR, MC, T1                                                                                       >::Type>(MM)
#define MCCAST_0( MM, MR, MC                                                                                           ) static_cast<MethodCaller::tMethodType<MR, MC                                                                                           >::Type>(MM)


#undef  FOREACH_START
#define FOREACH_START 0
#undef  FOREACH_END
#define FOREACH_END   FOREACH_MAX
#define MCAST_PRM_EXT(n) typename T ## n
#define MCAST_ARG_EXT(n)          T ## n
#undef  FOREACH
#define FOREACH \
	template <             typename ResultT, typename ClassT /**/ FOREACH_COMMA /**/ FOREACH_COMMA_EXT(MCAST_PRM_EXT) > \
		static inline   typename MethodCaller::tMethodType<ResultT, ClassT /**/ FOREACH_COMMA /**/ FOREACH_COMMA_EXT(MCAST_ARG_EXT)>::Type \
			method_cast(typename MethodCaller::tMethodType<ResultT, ClassT /**/ FOREACH_COMMA /**/ FOREACH_COMMA_EXT(MCAST_ARG_EXT)>::Type method) { return method; }
#include FOREACH_INCLUDE
#undef  MCAST_PRM_EXT
#undef  MCAST_ARG_EXT

template <typename T> static inline T method_cast(T method) { return method; }

#endif
