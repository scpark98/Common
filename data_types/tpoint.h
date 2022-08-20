#ifndef _T_POINT_H_
#define _T_POINT_H_

#ifndef		MIN
#define		MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef		MAX
#define		MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

template<class T> class tpoint
{
public:
	T		x, y;

	tpoint() : x(0), y(0) {}
	tpoint( T x1, T y1 )	{ x = x1; y = y1; }

	void	offset( T dx, T dy ) { x += dx; y += dy; }	
private:
};


#endif