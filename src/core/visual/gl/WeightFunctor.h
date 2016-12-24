/******************************************************************************/
/**
 * 各種フィルターのウェイトを計算する
 * ----------------------------------------------------------------------------
 * 	Copyright (C) T.Imoto <http://www.kaede-software.com>
 * ----------------------------------------------------------------------------
 * @author		T.Imoto
 * @date		2014/04/02
 * @note
 *****************************************************************************/


#ifndef __WEIGHT_FUNCTOR_H__
#define __WEIGHT_FUNCTOR_H__

/**
 * バイリニア
 */
struct BilinearWeight {
	static const float RANGE;
	inline float operator()( float distance ) const {
		float x = std::abs(distance);
		if( x < 1.0f ) {
			return 1.0f - x;
		} else {
			return 0.0f;
		}
	}
};

/**
 * バイキュービック
 */
struct BicubicWeight {
	static const float RANGE;

	float coeff;
	float p[5];
	/**
	 * @param c : シャープさ。小さくなるにしたがって強くなる
	 */
	BicubicWeight( float c = -1 ) : coeff(c) {
		p[0] = coeff + 3.0f;
		p[1] = coeff + 2.0f;
		p[2] = -coeff*4.0f;
		p[3] = coeff*8.0f;
		p[4] = coeff*5.0f;
	}
	inline float operator()( float distance ) const {
		float x = std::abs(distance);
		if( x <= 1.0f ) {
			return 1.0f - p[0]*x*x + p[1]*x*x*x;
		} else if( x <= 2.0f ) {
			return p[2] + p[3]*x - p[4]*x*x + coeff*x*x*x;
		} else {
			return 0.0f;
		}
	}
};

/**
 * Lanczos
 */
template<int TTap>
struct LanczosWeight {
	static const float RANGE;
	inline float operator()( float distance ) {
		float x = std::abs(distance);
		if( x < FLT_EPSILON ) return 1.0f;
		if( x >= (float)TTap ) return 0.0f;
		return std::sin((float)M_PI*distance)*std::sin((float)M_PI*distance/TTap)/((float)M_PI*(float)M_PI*distance*distance/TTap);
	}
};
template<int TTap>
const float LanczosWeight<TTap>::RANGE = (float)TTap;

/**
 * Spline16 用ウェイト関数
 */
struct Spline16Weight {
	static const float RANGE;
	inline float operator()( float distance ) const {
		float x = std::abs(distance);
		if( x <= 1.0f ) {
			return  x*x*x           - x*x*9.0f/5.0f - x* 1.0f/ 5.0f + 1.0f;
		} else if( x <= 2.0f ) {
			return -x*x*x*1.0f/3.0f + x*x*9.0f/5.0f - x*46.0f/15.0f + 8.0f/5.0f;
		} else {
			return 0.0f;
		}
	}
};

/**
 * Spline36 用ウェイト関数
 */
struct Spline36Weight {
	static const float RANGE;
	inline float operator()( float distance ) const {
		float x = std::abs(distance);
		if( x <= 1.0f ) {
			return  x*x*x*13.0f/11.0f - x*x*453.0f/209.0f - x*   3.0f/209.0f + 1.0f;
		} else if( x <= 2.0f ) {
			return -x*x*x* 6.0f/11.0f + x*x*612.0f/209.0f - x*1038.0f/209.0f + 540.0f/209.0f;
		} else if( x <= 3.0f ) {
			return  x*x*x* 1.0f/11.0f - x*x*159.0f/209.0f + x* 434.0f/209.0f - 384.0f/209.0f;
		} else {
			return 0.0f;
		}
	}
};
/**
 * ガウス関数
 */
struct GaussianWeight {
	static const float RANGE;
	float sq2pi;
	GaussianWeight() : sq2pi( std::sqrt( 2.0f/(float)M_PI ) ) {
	}
	inline float operator()( float distance ) const {
		float x = distance;
		return std::exp( (float)(-2.0f*x*x) ) * sq2pi;
	}
};

/**
 * Blackman-Sinc 関数
 * Blackman(x) = 0.42 - 0.5 cos( 2 pi x ) + 0.08 cos( 4 pi x ); if 0 <= x <= 1
 * Sinc(x) = sin( pi x ) / (pi x)
 */
struct BlackmanSincWeight {
	static const float RANGE;
	float coeff;
	BlackmanSincWeight( float c = RANGE ) : coeff(1.0f/c) {}
	inline float operator()( float distance ) const {
		float x = std::abs(distance);
		if( x < FLT_EPSILON ) {
			return 1.0f;
		} else {
			return (0.42f + 0.5f*std::cos( (float)M_PI*x*coeff ) + 0.08f*std::cos( 2.0f*(float)M_PI*x*coeff )) *
				(std::sin( (float)M_PI*x ) / ((float)M_PI*x));
		}
	}
};

#endif // __WEIGHT_FUNCTOR_H__
