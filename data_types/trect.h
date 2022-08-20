#ifndef _T_RECT_H_
#define _T_RECT_H_

#ifndef		MIN
#define		MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef		MAX
#define		MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

template<class T> class trect
{
public:
	T	l, t, r, b;

	trect() : l(0), t(0), r(0), b(0) {}
	trect( T l1, T t1, T r1, T b1 )	{ l = l1; t = t1; r = r1; b = b1; }

	void	offset( T dx, T dy ) { l += dx; t += dy; r += dx; b += dy; }	
	T		width() { return (r - l); }
	T		height() { return (b - t); }
	T		centerx() { return ((l + r) / 2.0); }
	T		centery() { return ((t + b) / 2.0); }

	//두 사각형의 겹치는 영역을 리턴한다.	
	trect	getIntersectionRect( trect r1 )
	{
		scp_rect	r(0, 0, 0, 0);

		if (l > r1.r ) return r;
		if (r < r1.l ) return r;
		if (t > r1.b ) return r;
		if (b < r1.t ) return r;

		r.l = MAX( l, r1.l );
		r.t = MAX( t, r1.t );
		r.r = MIN( r, r1.r ) - r.l;
		r.b = MIN( b, r1.b ) - r.t;

		return r;
	}

	void setRectEmpty()
	{
		l = t = r = b = 0;
	}

	bool isRectEmpty()
	{
		return ( width() <= 0 || height() <= 0 );
	}

	bool isRectNull()
	{
		return ( width() == 0 && height() == 0 );
	}

	T getSize() { return width() * height(); }

	//format
	//0 : "1, 2, 3, 4"
	//1 : "(1,2) x (3,4)"
	//2 : "l = 1, t = 2, r = 3, b = 4"
	//template<class T>
	char* getRectInfoString( int format )
	{
		static char str[128];

		if ( format == 2 )
			sprintf( str, "l = %d, t = %d, r = %d, b = %d", l, t, r, b );
		else if ( format == 1 )
			sprintf( str, "(%d,%d) x (%d,%d)", l, t, r, b );
		else
			sprintf( str, "%d, %d, %d, %d", l, t, r, b );
		return str;
	}
	
};

#endif