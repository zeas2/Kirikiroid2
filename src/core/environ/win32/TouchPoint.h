// 環境依存ではないがいったんここに

#ifndef __TOUCH_POINT_H__
#define __TOUCH_POINT_H__

#include <cmath>

class TouchHandler {
public:
	static const int START_EVENT = 0x01 << 0;

	virtual void OnTouchScaling( double startdist, double currentdist, double cx, double cy, int flag ) = 0;
	virtual void OnTouchRotate( double startangle, double currentangle, double distance, double cx, double cy, int flag ) = 0;
	virtual void OnMultiTouch() = 0;
};

struct TouchPoint {
	static const int USE_POINT = 0x01 << 0;
	static const int START_SCALING = 0x01 << 1;
	static const int START_ROT = 0x01 << 2;
	tjs_uint32 id;
	double sx;
	double sy;
	double x;
	double y;
	tjs_uint32 flag;
	tjs_uint32 tick;
	TouchPoint() : id(0), sx(0), sy(0), x(0), y(0), flag(0), tick(0) {
	}
};


class TouchPointList {
	static const double SCALE_THRESHOLD;
	static const double ROTATE_THRESHOLD;
	static const int INIT_COUNT = 4;

	TouchHandler* handler_;
	TouchPoint* touch_points_;
	int count_;

	double scale_threshold_;
	double rotate_threshold_;

private:
	int FindEmptyEntry() {
		for( int i = 0; i < count_; i++ ) {
			if( (touch_points_[i].flag & TouchPoint::USE_POINT) == 0 ) {
				return i;
			}
		}
		int idx = count_;
		GrowPoints();
		return idx;
	}
	int FindEntry( tjs_uint32 id ) {
		for( int i = 0; i < count_; i++ ) {
			if( (touch_points_[i].flag & TouchPoint::USE_POINT) && (touch_points_[i].id == id) ) {
				return i;
			}
		}
		return -1;
	}
	int FindOtherEntry( tjs_uint32 id ) {
		for( int i = 0; i < count_; i++ ) {
			if( (touch_points_[i].flag & TouchPoint::USE_POINT) && (touch_points_[i].id != id) ) {
				return i;
			}
		}
		return -1;
	}
	void ClearIndex( int index ) {
		memset( &touch_points_[index], 0, sizeof(TouchPoint) );
	}
	void GrowPoints() {
		int count = count_ * 2;
		TouchPoint* points = new TouchPoint[count];
		for( int i = 0; i < count_; i++ ) {
			points[i] = touch_points_[i];
		}
		delete[] touch_points_;
		touch_points_ = points;
		count_ = count;
	}
public:
	TouchPointList( TouchHandler* handler )
	: handler_(handler) {
		scale_threshold_ = SCALE_THRESHOLD;
		rotate_threshold_ = ROTATE_THRESHOLD;
		count_ = INIT_COUNT;
		touch_points_ = new TouchPoint[count_];
	}
	~TouchPointList() {
		if( touch_points_ ) delete[] touch_points_;
		touch_points_ = NULL;
	}
	
	void TouchDown( double x, double y, double cx, double cy, tjs_uint32 id, tjs_uint32 tick ) {
		int idx = FindEmptyEntry();
		touch_points_[idx].sx = touch_points_[idx].x = x;
		touch_points_[idx].sy = touch_points_[idx].y = y;
		touch_points_[idx].flag = TouchPoint::USE_POINT;
		touch_points_[idx].id = id;
		touch_points_[idx].tick = tick;
		int num_of_points = CountUsePoint();
		if( num_of_points >= 2 ) {
			handler_->OnMultiTouch();
		}
	}
	void TouchMove( double x, double y, double cx, double cy, tjs_uint32 id, tjs_uint32 tick ) {
		int num_of_points = CountUsePoint();
		// 2点タッチのみ反応
		if( num_of_points == 2 ) {
			int targetidx = FindOtherEntry(id);
			int selfidx = FindEntry( id );
			if( targetidx >= 0 && selfidx >= 0 ) {
				TouchPoint& target = touch_points_[targetidx];
				TouchPoint& self = touch_points_[selfidx];

				double dx = (target.sx - self.sx);
				double dy = (target.sy - self.sy);
				double startdistance = std::sqrt( dx*dx +dy*dy );
				double startangle = std::atan2( dy, dx );
				dx = (target.x - x);
				dy = (target.y - y);
				double currentdistance = std::sqrt( dx*dx +dy*dy );
				double curentangle = std::atan2( dy, dx );
				double cx = (target.x + x) * 0.5;
				double cy = (target.y + y) * 0.5;

				double distance = std::abs(currentdistance-startdistance);
				bool scale_started = (self.flag & TouchPoint::START_SCALING) != 0;
				if( scale_started || (distance > scale_threshold_) ) {
					if( (target.flag & TouchPoint::START_SCALING) == 0 ) {
						int flag = ( scale_started == false ) ? TouchHandler::START_EVENT : 0;
						self.flag |= TouchPoint::START_SCALING;
						handler_->OnTouchScaling( startdistance, currentdistance, cx, cy, flag );
					}
				}
				dx = self.sx - x;
				dy = self.sy - y;
				double dist = std::sqrt( dx*dx +dy*dy );
				dx = target.sx - target.x;
				dy = target.sy - target.y;
				dist += std::sqrt( dx*dx +dy*dy );
				bool rotate_started = (self.flag & TouchPoint::START_ROT) != 0;
				if( rotate_started || (dist > rotate_threshold_) ) {
					if( (target.flag & TouchPoint::START_ROT) == 0 ) {
						int flag = ( rotate_started == false ) ? TouchHandler::START_EVENT : 0;
						self.flag |= TouchPoint::START_ROT;
						handler_->OnTouchRotate( startangle, curentangle, currentdistance, cx, cy, flag );
					}
				}
			}
		}
		int selfidx = FindEntry( id );
		if( selfidx >= 0 ) {
			TouchPoint& self = touch_points_[selfidx];
			self.x = x;
			self.y = y;
			self.tick = tick;
		}
		if( num_of_points >= 2 ) {
			handler_->OnMultiTouch();
		}
	}
	void TouchUp( double x, double y, double cx, double cy, tjs_uint32 id, tjs_uint32 tick ) {
		int num_of_points = CountUsePoint();
		if( num_of_points >= 2 ) {
			handler_->OnMultiTouch();
		}

		int idx = FindEntry(id);
		if( idx >= 0 ) {
			ClearIndex(idx);
		}
	}
	void SetScaleThreshold( double threshold ) {
		scale_threshold_ = threshold;
	}
	double GetScaleThreshold() const {
		return scale_threshold_;
	}
	void SetRotateThreshold( double threshold ) {
		rotate_threshold_ = threshold;
	}
	double GetRotateThreshold() const {
		return rotate_threshold_;
	}
	int CountUsePoint() const {
		int count = 0;
		for( int i = 0; i < count_; i++ ) {
			if( touch_points_[i].flag & TouchPoint::USE_POINT ) {
				count++;
			}
		}
		return count;
	}
	double GetStartX( int index ) const {
		int count = 0;
		for( int i = 0; i < count_; i++ ) {
			if( touch_points_[i].flag & TouchPoint::USE_POINT ) {
				if( count == index ) {
					return touch_points_[i].sx;
				}
				count++;
			}
		}
		return 0;
	}
	double GetStartY( int index ) const {
		int count = 0;
		for( int i = 0; i < count_; i++ ) {
			if( touch_points_[i].flag & TouchPoint::USE_POINT ) {
				if( count == index ) {
					return touch_points_[i].sy;
				}
				count++;
			}
		}
		return 0;
	}
	double GetX( int index ) const {
		int count = 0;
		for( int i = 0; i < count_; i++ ) {
			if( touch_points_[i].flag & TouchPoint::USE_POINT ) {
				if( count == index ) {
					return touch_points_[i].x;
				}
				count++;
			}
		}
		return 0;
	}
	double GetY( int index ) const {
		int count = 0;
		for( int i = 0; i < count_; i++ ) {
			if( touch_points_[i].flag & TouchPoint::USE_POINT ) {
				if( count == index ) {
					return touch_points_[i].y;
				}
				count++;
			}
		}
		return 0;
	}
	tjs_uint32 GetID( int index ) const {
		int count = 0;
		for( int i = 0; i < count_; i++ ) {
			if( touch_points_[i].flag & TouchPoint::USE_POINT ) {
				if( count == index ) {
					return touch_points_[i].id;
				}
				count++;
			}
		}
		return 0;
	}
	tjs_uint32 GetTick( int index ) const {
		int count = 0;
		for( int i = 0; i < count_; i++ ) {
			if( touch_points_[i].flag & TouchPoint::USE_POINT ) {
				if( count == index ) {
					return touch_points_[i].tick;
				}
				count++;
			}
		}
		return 0;
	}
	bool Validate( int index ) const {
		int count = 0;
		for( int i = 0; i < count_; i++ ) {
			if( touch_points_[i].flag & TouchPoint::USE_POINT ) {
				if( count == index ) {
					return true;
				}
				count++;
			}
		}
		return false;
	}
};

#endif // __TOUCH_POINT_H__
