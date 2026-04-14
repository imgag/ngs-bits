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
	coverage_.fill(BaseCoverage(), MAX_REGION_LENGTH);

	const BedLine& region = SharedData::region();
	if (region.length() > MAX_REGION_LENGTH || !track_data_) return;

	const QVector<BamAlignment>& aligns = track_data_->getAlignments();
	foreach (BamAlignment al, aligns)
	{
		if (al.isUnmapped() || al.isDuplicate() || al.isSecondaryAlignment()) continue;

		if (al.end() < region.start()) continue;
		if (al.start() > region.end()) continue;

		int al_start = std::max(al.start(), region.start());
		int al_end	 = std::min(al.end(), region.end());

		for (int pos = al_start; pos < al_end; ++pos)
		{
			int idx = pos - region.start();
			char base;
			/*TODO: see if extractBaseByCIGAR can be made const
			 * without introducing bugs in the rest of the repo,
			 * that would enable the use of const BamAlignment&
			 */
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

		int max_base = cov.max();
		/*TODO: use reference genome!!*/
		bool is_variant = ((double)max_base / total_count) < 0.8;

		if (!is_variant)
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
			painter.setPen(QColor(0, 150, 0));
			painter.setBrush(QColor(0, 150, 0)); //a
			painter.drawRect(pX, y_start, dX, a_height);

			y_start -= c_height;
			painter.setPen(Qt::blue);
			painter.setBrush(Qt::blue); //c
			painter.drawRect(pX, y_start, dX, c_height);

			y_start -= g_height;
			painter.setPen(QColor(209, 113, 5));
			painter.setBrush(QColor(209, 113, 5)); //g
			painter.drawRect(pX, y_start, dX, g_height);

			y_start -= t_height;
			painter.setPen(Qt::red);
			painter.setBrush(Qt::red); //t
			painter.drawRect(pX, y_start, dX, t_height);
			painter.setPen(pen);
		}
	}
}
