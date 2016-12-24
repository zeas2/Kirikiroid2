#include "ncbind/ncbind.hpp"

#define NCB_MODULE_NAME TJS_W("fftgraph.dll")

static void InitPlugin()
{
	TVPExecuteScript(TJS_W("function drawFFTGraph(){}"));
}

NCB_PRE_REGIST_CALLBACK(InitPlugin);
