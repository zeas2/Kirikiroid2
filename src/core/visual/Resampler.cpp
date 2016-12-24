//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Image resampler
//---------------------------------------------------------------------------
/*
	based on Graphics Gems III
	"Filtered Image Rescaling" by Dale Schumacher
*/
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <math.h>
#include "LayerBitmapIntf.h"
#include "MsgIntf.h"
#include "RenderManager.h"

//---------------------------------------------------------------------------
typedef float real_t;
//---------------------------------------------------------------------------
#pragma pack(push,1)
typedef struct
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} pixel_t;
#pragma pack(pop)
typedef struct
{
	real_t r;
	real_t g;
	real_t b;
	real_t a;
} pixel_real_t_t;
//---------------------------------------------------------------------------
static inline int CLAMP(int v, int l, int h)
	{ return ((v)<(l) ? (l) : (v) > (h) ? (h) : (v)); }
//---------------------------------------------------------------------------
typedef struct {
        int     pixel;
        real_t  weight;
} CONTRIB;

typedef struct {
        int     n;              /* number of contributors */
        CONTRIB *p;             /* pointer to list of contributions */
} CLIST;
//---------------------------------------------------------------------------
static int roundcloser(real_t d)
{
	/* return fabs(ceil(d)-d) <= 0.5 ? ceil(d) : floor(d); */

	int n = (int) d;
	real_t diff = d - (real_t)n;
	if(diff < 0)
		diff = -diff;
	if(diff >= 0.5)
	{
		if(d < 0)
				n--;
		else
				n++;
	}
	return n;
} /* roundcloser */
//---------------------------------------------------------------------------
static int calc_x_contrib(CLIST *contribX, real_t xscale, real_t fwidth, int dstwidth,
	int srcwidth, real_t (*filterf)(real_t), int i)
{
	real_t width;
	real_t fscale;
	real_t center, left, right;
	real_t weight;
	int j, k, n;

	if(xscale < 1.0)
	{
		/* Shrinking image */
		real_t w_sum = 0;
		width = fwidth / xscale;
//		fscale = 1.0 / xscale;

		contribX->n = 0;
		contribX->p = (CONTRIB *)calloc((int) (width * 2 + 2),
						sizeof(CONTRIB));
		if(contribX->p == NULL)
				return -1;

		center = (real_t) i / xscale;
		left = ceil(center - width);
		right = floor(center + width);
		for(j = (int)left; j <= right; ++j)
		{
			weight = center - (real_t) j;
			weight = (*filterf)(weight * xscale); // / fscale;
			w_sum += weight;
			if(j < 0)
					n = -j;
			else if(j >= srcwidth)
					n = (srcwidth - j) + srcwidth - 1;
			else
					n = j;

			if(n < 0) n = 0;
			if(n >= srcwidth - 1) n = srcwidth - 1;

			k = contribX->n++;
			contribX->p[k].pixel = n;
			contribX->p[k].weight = weight;
		}

		if(w_sum != 0.0)
		{
			w_sum = 1.0 / w_sum;
			for(j = 0; j < contribX->n; j ++)
				contribX->p[j].weight *= w_sum;
		}

	}
	else
	{
		/* Expanding image */
		contribX->n = 0;
		contribX->p = (CONTRIB *)calloc((int) (fwidth * 2 + 1),
						sizeof(CONTRIB));
		if(contribX->p == NULL)
				return -1;
		center = (real_t) i / xscale;
		left = ceil(center - fwidth);
		right = floor(center + fwidth);

		for(j = (int)left; j <= right; ++j)
		{
			weight = center - (real_t) j;
			weight = (*filterf)(weight);
			if(j < 0) {
					n = -j;
			} else if(j >= srcwidth) {
					n = (srcwidth - j) + srcwidth - 1;
			} else {
					n = j;
			}

			if(n < 0) n = 0;
			if(n >= srcwidth - 1) n = srcwidth - 1;

			k = contribX->n++;
			contribX->p[k].pixel = n;
			contribX->p[k].weight = weight;
		}
	}
	return 0;
} /* calc_x_contrib */
//---------------------------------------------------------------------------
static int ib_resample(iTVPBaseBitmap *dst,
	const tTVPRect &destrect,
	const iTVPBaseBitmap *src,
	const tTVPRect &srcrect,
	real_t (*filterf)(real_t),
//	void (*blend)(tjs_uint32 *dest, tjs_uint32 src),
	real_t fwidth)
{
	pixel_t * tmp;
	real_t xscale, yscale;          /* zoom scale factors */
	int xx;
	int i, j, k;                    /* loop variables */
	int n;                          /* pixel number */
	real_t center, left, right;     /* filter calculation variables */
	real_t width, fscale;   /* filter calculation variables */
	pixel_real_t_t weight;
	pixel_t pel, pel2;
	pixel_t bPelDelta;
	CLIST   *contribY;              /* array of contribution lists */
	CLIST   contribX;
	int             nRet = -1;
	int srcwidth = srcrect.get_width();
	int srcheight = srcrect.get_height();
	int destwidth = destrect.get_width();
	int destheight = destrect.get_height();

	/* create intermediate column to hold horizontal dst column zoom */
	tmp = (pixel_t*)malloc(srcheight * sizeof(pixel_t));
	if(tmp == NULL)
		return 0;

	xscale = (real_t) destwidth / (real_t) srcwidth;

	/* Build y weights */
	/* pre-calculate filter contributions for a column */
	contribY = (CLIST *)calloc(destheight, sizeof(CLIST));
	if(contribY == NULL)
	{
		free(tmp);
		return -1;
	}

	yscale = (real_t) destheight / (real_t) srcheight;

	if(yscale < 1.0)
	{
		real_t weight;
		width = fwidth / yscale;
		fscale = 1.0 / yscale;
		for(i = 0; i < destheight; ++i)
		{
			real_t w_sum = 0;

			contribY[i].n = 0;
			contribY[i].p = (CONTRIB *)calloc((int) (width * 2 + 1),
							sizeof(CONTRIB));
			if(contribY[i].p == NULL)
			{
				free(tmp);
				free(contribY);
				return -1;
			}
			center = (real_t) i * fscale;
			left = ceil(center - width);
			right = floor(center + width);
			for(j = (int)left; j <= right; ++j)
			{
				weight = center - (real_t) j;
				weight = (*filterf)(weight * yscale); //* yscale;
				w_sum += weight;
				if(j < 0)
				{
					n = -j;
				}
				else if(j >= srcheight)
				{
					n = (srcheight - j) + srcheight - 1;
				}
				else
				{
					n = j;
				}
				if(n < 0) n = 0;
				if(n >= srcheight - 1) n = srcheight - 1;
				k = contribY[i].n++;
				contribY[i].p[k].pixel = n;
				contribY[i].p[k].weight = weight;
			}

			if(w_sum != 0.0)
			{
				w_sum = 1.0 / w_sum;
				for(j = 0; j < contribY[i].n; j ++)
					contribY[i].p[j].weight *= w_sum;
			}

		}
	}
	else
	{
		real_t weight;
		for(i = 0; i < destheight; ++i)
		{
			contribY[i].n = 0;
			contribY[i].p = (CONTRIB *)calloc((int) (fwidth * 2 + 1),
							sizeof(CONTRIB));
			if(contribY[i].p == NULL)
			{
				free(tmp);
				free(contribY);
				return -1;
			}
			center = (real_t) i / yscale;
			left = ceil(center - fwidth);
			right = floor(center + fwidth);
			for(j = (int)left; j <= right; ++j)
			{
				weight = center - (real_t) j;
				weight = (*filterf)(weight);
				if(j < 0)
				{
					n = -j;
				}
				else if(j >= srcheight)
				{
					n = (srcheight - j) + srcheight - 1;
				}
				else
				{
					n = j;
				}
				if(n < 0) n = 0;
				if(n >= srcheight - 1) n = srcheight - 1;
				k = contribY[i].n++;
				contribY[i].p[k].pixel = n;
				contribY[i].p[k].weight = weight;
			}
		}
	}


	tjs_int srcpitchbytes = src->GetPitchBytes();
	const pixel_t *srclinestart = (const pixel_t*)
		((const tjs_uint8*)src->GetTexture()->GetPixelData() + srcrect.top * srcpitchbytes) + srcrect.left;

	pixel_t *destlinestart = (pixel_t*)dst->GetScanLineForWrite(destrect.top) + destrect.left;
	tjs_int destpitchbytes = dst->GetPitchBytes();

	for(xx = 0; xx < destwidth; xx++)
	{
		if(0 != calc_x_contrib(&contribX, xscale, fwidth,
			destwidth, srcwidth, filterf, xx))
		{
			goto __zoom_cleanup;
		}
		/* Apply horz filter to make dst column in tmp. */
		{
			const pixel_t *line = srclinestart;
			for(k = 0; k < srcheight; ++k)
			{
				weight.r = weight.g = weight.b = weight.a = 0;
				bPelDelta.r = bPelDelta.g = bPelDelta.b = bPelDelta.a = 0;
				pel = line[contribX.p[0].pixel];
				for(j = contribX.n - 1; j >= 0; --j)
				{
					CONTRIB *c = contribX.p + j;
					pel2 = line[c->pixel];
					bPelDelta.r |= (pel2.r - pel.r);
					bPelDelta.b |= (pel2.b - pel.b);
					bPelDelta.g |= (pel2.g - pel.g);
					bPelDelta.a |= (pel2.a - pel.a);
					weight.r += pel2.r * c->weight;
					weight.g += pel2.g * c->weight;
					weight.b += pel2.b * c->weight;
					weight.a += pel2.a * c->weight;
				}
				weight.r = bPelDelta.r ? roundcloser(weight.r) : pel.r;
				weight.g = bPelDelta.g ? roundcloser(weight.g) : pel.g;
				weight.b = bPelDelta.b ? roundcloser(weight.b) : pel.b;
				weight.a = bPelDelta.a ? roundcloser(weight.a) : pel.a;

				tmp[k].r = (unsigned char)CLAMP(weight.r, 0, 255);
				tmp[k].g = (unsigned char)CLAMP(weight.g, 0, 255);
				tmp[k].b = (unsigned char)CLAMP(weight.b, 0, 255);
				tmp[k].a = (unsigned char)CLAMP(weight.a, 0, 255);

				*(tjs_uint8**)(&line) += srcpitchbytes;
			} /* next row in temp column */
		}

		free(contribX.p);

		/* The temp column has been built. Now stretch it
		 vertically into dst column. */
		{
			pixel_t *line = destlinestart;
			for(i = 0; i < destheight; ++i)
			{
				CLIST *cl = contribY + i;
				weight.r = weight.g = weight.b = weight.a = 0;
				bPelDelta.r = bPelDelta.g = bPelDelta.b = bPelDelta.a = 0;
				pel = tmp[cl->p[0].pixel];

				for(j = cl->n - 1; j >= 0; --j)
				{
					CONTRIB *c = cl->p + j;
					pel2 = tmp[c->pixel];
					bPelDelta.r |= (pel2.r - pel.r);
					bPelDelta.b |= (pel2.b - pel.b);
					bPelDelta.g |= (pel2.g - pel.g);
					bPelDelta.a |= (pel2.a - pel.a);
					weight.r += pel2.r * c->weight;
					weight.g += pel2.g * c->weight;
					weight.b += pel2.b * c->weight;
					weight.a += pel2.a * c->weight;
				}
				weight.r = bPelDelta.r ? roundcloser(weight.r) : pel.r;
				weight.g = bPelDelta.g ? roundcloser(weight.g) : pel.g;
				weight.b = bPelDelta.b ? roundcloser(weight.b) : pel.b;
				weight.a = bPelDelta.a ? roundcloser(weight.a) : pel.a;
				line[xx].r = (unsigned char)CLAMP(weight.r, 0, 255);
				line[xx].g = (unsigned char)CLAMP(weight.g, 0, 255);
				line[xx].b = (unsigned char)CLAMP(weight.b, 0, 255);
				line[xx].a = (unsigned char)CLAMP(weight.a, 0, 255);

 				*(tjs_uint8**)(&line) += destpitchbytes;
			} /* next dst row */
		}
	} /* next dst column */
	nRet = 0; /* success */

__zoom_cleanup:
	free(tmp);

	/* free the memory allocated for vertical filter weights */
	for(i = 0; i < destheight; ++i)
			free(contribY[i].p);
	free(contribY);

	return nRet;
} /* ib_resample */
//---------------------------------------------------------------------------
#define	filter_support		(1.0)
real_t
filter(real_t t)
{
	/* f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1 */
	if(t < 0.0) t = -t;
	if(t < 1.0) return((2.0 * t - 3.0) * t * t + 1.0);
	return(0.0);
}

#define	box_support		(0.5)
real_t
box_filter(real_t t)
{
	if((t > -0.5) && (t <= 0.5)) return(1.0);
	return(0.0);
}


#define	triangle_support	(1.0)
static real_t
triangle_filter(real_t t)
{
	if(t < 0.0) t = -t;
	if(t < 1.0) return(1.0 - t);
	return(0.0);
}

#define	bell_support		(1.5)
static real_t
bell_filter(real_t t)		/* box (*) box (*) box */
{
	if(t < 0) t = -t;
	if(t < 0.5) return(0.75 - (t * t));
	if(t < 1.5) {
		t = (t - 1.5);
		return(0.5 * (t * t));
	}
	return(0.0);
}


//---------------------------------------------------------------------------
void TVPResampleImage(const tTVPRect &cliprect, iTVPBaseBitmap *dest, const tTVPRect &destrect, const iTVPBaseBitmap *src, const tTVPRect &srcrect,
	tTVPBBStretchType type, tjs_real typeopt, tTVPBBBltMethod method, tjs_int opa, bool hda)
{
	int ret = -1;
	switch(mode)
	{
	case 1:	// linear interpolation
	default:
		ret = ib_resample(dest, destrect, src, srcrect, triangle_filter, triangle_support);
		break;

	case 2:	// cubic interpolation
		ret = ib_resample(dest, destrect, src, srcrect, bell_filter, bell_support);
		break;
	}

	if(ret) TVPThrowInternalError;
}
//---------------------------------------------------------------------------

