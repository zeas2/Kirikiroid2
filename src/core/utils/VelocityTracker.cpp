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
 */

#include "tjsCommHead.h"

#include <math.h>
#include <limits.h>
#include "VelocityTracker.h"


// Nanoseconds per milliseconds.
//static const nsecs_t NANOS_PER_MS = 1000000;
// tick 値で計算するので、元の分解能はミリ秒
static const tjs_uint64 NANOS_PER_MS = 1;

// Threshold for determining that a pointer has stopped moving.
// Some input devices do not send ACTION_MOVE events in the case where a pointer has
// stopped.  We need to detect this case so that we can accurately predict the
// velocity after the pointer starts moving again.
static const tjs_uint64 ASSUME_POINTER_STOPPED_TIME = 40 * NANOS_PER_MS;


static float vectorDot(const float* a, const float* b, tjs_uint32 m) {
    float r = 0;
    while (m--) {
        r += *(a++) * *(b++);
    }
    return r;
}

static float vectorNorm(const float* a, tjs_uint32 m) {
    float r = 0;
    while (m--) {
        float t = *(a++);
        r += t * t;
    }
    return sqrtf(r);
}


// --- VelocityTracker ---
VelocityTracker::VelocityTracker() : mStrategy(2), mID(-1) { }
VelocityTracker::~VelocityTracker() {}
bool VelocityTracker::getVelocity( float& outVx, float& outVy ) const {
    VelocityTrackerStrategy::Estimator estimator;
    if (getEstimator(&estimator) && estimator.degree >= 1) {
        outVx = estimator.xCoeff[1];
        outVy = estimator.yCoeff[1];
        return true;
    }
    outVx = 0;
    outVy = 0;
    return false;
}

// --- LeastSquaresVelocityTrackerStrategy ---

//const tjs_uint64 LeastSquaresVelocityTrackerStrategy::HORIZON;
//const tjs_uint32 LeastSquaresVelocityTrackerStrategy::HISTORY_SIZE;

LeastSquaresVelocityTrackerStrategy::LeastSquaresVelocityTrackerStrategy(tjs_uint32 degree, Weighting weighting)
 : mDegree(degree), mWeighting(weighting) {
    clear();
}

LeastSquaresVelocityTrackerStrategy::~LeastSquaresVelocityTrackerStrategy() {
}

void LeastSquaresVelocityTrackerStrategy::clear() {
    mIndex = 0;
	mMovements[0].usingThis = false;
}

void LeastSquaresVelocityTrackerStrategy::addMovement( tjs_uint64 eventTime, float x, float y ) {
    if (++mIndex == HISTORY_SIZE) {
        mIndex = 0;
    }

    Movement& movement = mMovements[mIndex];
    movement.eventTime = eventTime;
    movement.positions.x = x;
	movement.positions.y = y;
	movement.usingThis = true;
}

/**
 * Solves a linear least squares problem to obtain a N degree polynomial that fits
 * the specified input data as nearly as possible.
 *
 * Returns true if a solution is found, false otherwise.
 *
 * The input consists of two vectors of data points X and Y with indices 0..m-1
 * along with a weight vector W of the same size.
 *
 * The output is a vector B with indices 0..n that describes a polynomial
 * that fits the data, such the sum of W[i] * W[i] * abs(Y[i] - (B[0] + B[1] X[i]
 * + B[2] X[i]^2 ... B[n] X[i]^n)) for all i between 0 and m-1 is minimized.
 *
 * Accordingly, the weight vector W should be initialized by the caller with the
 * reciprocal square root of the variance of the error in each input data point.
 * In other words, an ideal choice for W would be W[i] = 1 / var(Y[i]) = 1 / stddev(Y[i]).
 * The weights express the relative importance of each data point.  If the weights are
 * all 1, then the data points are considered to be of equal importance when fitting
 * the polynomial.  It is a good idea to choose weights that diminish the importance
 * of data points that may have higher than usual error margins.
 *
 * Errors among data points are assumed to be independent.  W is represented here
 * as a vector although in the literature it is typically taken to be a diagonal matrix.
 *
 * That is to say, the function that generated the input data can be approximated
 * by y(x) ~= B[0] + B[1] x + B[2] x^2 + ... + B[n] x^n.
 *
 * The coefficient of determination (R^2) is also returned to describe the goodness
 * of fit of the model for the given data.  It is a value between 0 and 1, where 1
 * indicates perfect correspondence.
 *
 * This function first expands the X vector to a m by n matrix A such that
 * A[i][0] = 1, A[i][1] = X[i], A[i][2] = X[i]^2, ..., A[i][n] = X[i]^n, then
 * multiplies it by w[i]./
 *
 * Then it calculates the QR decomposition of A yielding an m by m orthonormal matrix Q
 * and an m by n upper triangular matrix R.  Because R is upper triangular (lower
 * part is all zeroes), we can simplify the decomposition into an m by n matrix
 * Q1 and a n by n matrix R1 such that A = Q1 R1.
 *
 * Finally we solve the system of linear equations given by R1 B = (Qtranspose W Y)
 * to find B.
 *
 * For efficiency, we lay out A and Q column-wise in memory because we frequently
 * operate on the column vectors.  Conversely, we lay out R row-wise.
 *
 * http://en.wikipedia.org/wiki/Numerical_methods_for_linear_least_squares
 * http://en.wikipedia.org/wiki/Gram-Schmidt
 */
static bool solveLeastSquares(const float* x, const float* y,
        const float* w, tjs_uint32 m, tjs_uint32 n, float* outB, float* outDet) {

    // Expand the X vector to a matrix A, pre-multiplied by the weights.
    //float a[n][m]; // column-major order
	std::vector<std::vector<float> >	a(n);
	for( tjs_uint32 i = 0; i < n; i++ ) a[i].resize(m);
    for (tjs_uint32 h = 0; h < m; h++) {
        a[0][h] = w[h];
        for (tjs_uint32 i = 1; i < n; i++) {
            a[i][h] = a[i - 1][h] * x[h];
        }
    }

    // Apply the Gram-Schmidt process to A to obtain its QR decomposition.
    //float q[n][m]; // orthonormal basis, column-major order
	std::vector<std::vector<float> >	q(n);
	for( tjs_uint32 i = 0; i < n; i++ ) q[i].resize(m);
    //float r[n][n]; // upper triangular matrix, row-major order
	std::vector<std::vector<float> >	r(n);
	for( tjs_uint32 i = 0; i < n; i++ ) r[i].resize(n);
    for (tjs_uint32 j = 0; j < n; j++) {
        for (tjs_uint32 h = 0; h < m; h++) {
            q[j][h] = a[j][h];
        }
        for (tjs_uint32 i = 0; i < j; i++) {
            float dot = vectorDot(&q[j][0], &q[i][0], m);
            for (tjs_uint32 h = 0; h < m; h++) {
                q[j][h] -= dot * q[i][h];
            }
        }

        float norm = vectorNorm(&q[j][0], m);
        if (norm < 0.000001f) {
            // vectors are linearly dependent or zero so no solution
            return false;
        }

        float invNorm = 1.0f / norm;
        for (tjs_uint32 h = 0; h < m; h++) {
            q[j][h] *= invNorm;
        }
        for (tjs_uint32 i = 0; i < n; i++) {
            r[j][i] = i < j ? 0 : vectorDot(&q[j][0], &a[i][0], m);
        }
    }
    // Solve R B = Qt W Y to find B.  This is easy because R is upper triangular.
    // We just work from bottom-right to top-left calculating B's coefficients.
    //float wy[m];
	std::vector<float> wy(m);
    for (tjs_uint32 h = 0; h < m; h++) {
        wy[h] = y[h] * w[h];
    }
    for (tjs_uint32 i = n; i-- != 0; ) {
        outB[i] = vectorDot(&q[i][0], &(wy[0]), m);
        for (tjs_uint32 j = n - 1; j > i; j--) {
            outB[i] -= r[i][j] * outB[j];
        }
        outB[i] /= r[i][i];
    }

    // Calculate the coefficient of determination as 1 - (SSerr / SStot) where
    // SSerr is the residual sum of squares (variance of the error),
    // and SStot is the total sum of squares (variance of the data) where each
    // has been weighted.
    float ymean = 0;
    for (tjs_uint32 h = 0; h < m; h++) {
        ymean += y[h];
    }
    ymean /= m;

    float sserr = 0;
    float sstot = 0;
    for (tjs_uint32 h = 0; h < m; h++) {
        float err = y[h] - outB[0];
        float term = 1;
        for (tjs_uint32 i = 1; i < n; i++) {
            term *= x[h];
            err -= term * outB[i];
        }
        sserr += w[h] * w[h] * err * err;
        float var = y[h] - ymean;
        sstot += w[h] * w[h] * var * var;
    }
    *outDet = sstot > 0.000001f ? 1.0f - (sserr / sstot) : 1;
    return true;
}

bool LeastSquaresVelocityTrackerStrategy::getEstimator(VelocityTrackerStrategy::Estimator* outEstimator) const {
    outEstimator->clear();

    // Iterate over movement samples in reverse time order and collect samples.
    float x[HISTORY_SIZE];
    float y[HISTORY_SIZE];
    float w[HISTORY_SIZE];
    float time[HISTORY_SIZE];
    tjs_uint32 m = 0;
    tjs_uint32 index = mIndex;
    const Movement& newestMovement = mMovements[mIndex];
    do {
        const Movement& movement = mMovements[index];
        if( !movement.usingThis ) {
            break;
        }
        tjs_uint64 age = newestMovement.eventTime - movement.eventTime;
        if (age > HORIZON) {
            break;
        }

        const Position& position = movement.getPosition();
        x[m] = position.x;
        y[m] = position.y;
        w[m] = chooseWeight(index);
        //time[m] = -age * 0.000000001f;
    	time[m] = -(tjs_int64)age * 0.001f;
        index = (index == 0 ? HISTORY_SIZE : index) - 1;
    } while (++m < HISTORY_SIZE);

    if (m == 0) {
        return false; // no data
    }

    // Calculate a least squares polynomial fit.
    tjs_uint32 degree = mDegree;
    if (degree > m - 1) {
        degree = m - 1;
    }
    if (degree >= 1) {
        float xdet, ydet;
        tjs_uint32 n = degree + 1;
        if (solveLeastSquares(time, x, w, m, n, outEstimator->xCoeff, &xdet)
                && solveLeastSquares(time, y, w, m, n, outEstimator->yCoeff, &ydet)) {
            outEstimator->time = newestMovement.eventTime;
            outEstimator->degree = degree;
            outEstimator->confidence = xdet * ydet;
            return true;
        }
    }

    // No velocity data available for this pointer, but we do have its current position.
    outEstimator->xCoeff[0] = x[0];
    outEstimator->yCoeff[0] = y[0];
    outEstimator->time = newestMovement.eventTime;
    outEstimator->degree = 0;
    outEstimator->confidence = 1;
    return true;
}

float LeastSquaresVelocityTrackerStrategy::chooseWeight(tjs_uint32 index) const {
    switch (mWeighting) {
    case WEIGHTING_DELTA: {
        // Weight points based on how much time elapsed between them and the next
        // point so that points that "cover" a shorter time span are weighed less.
        //   delta  0ms: 0.5
        //   delta 10ms: 1.0
        if (index == mIndex) {
            return 1.0f;
        }
        tjs_uint32 nextIndex = (index + 1) % HISTORY_SIZE;
        //float deltaMillis = (mMovements[nextIndex].eventTime- mMovements[index].eventTime) * 0.000001f;
    	float deltaMillis = (float)(mMovements[nextIndex].eventTime- mMovements[index].eventTime);
        if (deltaMillis < 0) {
            return 0.5f;
        }
        if (deltaMillis < 10) {
            return 0.5f + deltaMillis * 0.05f;
        }
        return 1.0f;
    }

    case WEIGHTING_CENTRAL: {
        // Weight points based on their age, weighing very recent and very old points less.
        //   age  0ms: 0.5
        //   age 10ms: 1.0
        //   age 50ms: 1.0
        //   age 60ms: 0.5
        //float ageMillis = (mMovements[mIndex].eventTime - mMovements[index].eventTime) * 0.000001f;
    	float ageMillis = (float)(mMovements[mIndex].eventTime - mMovements[index].eventTime);
        if (ageMillis < 0) {
            return 0.5f;
        }
        if (ageMillis < 10) {
            return 0.5f + ageMillis * 0.05f;
        }
        if (ageMillis < 50) {
            return 1.0f;
        }
        if (ageMillis < 60) {
            return 0.5f + (60 - ageMillis) * 0.05f;
        }
        return 0.5f;
    }

    case WEIGHTING_RECENT: {
        // Weight points based on their age, weighing older points less.
        //   age   0ms: 1.0
        //   age  50ms: 1.0
        //   age 100ms: 0.5
        //float ageMillis = (mMovements[mIndex].eventTime - mMovements[index].eventTime) * 0.000001f;
    	float ageMillis = (float)(mMovements[mIndex].eventTime - mMovements[index].eventTime);
        if (ageMillis < 50) {
            return 1.0f;
        }
        if (ageMillis < 100) {
            return 0.5f + (100 - ageMillis) * 0.01f;
        }
        return 0.5f;
    }

    case WEIGHTING_NONE:
    default:
        return 1.0f;
    }
}

