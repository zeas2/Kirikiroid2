/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * -------------------------------------------------------------------
 * このソースコードは Android のソースコードから吉里吉里Zに合うように
 * 修正したものです。
 * 複数ポイント同時管理していたものは、IDの割り当て方がWindowsでは異な
 * る(一定の範囲に収まらない)ため、バラバラで管理する形に変更。
 * 時間はms単位に変更。
 */

#ifndef _LIBINPUT_VELOCITY_TRACKER_H
#define _LIBINPUT_VELOCITY_TRACKER_H

/*
 * Implements a particular velocity tracker algorithm.
 */
class VelocityTrackerStrategy {
public:
    struct Position {
        float x, y;
    };

    struct Estimator {
        static const size_t MAX_DEGREE = 4;

        // Estimator time base.
    	tjs_uint64 time;

        // Polynomial coefficients describing motion in X and Y.
        float xCoeff[MAX_DEGREE + 1], yCoeff[MAX_DEGREE + 1];

        // Polynomial degree (number of coefficients), or zero if no information is
        // available.
        tjs_uint32 degree;

        // Confidence (coefficient of determination), between 0 (no fit) and 1 (perfect fit).
        float confidence;

        inline void clear() {
            time = 0;
            degree = 0;
            confidence = 0;
            for (size_t i = 0; i <= MAX_DEGREE; i++) {
                xCoeff[i] = 0;
                yCoeff[i] = 0;
            }
        }
    };
protected:
    VelocityTrackerStrategy() { }

public:
    virtual ~VelocityTrackerStrategy() { }

    virtual void clear() = 0;
    virtual void addMovement( tjs_uint64 eventTime, float x, float y ) = 0;
    virtual bool getEstimator( Estimator* outEstimator) const = 0;
};


/*
 * Velocity tracker algorithm based on least-squares linear regression.
 */
class LeastSquaresVelocityTrackerStrategy : public VelocityTrackerStrategy {
public:
    enum Weighting {
        WEIGHTING_NONE,		// No weights applied.  All data points are equally reliable.
        WEIGHTING_DELTA,	// Weight by time delta.  Data points clustered together are weighted less.
        WEIGHTING_CENTRAL,	// Weight such that points within a certain horizon are weighed more than those outside of that horizon.
        WEIGHTING_RECENT,	// Weight such that points older than a certain amount are weighed less.
    };

    // Degree must be no greater than Estimator::MAX_DEGREE.
    LeastSquaresVelocityTrackerStrategy(tjs_uint32 degree, Weighting weighting = WEIGHTING_NONE);
    virtual ~LeastSquaresVelocityTrackerStrategy();

    virtual void clear();
    virtual void addMovement( tjs_uint64 eventTime, float x, float y );
    virtual bool getEstimator( Estimator* outEstimator ) const;

private:
    // Sample horizon.
    // We don't use too much history by default since we want to react to quick
    // changes in direction.
    //static const tjs_uint64 HORIZON = 100 * 1000000; // 100 ms
	static const tjs_uint64 HORIZON = 100; // 100 ms

    // Number of samples to keep.
    static const tjs_uint32 HISTORY_SIZE = 20;

    struct Movement {
        tjs_uint64 eventTime;
        Position positions;
    	bool usingThis;

        inline const Position& getPosition() const {
            return positions;
        }
    };

    float chooseWeight(tjs_uint32 index) const;

    const tjs_uint32 mDegree;
    const Weighting mWeighting;
    tjs_uint32 mIndex;
    Movement mMovements[HISTORY_SIZE];
};


/*
 * Calculates the velocity of pointer movements over time.
 * ポインタの動きの速度を計算する
 */
class VelocityTracker {
public:

    // Creates a velocity tracker using the specified strategy.
    // If strategy is NULL, uses the default strategy for the platform.
    VelocityTracker();
    ~VelocityTracker();

    // Resets the velocity tracker state.
	void clear() {
		mID = -1;
		mStrategy.clear();
	}

    // Adds movement information for a set of pointers.
    // The idBits bitfield specifies the pointer ids of the pointers whose positions
    // are included in the movement.
    // The positions array contains position information for each pointer in order by
    // increasing id.  Its size should be equal to the number of one bits in idBits.
	void addMovement( tjs_uint64 eventTime, float x, float y ) {
		mStrategy.addMovement( eventTime, x, y );
	}

    // Gets the velocity of the specified pointer id in position units per second.
    // Returns false and sets the velocity components to zero if there is
    // insufficient movement information for the pointer.
    bool getVelocity( float& outVx, float& outVy ) const;

	bool getVelocity( float& speed ) const {
		float x, y;
		if( getVelocity( x, y ) ) {
			speed = hypotf(x, y);
			return true;
		}
		return false;
	}

    // Gets an estimator for the recent movements of the specified pointer id.
    // Returns false and clears the estimator if there is no information available
    // about the pointer.
	bool getEstimator( VelocityTrackerStrategy::Estimator* outEstimator ) const {
		return mStrategy.getEstimator(outEstimator);
	}

	inline void setID( tjs_int id ) { mID = id; }
	inline tjs_int getID() const { return mID; }
	inline void clearID() { mID = -1; }

private:
    class LeastSquaresVelocityTrackerStrategy mStrategy;
	tjs_int mID;
};


// 余り効率のいい実装ではないが、数が少ないので問題ない速度のはず
class VelocityTrackers {
public:
	static const tjs_int MAX_TRACKING = 10;

private:
	VelocityTracker	tracker_[MAX_TRACKING];

private:
	int findEmptyEntry() const {
		for( int i = 0; i < MAX_TRACKING; i++ ) {
			if( tracker_[i].getID() < 0 ) {
				return i;
			}
		}
		return -1;
	}
	int findEntry( tjs_int32 id ) const {
		for( int i = 0; i < MAX_TRACKING; i++ ) {
			if( tracker_[i].getID() == id ) {
				return i;
			}
		}
		return -1;
	}
public:
	void start( tjs_int id ) {
		int index = findEmptyEntry();
		if( index >= 0 ) {
			tracker_[index].clear();
			tracker_[index].setID( id );
		}
	}
	void update( tjs_int id, tjs_uint32 tick, float x, float y ) {
		int index = findEntry( id );
		if( index >= 0 ) {
			tracker_[index].addMovement( tick, x, y );
		}
	}
	void end( tjs_int id ) {
		int index = findEntry( id );
		if( index >= 0 ) {
			tracker_[index].clearID();
		}
	}
	bool getVelocity( tjs_int id, float& x, float& y, float& speed ) const {
		int index = findEntry( id );
		if( index >= 0 ) {
			if( tracker_[index].getVelocity( x, y ) ) {
				speed = hypotf(x, y);
				return true;
			}
		}
		return false;
	}
};

#endif // _LIBINPUT_VELOCITY_TRACKER_H
