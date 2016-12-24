
#ifndef __TVP_TIMER_H__
#define __TVP_TIMER_H__

class TVPTimerEventIntarface {
public:
	virtual ~TVPTimerEventIntarface() {}
	virtual void Handle() = 0;
};

template<typename T>
class TVPTimerEvent : public TVPTimerEventIntarface {
	void (T::*handler_)();
	T* owner_;

public:
	TVPTimerEvent( T* owner, void (T::*Handler)() ) : owner_(owner), handler_(Handler) {}
	void Handle() { (owner_->*handler_)(); }
};
class tTVPTimerImpl;
class TVPTimer {
	TVPTimerEventIntarface* event_;
	int		interval_;
	bool	enabled_;

	void UpdateTimer();

	void FireEvent() {
		if( event_ ) {
			event_->Handle();
		}
	}

	friend class tTVPTimerImpl;
	tTVPTimerImpl *impl_;

public:
	TVPTimer();
	~TVPTimer();

	template<typename T>
	void SetOnTimerHandler( T* owner, void (T::*Handler)() ) {
		if( event_ ) delete event_;
		event_ = new TVPTimerEvent<T>( owner, Handler );
		UpdateTimer();
	}

	void SetInterval( int i ) {
		if( interval_ != i ) {
			interval_ = i;
			UpdateTimer();
		}
	}
	int GetInterval() const {
		return interval_;
	}
	void SetEnabled( bool b ) {
		if( enabled_ != b ) {
			enabled_ = b;
			UpdateTimer();
		}
	}
	bool GetEnable() const { return enabled_; }

	static void ProgressAllTimer();
};


#endif // __TVP_TIMER_H__
