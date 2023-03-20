/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef _BITMAP_UTILITIES_H
#define	_BITMAP_UTILITIES_H


#include <Bitmap.h>


#include "PixelOperations.h"


class BitmapUtilities {
public:
	static	status_t	FixMissingAlpha(BBitmap *bitmap);
	static	BBitmap*	ConvertColorSpace(BBitmap *inBitmap,
							color_space wantSpace);
	static	BBitmap*	ConvertToMask(BBitmap *inBitmap, uint8 color);
	static  void		CompositeBitmapOnSource(BBitmap* toBuffer,
							BBitmap* srcBuffer, BBitmap* fromBuffer,
							BRect updated_rect,
							uint32 (*composite_func)(uint32, uint32) = src_over_fixed);
	static  void		ClearBitmap(BBitmap* bitmap, uint32 color,
							BRect* area = NULL);
	static	void		CheckerBitmap(BBitmap* bitmap,
							uint32 color1, uint32 color2,
							uint32 grid_size, BRect* area = NULL);
};


#endif	// _BITMAP_UTILITIES_H
