
#define DCTSIZE2	    64	/* DCTSIZE squared; # of elements in a block */
#define DCTSIZE		    8	/* The basic DCT block is 8x8 coefficients */
#ifdef __cplusplus
extern "C"
#endif
const int jpeg_natural_order[DCTSIZE2 + 16]; // in libjpeg
#define DEQUANTIZE(coef,quantval)  ((coef) * (quantval))
#define CONST_BITS  13
#define PASS1_BITS  2
#define ONE	((int) 1)
#define MULTIPLY(a,b)  ((a) * (b))
#define RIGHT_SHIFT(a,b) ((a) >> (b))
#define DESCALE(x,n)  RIGHT_SHIFT((x) + (ONE << ((n)-1)), n)
#define FIX_0_298631336  ((int)  2446)	/* FIX(0.298631336) */
#define FIX_0_390180644  ((int)  3196)	/* FIX(0.390180644) */
#define FIX_0_541196100  ((int)  4433)	/* FIX(0.541196100) */
#define FIX_0_765366865  ((int)  6270)	/* FIX(0.765366865) */
#define FIX_0_899976223  ((int)  7373)	/* FIX(0.899976223) */
#define FIX_1_175875602  ((int)  9633)	/* FIX(1.175875602) */
#define FIX_1_501321110  ((int)  12299)	/* FIX(1.501321110) */
#define FIX_1_847759065  ((int)  15137)	/* FIX(1.847759065) */
#define FIX_1_961570560  ((int)  16069)	/* FIX(1.961570560) */
#define FIX_2_053119869  ((int)  16819)	/* FIX(2.053119869) */
#define FIX_2_562915447  ((int)  20995)	/* FIX(2.562915447) */
#define FIX_3_072711026  ((int)  25172)	/* FIX(3.072711026) */

static void AlphaMovie_mjpeg_IDCT_neon(int16_t *quantptr, int16_t *inptr, uint8_t *outptr, int outpitch) {
	int workspace[DCTSIZE2];	/* buffers data between passes */
	int * wsptr = workspace;
	int tmp0, tmp1, tmp2, tmp3;
	int tmp10, tmp11, tmp12, tmp13;
	int z1, z2, z3, ctr;
	for (ctr = DCTSIZE; ctr > 0; ctr--) {
		if (inptr[DCTSIZE * 1] == 0 && inptr[DCTSIZE * 2] == 0 &&
			inptr[DCTSIZE * 3] == 0 && inptr[DCTSIZE * 4] == 0 &&
			inptr[DCTSIZE * 5] == 0 && inptr[DCTSIZE * 6] == 0 &&
			inptr[DCTSIZE * 7] == 0) {
			/* AC terms all zero */
			int dcval = DEQUANTIZE(inptr[DCTSIZE * 0], quantptr[DCTSIZE * 0]) << PASS1_BITS;

			wsptr[DCTSIZE * 0] = dcval;
			wsptr[DCTSIZE * 1] = dcval;
			wsptr[DCTSIZE * 2] = dcval;
			wsptr[DCTSIZE * 3] = dcval;
			wsptr[DCTSIZE * 4] = dcval;
			wsptr[DCTSIZE * 5] = dcval;
			wsptr[DCTSIZE * 6] = dcval;
			wsptr[DCTSIZE * 7] = dcval;

			inptr++;			/* advance pointers to next column */
			quantptr++;
			wsptr++;
			continue;
		}

		z2 = DEQUANTIZE(inptr[DCTSIZE * 2], quantptr[DCTSIZE * 2]);
		z3 = DEQUANTIZE(inptr[DCTSIZE * 6], quantptr[DCTSIZE * 6]);

		z1 = MULTIPLY(z2 + z3, FIX_0_541196100);       /* c6 */
		tmp2 = z1 + MULTIPLY(z2, FIX_0_765366865);     /* c2-c6 */
		tmp3 = z1 - MULTIPLY(z3, FIX_1_847759065);     /* c2+c6 */

		z2 = DEQUANTIZE(inptr[DCTSIZE * 0], quantptr[DCTSIZE * 0]);
		z3 = DEQUANTIZE(inptr[DCTSIZE * 4], quantptr[DCTSIZE * 4]);
		z2 <<= CONST_BITS;
		z3 <<= CONST_BITS;
		/* Add fudge factor here for final descale. */
		z2 += ONE << (CONST_BITS - PASS1_BITS - 1);

		tmp0 = z2 + z3;
		tmp1 = z2 - z3;

		tmp10 = tmp0 + tmp2;
		tmp13 = tmp0 - tmp2;
		tmp11 = tmp1 + tmp3;
		tmp12 = tmp1 - tmp3;

		/* Odd part per figure 8; the matrix is unitary and hence its
		* transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
		*/

		tmp0 = DEQUANTIZE(inptr[DCTSIZE * 7], quantptr[DCTSIZE * 7]);
		tmp1 = DEQUANTIZE(inptr[DCTSIZE * 5], quantptr[DCTSIZE * 5]);
		tmp2 = DEQUANTIZE(inptr[DCTSIZE * 3], quantptr[DCTSIZE * 3]);
		tmp3 = DEQUANTIZE(inptr[DCTSIZE * 1], quantptr[DCTSIZE * 1]);

		z2 = tmp0 + tmp2;
		z3 = tmp1 + tmp3;

		z1 = MULTIPLY(z2 + z3, FIX_1_175875602);       /*  c3 */
		z2 = MULTIPLY(z2, -FIX_1_961570560);          /* -c3-c5 */
		z3 = MULTIPLY(z3, -FIX_0_390180644);          /* -c3+c5 */
		z2 += z1;
		z3 += z1;

		z1 = MULTIPLY(tmp0 + tmp3, -FIX_0_899976223); /* -c3+c7 */
		tmp0 = MULTIPLY(tmp0, FIX_0_298631336);        /* -c1+c3+c5-c7 */
		tmp3 = MULTIPLY(tmp3, FIX_1_501321110);        /*  c1+c3-c5-c7 */
		tmp0 += z1 + z2;
		tmp3 += z1 + z3;

		z1 = MULTIPLY(tmp1 + tmp2, -FIX_2_562915447); /* -c1-c3 */
		tmp1 = MULTIPLY(tmp1, FIX_2_053119869);        /*  c1+c3-c5+c7 */
		tmp2 = MULTIPLY(tmp2, FIX_3_072711026);        /*  c1+c3+c5-c7 */
		tmp1 += z1 + z3;
		tmp2 += z1 + z2;

		/* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

		wsptr[DCTSIZE * 0] = (int)RIGHT_SHIFT(tmp10 + tmp3, CONST_BITS - PASS1_BITS);
		wsptr[DCTSIZE * 7] = (int)RIGHT_SHIFT(tmp10 - tmp3, CONST_BITS - PASS1_BITS);
		wsptr[DCTSIZE * 1] = (int)RIGHT_SHIFT(tmp11 + tmp2, CONST_BITS - PASS1_BITS);
		wsptr[DCTSIZE * 6] = (int)RIGHT_SHIFT(tmp11 - tmp2, CONST_BITS - PASS1_BITS);
		wsptr[DCTSIZE * 2] = (int)RIGHT_SHIFT(tmp12 + tmp1, CONST_BITS - PASS1_BITS);
		wsptr[DCTSIZE * 5] = (int)RIGHT_SHIFT(tmp12 - tmp1, CONST_BITS - PASS1_BITS);
		wsptr[DCTSIZE * 3] = (int)RIGHT_SHIFT(tmp13 + tmp0, CONST_BITS - PASS1_BITS);
		wsptr[DCTSIZE * 4] = (int)RIGHT_SHIFT(tmp13 - tmp0, CONST_BITS - PASS1_BITS);

		inptr++;			/* advance pointers to next column */
		quantptr++;
		wsptr++;
	}
	/* Pass 2: process rows from work array, store into output array.
	* Note that we must descale the results by a factor of 8 == 2**3,
	* and also undo the PASS1_BITS scaling.
	*/
	wsptr = workspace;
	for (ctr = 0; ctr < DCTSIZE; ctr++, outptr += outpitch) {
		if (wsptr[1] == 0 && wsptr[2] == 0 && wsptr[3] == 0 && wsptr[4] == 0 &&
			wsptr[5] == 0 && wsptr[6] == 0 && wsptr[7] == 0) {
			/* AC terms all zero */
#define RANGE_MASK  (255 * 4 + 3) /* 2 bits wider than legal samples */
			tjs_uint8 dcval = range_limit(DESCALE((int)wsptr[0], PASS1_BITS + 3) + 128);
			outptr[0] = dcval;
			outptr[1] = dcval;
			outptr[2] = dcval;
			outptr[3] = dcval;
			outptr[4] = dcval;
			outptr[5] = dcval;
			outptr[6] = dcval;
			outptr[7] = dcval;

			wsptr += DCTSIZE;		/* advance pointer to next row */
			continue;
		}

		/* Even part: reverse the even part of the forward DCT.
		* The rotator is c(-6).
		*/

		z2 = (int32_t)wsptr[2];
		z3 = (int32_t)wsptr[6];

		z1 = MULTIPLY(z2 + z3, FIX_0_541196100);       /* c6 */
		tmp2 = z1 + MULTIPLY(z2, FIX_0_765366865);     /* c2-c6 */
		tmp3 = z1 - MULTIPLY(z3, FIX_1_847759065);     /* c2+c6 */

		/* Add fudge factor here for final descale. */
		z2 = (int32_t)wsptr[0] + (ONE << (PASS1_BITS + 2));
		z3 = (int32_t)wsptr[4];

		tmp0 = (z2 + z3) << CONST_BITS;
		tmp1 = (z2 - z3) << CONST_BITS;

		tmp10 = tmp0 + tmp2;
		tmp13 = tmp0 - tmp2;
		tmp11 = tmp1 + tmp3;
		tmp12 = tmp1 - tmp3;

		/* Odd part per figure 8; the matrix is unitary and hence its
		* transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
		*/

		tmp0 = (int32_t)wsptr[7];
		tmp1 = (int32_t)wsptr[5];
		tmp2 = (int32_t)wsptr[3];
		tmp3 = (int32_t)wsptr[1];

		z2 = tmp0 + tmp2;
		z3 = tmp1 + tmp3;

		z1 = MULTIPLY(z2 + z3, FIX_1_175875602);       /*  c3 */
		z2 = MULTIPLY(z2, -FIX_1_961570560);          /* -c3-c5 */
		z3 = MULTIPLY(z3, -FIX_0_390180644);          /* -c3+c5 */
		z2 += z1;
		z3 += z1;

		z1 = MULTIPLY(tmp0 + tmp3, -FIX_0_899976223); /* -c3+c7 */
		tmp0 = MULTIPLY(tmp0, FIX_0_298631336);        /* -c1+c3+c5-c7 */
		tmp3 = MULTIPLY(tmp3, FIX_1_501321110);        /*  c1+c3-c5-c7 */
		tmp0 += z1 + z2;
		tmp3 += z1 + z3;

		z1 = MULTIPLY(tmp1 + tmp2, -FIX_2_562915447); /* -c1-c3 */
		tmp1 = MULTIPLY(tmp1, FIX_2_053119869);        /*  c1+c3-c5+c7 */
		tmp2 = MULTIPLY(tmp2, FIX_3_072711026);        /*  c1+c3+c5-c7 */
		tmp1 += z1 + z3;
		tmp2 += z1 + z2;

		/* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

		outptr[0] = range_limit((int)RIGHT_SHIFT(tmp10 + tmp3,
			CONST_BITS + PASS1_BITS + 3)
			+ 128);
		outptr[7] = range_limit((int)RIGHT_SHIFT(tmp10 - tmp3,
			CONST_BITS + PASS1_BITS + 3)
			+ 128);
		outptr[1] = range_limit((int)RIGHT_SHIFT(tmp11 + tmp2,
			CONST_BITS + PASS1_BITS + 3)
			+ 128);
		outptr[6] = range_limit((int)RIGHT_SHIFT(tmp11 - tmp2,
			CONST_BITS + PASS1_BITS + 3)
			+ 128);
		outptr[2] = range_limit((int)RIGHT_SHIFT(tmp12 + tmp1,
			CONST_BITS + PASS1_BITS + 3)
			+ 128);
		outptr[5] = range_limit((int)RIGHT_SHIFT(tmp12 - tmp1,
			CONST_BITS + PASS1_BITS + 3)
			+ 128);
		outptr[3] = range_limit((int)RIGHT_SHIFT(tmp13 + tmp0,
			CONST_BITS + PASS1_BITS + 3)
			+ 128);
		outptr[4] = range_limit((int)RIGHT_SHIFT(tmp13 - tmp0,
			CONST_BITS + PASS1_BITS + 3)
			+ 128);

		wsptr += DCTSIZE;		/* advance pointer to next row */
	}
}
