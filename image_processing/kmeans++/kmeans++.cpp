#include "kmeans++.h"

vsdouble randf(vsdouble m)
{
	return m * rand() / (RAND_MAX - 1.);
}

kmpoint gen_xy( int count, vsdouble dMaxX, vsdouble dMaxY )
{
	kmpoint p, pt = (kmpoint)malloc(sizeof(kmpoint_t) * count);

	for (p = pt + count; p-- > pt;)
	{
		p->x = randf( dMaxX );
		p->y = randf( dMaxY );
		//printf( "%f, %f\n", p->x, p->y );
	}

	return pt;
}

inline vsdouble dist2(kmpoint a, kmpoint b)
{
	vsdouble x = a->x - b->x, y = a->y - b->y;
	return x*x + y*y;
}

inline int nearest(kmpoint pt, kmpoint cent, int n_cluster, vsdouble *d2)
{
	int i, min_i = pt->group;
	kmpoint c;
	double d, min_d = HUGE_VAL;

#define for_n for (c = cent, i = 0; i < n_cluster; i++, c++)
	for_n {
		min_d = HUGE_VAL;	//min_d가 int로 선언되면 안되는 이유!
		min_i = pt->group;
		for_n {
			if (min_d > (d = dist2(c, pt))) {
				min_d = d; min_i = i;
			}
		}
	}
	if (d2) *d2 = min_d;
	return min_i;
}

void kpp(kmpoint pts, int len, kmpoint cent, int n_cent)
{
#define for_len for (j = 0, p = pts; j < len; j++, p++)
	int j;
	int n_cluster;
	vsdouble sum, *d = (vsdouble*)malloc(sizeof(vsdouble) * len);

	kmpoint p;
	cent[0] = pts[ rand() % len ];
	for (n_cluster = 1; n_cluster < n_cent; n_cluster++) {
		sum = 0;
		for_len {
			nearest(p, cent, n_cluster, d + j);
			sum += d[j];
		}
		sum = randf(sum);
		for_len {
			if ((sum -= d[j]) > 0) continue;
			cent[n_cluster] = pts[j];
			break;
		}
	}
	for_len p->group = nearest(p, cent, n_cluster, 0);
	free(d);
}

kmpoint lloyd(kmpoint pts, int len, int n_cluster)
{
	int i, j, min_i;
	int changed;

	kmpoint cent = (kmpoint)malloc(sizeof(kmpoint_t) * n_cluster), p, c;

	/* assign init grouping randomly */
	//for_len p->group = j % n_cluster;

	/* or call k++ init */
	kpp(pts, len, cent, n_cluster);

	do {
		/* group element for centroids are used as counters */
		for_n { c->group = 0; c->x = c->y = 0; }
		for_len {
			c = cent + p->group;
			c->group++;
			c->x += p->x; c->y += p->y;
		}
		for_n
		{
			if ( c->group != 0 )
			{
				c->x /= c->group;
				c->y /= c->group;
			}
		}

		changed = 0;
		/* find closest centroid of each point */
		for_len {
			min_i = nearest(p, cent, n_cluster, 0);
			if (min_i != p->group) {
				changed++;
				p->group = min_i;
			}
		}
	} while (changed > (len >> 10)); /* stop when 99.9% of points are good */

	for_n { c->group = i; }

	for ( i = 0; i < n_cluster; i++ )
		printf( "%d : %f, %f\n", i, cent[i].x, cent[i].y );

	return cent;
}

void print_eps(kmpoint pts, int len, kmpoint cent, int n_cluster)
{
#	define W 400
#	define H 400
	int i, j;
	kmpoint p, c;
	vsdouble min_x, max_x, min_y, max_y, scale, cx, cy;
	vsdouble *colors = (vsdouble*)malloc(sizeof(vsdouble) * n_cluster * 3);

	for_n {
		colors[3*i + 0] = (3 * (i + 1) % 11)/11.;
		colors[3*i + 1] = (7 * i % 11)/11.;
		colors[3*i + 2] = (9 * i % 11)/11.;
	}

	max_x = max_y = -(min_x = min_y = HUGE_VAL);
	for_len {
		if (max_x < p->x) max_x = p->x;
		if (min_x > p->x) min_x = p->x;
		if (max_y < p->y) max_y = p->y;
		if (min_y > p->y) min_y = p->y;
	}
	scale = W / (max_x - min_x);
	if (scale > H / (max_y - min_y)) scale = H / (max_y - min_y);
	cx = (max_x + min_x) / 2;
	cy = (max_y + min_y) / 2;

	printf("%%!PS-Adobe-3.0\n%%%%BoundingBox: -5 -5 %d %d\n", W + 10, H + 10);
	printf( "/l {rlineto} def /m {rmoveto} def\n"
		"/c { .25 sub exch .25 sub exch .5 0 360 arc fill } def\n"
		"/s { moveto -2 0 m 2 2 l 2 -2 l -2 -2 l closepath "
		"	gsave 1 setgray fill grestore gsave 3 setlinewidth"
		" 1 setgray stroke grestore 0 setgray stroke }def\n"
		);
	for_n {
		printf("%g %g %g setrgbcolor\n",
			colors[3*i], colors[3*i + 1], colors[3*i + 2]);
		for_len {
			if (p->group != i) continue;
			//각 클러스터별 좌표값을 변환해서 출력.
			//printf("%.3f %.3f c\n", (p->x - cx) * scale + W / 2, (p->y - cy) * scale + H / 2);
			//데이터 확인을 위해 원래의 좌표값을 출력하도록 수정.
			printf("%f %f\n", p->x, p->y);
		}
		printf("\n0 setgray %g %g s\n",
			(c->x - cx) * scale + W / 2,
			(c->y - cy) * scale + H / 2);
	}
	printf("\n%%%%EOF");
	free(colors);
#	undef for_n
#	undef for_len
}

#define PTS 100
#define K 5
/*
int main()
{
int i;

//반지름 10의 PTS개의 랜덤 포인트 생성
point v = gen_xy(PTS, 10);

//포인트 데이터, 데이터 개수, 클러스터 수
point c = lloyd(v, PTS, K);

print_eps(v, PTS, c, K);
// free(v); free(c);
return 0;
}
*/