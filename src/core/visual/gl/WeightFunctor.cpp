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


#define _USE_MATH_DEFINES
#include "tjsCommHead.h"

#include <float.h>
#include <math.h>
#include <cmath>

#include "WeightFunctor.h"

const float BilinearWeight::RANGE = 1.0f;
const float BicubicWeight::RANGE = 2.0f;
const float Spline16Weight::RANGE = 2.0f;
const float Spline36Weight::RANGE = 3.0f;
const float GaussianWeight::RANGE = 2.0f;
const float BlackmanSincWeight::RANGE = 4.0f;
