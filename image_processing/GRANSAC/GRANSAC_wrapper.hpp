#pragma once

#include "GRANSAC.hpp"
#include "LineModel.hpp"

//opencv를 쓸 수 있는 환경이라면 
//cv::fitLine을 쓰는 것이 좀 더 정확하다.

class GRansacWrapper
{
	std::vector<std::shared_ptr<GRANSAC::AbstractParameter>> candPoints;
	Point2D result[2];

public:
	GRansacWrapper()
	{
		candPoints.clear();
		result[0] = Point2D(0,0);
		result[1] = Point2D(0,0);
	};

	void reset()
	{
		candPoints.clear();
	}

	void add_point( int x, int y )
	{
		std::shared_ptr<GRANSAC::AbstractParameter> CandPt = std::make_shared<Point2D>(x, y);
		candPoints.push_back(CandPt);
	}

	int get_cand_count()
	{
		return candPoints.size();
	}

	Point2D get_point( int idx )
	{
		//Point2D pt(0, 0);
		if ( idx < 0 || idx >= candPoints.size() )
			return Point2D(0,0);

		auto RPt = std::dynamic_pointer_cast<Point2D>(candPoints[idx]);
		return Point2D(RPt->x, RPt->y);
	}

	Point2D get_result( int idx )
	{
		return result[idx];
	}

	//두 점을 지나는 직선상의 x3를 구한다.
	double getLinePointX( double x1, double y1, double x2, double y2, double y3 )
	{
		if ( x1 == x2 )
			return x1;

		double a = (double)(y2 - y1) / (double)(x2 - x1);
		double b = y2 - a * x2;
		return ((y3 - b) / a);
	}

	//두 점을 지나는 직선상의 y3를 구한다.
	double getLinePointY( double x1, double y1, double x2, double y2, double x3 )
	{
		if ( x1 == x2 )
			return x1;

		double a = (double)(y2 - y1) / (double)(x2 - x1);
		double b = y2 - a * x2;
		return (a * x3 + b);
	}

	bool estimate(Point2D &p, Point2D &q)
	{
		Point2D pp(p), qq(q);

		GRANSAC::RANSAC<Line2DModel, 2> Estimator;
		Estimator.Initialize(1, 10); // Threshold, iterations
		//int start = cv::getTickCount();
		Estimator.Estimate(candPoints);
		//int end = cv::getTickCount();
		//std::cout << "RANSAC took: " << GRANSAC::VPFloat(end - start) / GRANSAC::VPFloat(cv::getTickFrequency()) * 1000.0 << " ms." << std::endl;
		/*
		auto BestInliers = Estimator.GetBestInliers();
		if (BestInliers.size() > 0)
		{
			for (auto& Inlier : BestInliers)
			{
				auto RPt = std::dynamic_pointer_cast<Point2D>(Inlier);
				//cv::Point Pt(floor(RPt->x), floor(RPt->y));
				////cv::circle(Canvas, Pt, floor(Side / 100), cv::Scalar(0, 255, 0), -1);
				//CClientDC dc(this);
				//dc.FillSolidRect( floor(RPt->x) - 2, floor(RPt->y) - 2, 4, 4, GREEN );
			}
		}
		*/
		auto BestLine = Estimator.GetBestModel();
		if (BestLine)
		{
			auto BestLinePt1 = std::dynamic_pointer_cast<Point2D>(BestLine->GetModelParams()[0]);
			auto BestLinePt2 = std::dynamic_pointer_cast<Point2D>(BestLine->GetModelParams()[1]);
			if (BestLinePt1 && BestLinePt2)
			{
				result[0].x = BestLinePt1->x;
				result[0].y = BestLinePt1->y;
				result[1].x = BestLinePt2->x;
				result[1].y = BestLinePt2->y;

				Point2D a(result[0].x, result[0].y);
				Point2D b(result[1].x, result[1].y);

				double slope = (b.y - a.y)/(b.x - a.x);

				p.y = -(a.x - pp.x) * slope + a.y;
				if ( p.y < pp.y )
				{
					p.x = getLinePointX( a.x, a.y, b.x, b.y, pp.y );
					p.y = pp.y;
				}
				else if ( p.y >= qq.y )
				{
					p.x = getLinePointX( a.x, a.y, b.x, b.y, qq.y );
					p.y = qq.y;
				}

				q.y = -(b.x - qq.x) * slope + b.y;
				if ( q.y < pp.y )
				{
					q.x = getLinePointX( a.x, a.y, b.x, b.y, pp.y );
					q.y = pp.y;
				}
				else if ( q.y >= qq.y )
				{
					q.x = getLinePointX( a.x, a.y, b.x, b.y, qq.y );
					q.y = qq.y;
				}

				//printf( "p : %.0f, %.0f    q = %.0f, %.0f\n", p.x, p.y, q.x, q.y );
			}
			return true;
		}
		return false;
	}

};