
#include "tjsCommHead.h"
#include "TVPTimer.h"
#include "TickCount.h"

struct tTVPTimerImpl {
	tTVPTimerImpl *Prev = nullptr, *Next = nullptr;
	TVPTimer *pTimer = nullptr;
	uint32_t Index;
	~tTVPTimerImpl() {
		if (Prev) Prev->Next = Next;
		if (Next) Next->Prev = Prev;
	}
	void SetTimer(TVPTimer * p) { pTimer = p; }
	void Kill() {
		if (Prev) Prev->Next = Next;
		if (Next) Next->Prev = Prev;
		Next = Prev = nullptr;
	}
	void Set();
	void Set(int idx);
	void Add(tTVPTimerImpl* p) {
		p->Next = Next;
		Next = p;
		p->Prev = this;
	}
	void FireNext();
};

static tTVPTimerImpl
	_timer_v1[256],
	_timer_v2[64],
	_timer_v3[64],
	_timer_v4[64],
	_timer_v5[64];
static struct t_timer_idx {
	union {
		struct {
			unsigned int _idx_v1 : 8;
			unsigned int _idx_v2 : 6;
			unsigned int _idx_v3 : 6;
			unsigned int _idx_v4 : 6;
			unsigned int _idx_v5 : 6;
		};
		uint32_t _idx;
	};
	t_timer_idx(uint32_t idx) : _idx(idx) {}
} _timer_idx(TVPGetRoughTickCount32());

void tTVPTimerImpl::Set() {
	int interval = pTimer->GetInterval();
	Set(interval);
}

void tTVPTimerImpl::Set(int idx)
{
	idx += _timer_idx._idx_v1;
	Index = idx;
	if (idx < 256) {
		_timer_v1[idx].Add(this);
		return;
	}
	idx >>= 8;
	idx += _timer_idx._idx_v2;
	if (idx < 64) {
		_timer_v2[idx].Add(this);
		return;
	}
	idx >>= 6;
	idx += _timer_idx._idx_v3;
	if (idx < 64) {
		_timer_v3[idx].Add(this);
		return;
	}
	idx >>= 6;
	idx += _timer_idx._idx_v4;
	if (idx < 64) {
		_timer_v4[idx].Add(this);
		return;
	}
	idx >>= 6;
	idx += _timer_idx._idx_v5;
	_timer_v5[idx].Add(this);
}

static tTVPTimerImpl _processedTimer;

void tTVPTimerImpl::FireNext() {
	tTVPTimerImpl* p = Next;
	if (!p) return;
	Next = nullptr;

	p->pTimer->FireEvent();
	p->FireNext();
	_processedTimer.Add(p);
// 	int interval = p->pTimer->GetInterval();
// 	p->Set(interval);
}

void TVPTimer::ProgressAllTimer()
{
	uint32_t curTick = TVPGetRoughTickCount32();
	int past = curTick - _timer_idx._idx;
	for (int i = 0; i < past; ++i) {
		_timer_v1[_timer_idx._idx_v1].FireNext();
		++_timer_idx._idx;
		if (_timer_idx._idx_v1 == 0) { // v2 step
			tTVPTimerImpl* root = &_timer_v2[_timer_idx._idx_v2];
			tTVPTimerImpl* p = root->Next;
			root->Next = nullptr;
			while (p) {
				tTVPTimerImpl* next = p->Next;
				p->Next = nullptr;
				p->Index &= 255;
				_timer_v1[p->Index].Add(p);
				p = next;
			}
		}
		if (_timer_idx._idx_v2 == 0) { // v3 step
			tTVPTimerImpl* root = &_timer_v3[_timer_idx._idx_v3];
			tTVPTimerImpl* p = root->Next;
			root->Next = nullptr;
			while (p) {
				tTVPTimerImpl* next = p->Next;
				p->Next = nullptr;
				int idx = p->Index >> 8;
				p->Index &= 255 | (63 << 8);
				_timer_v2[idx].Add(p);
				p = next;
			}
		}
		if (_timer_idx._idx_v3 == 0) { // v4 step
			tTVPTimerImpl* root = &_timer_v4[_timer_idx._idx_v4];
			tTVPTimerImpl* p = root->Next;
			root->Next = nullptr;
			while (p) {
				tTVPTimerImpl* next = p->Next;
				p->Next = nullptr;
				int idx = p->Index >> (8 + 6);
				p->Index &= 255 | (63 << 8) | (63 << (8 + 6));
				_timer_v3[idx].Add(p);
				p = next;
			}
		}
		if (_timer_idx._idx_v4 == 0) { // v5 step
			tTVPTimerImpl* root = &_timer_v5[_timer_idx._idx_v5];
			tTVPTimerImpl* p = root->Next;
			root->Next = nullptr;
			while (p) {
				tTVPTimerImpl* next = p->Next;
				p->Next = nullptr;
				int idx = p->Index >> (8 + 6 + 6);
				p->Index &= 255 | (63 << 8) | (63 << (8 + 6)) | (63 << (8 + 6 + 6));
				_timer_v4[idx].Add(p);
				p = next;
			}
		}
	}
	tTVPTimerImpl* p = _processedTimer.Next;
	_processedTimer.Next = nullptr;
	while (p) {
		tTVPTimerImpl* next = p->Next;
		p->Next = nullptr;
		p->Set();
		p = next;
	}
}

TVPTimer::TVPTimer() : event_(NULL), interval_(1000), enabled_(true) {
	impl_ = new tTVPTimerImpl;
	impl_->pTimer = this;
}

TVPTimer::~TVPTimer() {
	delete impl_;
	if( event_ ) {
		delete event_;
	}
}

void TVPTimer::UpdateTimer() {
	impl_->Kill();
	if( interval_ > 0 && enabled_ && event_ != NULL ) {
		impl_->Set();
	}
}

