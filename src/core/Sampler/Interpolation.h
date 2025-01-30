/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <cmath>
#include <QString>

namespace H2Core
{

namespace Interpolation
{
	enum class InterpolateMode { Linear = 0,
								Cosine = 1,
								Third = 2,
								Cubic = 3,
								Hermite = 4 };

	static const QString ModeToQString( const InterpolateMode& mode )
	{
		switch ( mode ) {
		case InterpolateMode::Linear:
			return "Linear";
		case InterpolateMode::Cosine:
			return "Cosine";
		case InterpolateMode::Third:
			return "Third";
		case InterpolateMode::Cubic:
			return "Cubic";
		case InterpolateMode::Hermite:
			return "Hermite";
		default:
			return "<unknown>";
		}
	}

	inline static float linear_Interpolate( float y1, float y2, float mu )
	{
			/*
			 * mu defines where to estimate the value on the interpolated line
			 * y1 = buffervalue on position
			 * y2 = buffervalue on position +1
			 */
			return y1 * ( 1 - mu ) + y2 * mu;
	};
	
	inline static float cosine_Interpolate( float y1, float y2, double mu )
	{
			/*
			 * mu defines where to estimate the value on the interpolated line
			 * y1 = buffervalue on position
			 * y2 = buffervalue on position +1
			 */
			double mu2;
	
			mu2 = ( 1 - cos ( mu * 3.14159 ) ) / 2;
			return( y1 * (1 - mu2 ) + y2 * mu2 );
	};
	
	inline static float third_Interpolate( float y0, float y1, float y2, float y3, double mu )
	{
			/*
			 * mu defines where to estimate the value on the interpolated line
			 * y0 = buffervalue on position -1
			 * y1 = buffervalue on position
			 * y2 = buffervalue on position +1
			 * y3 = buffervalue on position +2
			 */
	
			float c0 = y1;
			float c1 = 0.5f * ( y2 - y0 );
			float c3 = 1.5f * ( y1 - y2 ) + 0.5f * ( y3 - y0 );
			float c2 = y0 - y1 + c1 - c3;
			return ( ( c3 * mu + c2 ) * mu + c1 ) * mu + c0;
	};
	
	inline static float cubic_Interpolate( float y0, float y1, float y2, float y3, double mu)
	{
			/*
			 * mu defines where to estimate the value on the interpolated line
			 * y0 = buffervalue on position -1
			 * y1 = buffervalue on position
			 * y2 = buffervalue on position +1
			 * y3 = buffervalue on position +2
			 */
	
			double a0, a1, a2, a3, mu2;
	
			mu2 = mu * mu;
			a0 = y3 - y2 - y0 + y1;
			a1 = y0 - y1 - a0;
			a2 = y2 - y0;
			a3 = y1;
	
			return( a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3 );
	};
	
	inline static float hermite_Interpolate( float y0, float y1, float y2, float y3, double mu )
	{
			/*
			 * mu defines where to estimate the value on the interpolated line
			 * y0 = buffervalue on position -1
			 * y1 = buffervalue on position
			 * y2 = buffervalue on position +1
			 * y3 = buffervalue on position +2
			 */
	
			double a0, a1, a2, a3, mu2;
	
			mu2 = mu * mu;
			a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
			a1 = y0 - 2.5 * y1 + 2 * y2 - 0.5 * y3;
			a2 = -0.5 * y0 + 0.5 * y2;
			a3 = y1;
	
			return( a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3 );
	};

	template < InterpolateMode mode >
	inline static float interpolate( float y0, float y1, float y2, float y3, double mu )
	{
		switch ( mode ) {
		case InterpolateMode::Linear:
			return linear_Interpolate( y1, y2, mu );
		case InterpolateMode::Cosine:
			return cosine_Interpolate( y1, y2, mu );
		case InterpolateMode::Third:
			return third_Interpolate( y0, y1, y2, y3, mu );
		case InterpolateMode::Cubic:
			return cubic_Interpolate( y0, y1, y2, y3, mu );
		case InterpolateMode::Hermite:
			return hermite_Interpolate( y0, y1, y2, y3, mu );
		default:
			assert( false && "Unknown interpolation mode" );
		}

	}

};

}

#endif // INTERPOLATION_H
