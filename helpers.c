/* MyPaint - pressure sensitive painting application
 * Copyright (C) 2006-2007 Martin Renold and others
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "helpers.h"
#include <glib.h>
#include <math.h>

// stolen from the gimp (noisify.c)

/*
 * Return a Gaussian (aka normal) random variable.
 *
 * Adapted from ppmforge.c, which is part of PBMPLUS.
 * The algorithm comes from:
 * 'The Science Of Fractal Images'. Peitgen, H.-O., and Saupe, D. eds.
 * Springer Verlag, New York, 1988.
 *
 * It would probably be better to use another algorithm, such as that
 * in Knuth
 */
gdouble rand_gauss (GRand * rng)
{
  gint i;
  gdouble sum = 0.0;

  for (i = 0; i < 4; i++) {
    sum += g_rand_int_range (rng, 0, 0x7FFF);
  }

  return sum * 5.28596089837e-5 - 3.46410161514;
}

// stolen from the gimp (gimpcolorspace.c)

/*  gint functions  */

/**
 * gimp_rgb_to_hsv_int:
 * @red: The red channel value, returns the Hue channel
 * @green: The green channel value, returns the Saturation channel
 * @blue: The blue channel value, returns the Value channel
 *
 * The arguments are pointers to int representing channel values in
 * the RGB colorspace, and the values pointed to are all in the range
 * [0, 255].
 *
 * The function changes the arguments to point to the HSV value
 * corresponding, with the returned values in the following
 * ranges: H [0, 360], S [0, 255], V [0, 255].
 **/
void
rgb_to_hsv_int (gint *red,
                gint *green,
                gint *blue)
{
  gdouble  r, g, b;
  gdouble  h, s, v;
  gint     min;
  gdouble  delta;

  r = *red;
  g = *green;
  b = *blue;

  if (r > g)
    {
      v = MAX (r, b);
      min = MIN (g, b);
    }
  else
    {
      v = MAX (g, b);
      min = MIN (r, b);
    }

  delta = v - min;

  if (v == 0.0)
    s = 0.0;
  else
    s = delta / v;

  if (s == 0.0)
    h = 0.0;
  else
    {
      if (r == v)
	h = 60.0 * (g - b) / delta;
      else if (g == v)
	h = 120 + 60.0 * (b - r) / delta;
      else
	h = 240 + 60.0 * (r - g) / delta;

      if (h < 0.0)
	h += 360.0;
      if (h > 360.0)
	h -= 360.0;
    }

  *red   = ROUND (h);
  *green = ROUND (s * 255.0);
  *blue  = ROUND (v);
}

/**
 * gimp_hsv_to_rgb_int:
 * @hue: The hue channel, returns the red channel
 * @saturation: The saturation channel, returns the green channel
 * @value: The value channel, returns the blue channel
 *
 * The arguments are pointers to int, with the values pointed to in the
 * following ranges:  H [0, 360], S [0, 255], V [0, 255].
 *
 * The function changes the arguments to point to the RGB value
 * corresponding, with the returned values all in the range [0, 255].
 **/
void
hsv_to_rgb_int (gint *hue,
                gint *saturation,
                gint *value)
{
  gdouble h, s, v, h_temp;
  gdouble f, p, q, t;
  gint i;

  if (*saturation == 0)
    {
      *hue        = *value;
      *saturation = *value;
      *value      = *value;
    }
  else
    {
      h = *hue;
      s = *saturation / 255.0;
      v = *value      / 255.0;

      if (h == 360)
         h_temp = 0;
      else
         h_temp = h;

      h_temp = h_temp / 60.0;
      i = floor (h_temp);
      f = h_temp - i;
      p = v * (1.0 - s);
      q = v * (1.0 - (s * f));
      t = v * (1.0 - (s * (1.0 - f)));

      switch (i)
	{
	case 0:
	  *hue        = ROUND (v * 255.0);
	  *saturation = ROUND (t * 255.0);
	  *value      = ROUND (p * 255.0);
	  break;

	case 1:
	  *hue        = ROUND (q * 255.0);
	  *saturation = ROUND (v * 255.0);
	  *value      = ROUND (p * 255.0);
	  break;

	case 2:
	  *hue        = ROUND (p * 255.0);
	  *saturation = ROUND (v * 255.0);
	  *value      = ROUND (t * 255.0);
	  break;

	case 3:
	  *hue        = ROUND (p * 255.0);
	  *saturation = ROUND (q * 255.0);
	  *value      = ROUND (v * 255.0);
	  break;

	case 4:
	  *hue        = ROUND (t * 255.0);
	  *saturation = ROUND (p * 255.0);
	  *value      = ROUND (v * 255.0);
	  break;

	case 5:
	  *hue        = ROUND (v * 255.0);
	  *saturation = ROUND (p * 255.0);
	  *value      = ROUND (q * 255.0);
	  break;
	}
    }
}


// inputs and outputs are all [0,1]
void
rgb_to_hsv_float (float *r_, float *g_, float *b_)
{
  float max, min, delta;
  float h, s, v;
  float r, g, b;

  r = *r_;
  g = *g_;
  b = *b_;

  max = MAX3(r, g, b);
  min = MIN3(r, g, b);

  v = max;
  delta = max - min;

  if (delta > 0.0001)
    {
      s = delta / max;

      if (r == max)
        {
          h = (g - b) / delta;
          if (h < 0.0)
            h += 6.0;
        }
      else if (g == max)
        {
          h = 2.0 + (b - r) / delta;
        }
      else if (b == max)
        {
          h = 4.0 + (r - g) / delta;
        }

      h /= 6.0;
    }
  else
    {
      s = 0.0;
      h = 0.0;
    }

  *r_ = h;
  *g_ = s;
  *b_ = v;
}


// tested, copied from my mass project
void ExpandRectToIncludePoint(Rect * r, int x, int y) 
{
  if (r->w == 0) {
    r->w = 1; r->h = 1;
    r->x = x; r->y = y;
  } else {
    if (x < r->x) { r->w += r->x-x; r->x = x; } else
    if (x >= r->x+r->w) { r->w = x - r->x + 1; }

    if (y < r->y) { r->h += r->y-y; r->y = y; } else
    if (y >= r->y+r->h) { r->h = y - r->y + 1; }
  }
}
