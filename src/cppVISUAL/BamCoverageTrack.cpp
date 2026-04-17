#include "BamCoverageTrack.h"
#include "SharedData.h"

#include <QPainter>

static constexpr int TRACK_HEIGHT = 50;
static constexpr int SPACING_BELOW = 4;
static constexpr int MAX_REGION_LENGTH = 30'000;

BamCoverageTrack::BamCoverageTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name)
{
	max_coverage_ = 0;
	coverage_.fill(BaseCoverage(), MAX_REGION_LENGTH);
}

QSize BamCoverageTrack::sizeHint() const
{
	return QSize(parentWidget() ? parentWidget()->width() : 200,
				 TRACK_HEIGHT + SPACING_BELOW);
}

void BamCoverageTrack::setTrackData(QSharedPointer<BamTrackData> track_data)
{
	track_data_ = track_data;
	connect(track_data_.get(), SIGNAL(onDataUpdate()), this, SLOT(dataReady()));
}

void BamCoverageTrack::dataReady()
{
	storeCoverage();
	update();
}

void BamCoverageTrack::storeCoverage()
{
	max_coverage_ = 0;
	int max_region_len = SharedData::settings().bam_max_region_len;
	coverage_.fill(BaseCoverage(), max_region_len);

	const BedLine& region = SharedData::region();
	if (region.length() > max_region_len || !track_data_) return;

	const QVector<BamAlignment>& aligns = track_data_->getAlignments();

	//store coverage
	foreach (const BamAlignment& al, aligns)
	{
		if (al.end() < region.start()) continue;
		if (al.start() > region.end()) continue;

		int al_start = std::max(al.start(), region.start());
		int al_end	 = std::min(al.end(), region.end() + 1);

		for (int pos = al_start; pos < al_end; ++pos)
		{
			int idx = pos - region.start();
			char base;
			auto base_info = al.extractBaseByCIGAR(pos);
			base = base_info.first;
			switch (base)
			{
				case 'A': case 'a':  ++coverage_[idx].a; break;
				case 'C': case 'c':  ++coverage_[idx].c; break;
				case 'G': case 'g':  ++coverage_[idx].g; break;
				case 'T': case 't':  ++coverage_[idx].t; break;
				default: break;
			}
			if (coverage_[idx].total() > max_coverage_) max_coverage_ = coverage_[idx].total();
		}
	}

	const auto& ref_seq = track_data_->getReferenceSeq();
	for (int idx =0; idx < region.length(); ++idx)
	{
		BaseCoverage& cov = coverage_[idx];
		int total_count = cov.total();
		char ref_base = ref_seq[idx];
		int base_count = 0;
		switch (ref_base)
		{
			case 'A': case 'a': base_count = cov.a; break;
			case 'C': case 'c': base_count = cov.c; break;
			case 'G': case 'g': base_count = cov.g; break;
			case 'T': case 't': base_count = cov.t; break;
		}
		cov.is_variant = (total_count - base_count >= 3) && (((double)base_count / total_count) < 0.8);
	}
}

void BamCoverageTrack::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.fillRect(rect(), Qt::white);
	const BedLine& region = SharedData::region();
	drawLabel(painter);
	if (region.length() > MAX_REGION_LENGTH) drawZoomInText(painter);
	else drawCoverage(painter);
}

void BamCoverageTrack::drawZoomInText(QPainter& painter)
{
	painter.setPen(Qt::gray);
	painter.drawText(rect(), Qt::AlignCenter, "Zoom In To See Coverage");
}

void BamCoverageTrack::drawCoverage(QPainter& painter)
{
	const BedLine& region = SharedData::region();

	int draw_height = height();
	int label_width = SharedData::settings().label_width;
	float total_width = width() - label_width - 4;
	int x0 = label_width + 2;

	if (region.length() < 1500) painter.setPen(Qt::white);
	else painter.setPen(Qt::gray);

	float scale = (float)region.length() / total_width;

	for (int idx =0; idx < region.length(); ++idx)
	{
		const BaseCoverage& cov = coverage_[idx];
		int total_count = cov.total();
		if (total_count ==0) continue;

		int pX = x0 + (int)((float)idx / scale);
		int endX = x0 + (int)((float)(idx + 1) / scale);
		int dX = std::max(1, endX - pX);
		float norm = (float)total_count/max_coverage_;
		int bar_h = norm * draw_height;

		if (!cov.is_variant)
		{
			painter.setBrush(Qt::gray);
			painter.drawRect(pX, draw_height - bar_h, dX, bar_h);
		}
		else
		{
			QPen pen = painter.pen();
			int a_height = bar_h * ((float)cov.a / total_count);
			int c_height = bar_h * ((float)cov.c / total_count);
			int g_height = bar_h * ((float)cov.g / total_count);
			int t_height = bar_h * ((float)cov.t / total_count);

			int y_start = draw_height - a_height;
			if (region.length() >= 1500) painter.setPen(QColor(0, 150, 0));
			painter.setBrush(QColor(0, 150, 0)); //a
			painter.drawRect(pX, y_start, dX, a_height);

			y_start -= c_height;
			if (region.length() >= 1500) painter.setPen(Qt::blue);
			painter.setBrush(Qt::blue); //c
			painter.drawRect(pX, y_start, dX, c_height);

			y_start -= g_height;
			if (region.length() >= 1500) painter.setPen(QColor(209, 113, 5));
			painter.setBrush(QColor(209, 113, 5)); //g
			painter.drawRect(pX, y_start, dX, g_height);

			y_start -= t_height;
			if (region.length() >= 1500) painter.setPen(Qt::red);
			painter.setBrush(Qt::red); //t
			painter.drawRect(pX, y_start, dX, t_height);
			painter.setPen(pen);
		}
	}
}
