#pragma once
#define NS_KRMOVIE_BEGIN namespace KRMovie {
#define NS_KRMOVIE_END }

#define SAFE_DELETE(p)       do { delete (p);     (p)=NULL; } while (0)
#define SAFE_RELEASE(p)      do { if(p) { (p)->Release(); (p)=NULL; } } while (0)

#ifdef _MSC_VER
#define NOMINMAX
#endif