#include "tjsCommHead.h"
#include "NativeEventQueue.h"
#include "Application.h"

void NativeEventQueueImplement::PostEvent(const NativeEvent& ev) {
	Application->PostUserMessage([this, ev](){ Dispatch(*const_cast<NativeEvent*>(&ev)); }, this);
}

void NativeEventQueueImplement::Clear(int msg)
{
	Application->FilterUserMessage([this, msg](std::vector<std::tuple<void*, int, tTVPApplication::tMsg> > &lst){
		for (auto it = lst.begin(); it != lst.end();) {
			if (std::get<0>(*it) == this && (!msg || std::get<1>(*it) == msg)) {
				it = lst.erase(it);
			} else {
				++it;
			}
		}
	});
}

