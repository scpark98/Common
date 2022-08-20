#pragma once

#include "GRANSAC.hpp"
#include "LineModel.hpp"

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
		return Point2D(RPt->m_Point2D[0], RPt->m_Point2D[1]);
	}

	Point2D get_result( int idx )
	{
		return result[idx];
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
				//cv::Point Pt(floor(RPt->m_Point2D[0]), floor(RPt->m_Point2D[1]));
				////cv::circle(Canvas, Pt, floor(Side / 100), cv::Scalar(0, 255, 0), -1);
				//CClientDC dc(this);
				//dc.FillSolidRect( floor(RPt->m_Point2D[0]) - 2, floor(RPt->m_Point2D[1]) - 2, 4, 4, GREEN );
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
				result[0].m_Point2D[0] = BestLinePt1->m_Point2D[0];
				result[0].m_Point2D[1] = BestLinePt1->m_Point2D[1];
				result[1].m_Point2D[0] = BestLinePt2->m_Point2D[0];
				result[1].m_Point2D[1] = BestLinePt2->m_Point2D[1];

				Point2D a(result[0].m_Point2D[0], result[0].m_Point2D[1]);
				Point2D b(result[1].m_Point2D[0], result[1].m_Point2D[1]);

				double slope = (double)(b.m_Point2D[1] - a.m_Point2D[1])/(b.m_Point2D[0] - a.m_Point2D[0]);

				p.m_Point2D[1] = -(a.m_Point2D[0] - pp.m_Point2D[0]) * slope + a.m_Point2D[1];
				if ( p.m_Point2D[1] < pp.m_Point2D[1] )
					p.m_Point2D[1] = pp.m_Point2D[1];
				else if ( p.m_Point2D[1] >= qq.m_Point2D[1] )
					p.m_Point2D[1] = qq.m_Point2D[1] - 1;

				q.m_Point2D[1] = -(b.m_Point2D[0] - qq.m_Point2D[0]) * slope + b.m_Point2D[1];
				if ( q.m_Point2D[1] < pp.m_Point2D[1] )
					q.m_Point2D[1] = pp.m_Point2D[1];
				else if ( q.m_Point2D[1] >= qq.m_Point2D[1] )
					q.m_Point2D[1] = qq.m_Point2D[1] - 1;
			}
			return true;
		}
		return false;
	}

};