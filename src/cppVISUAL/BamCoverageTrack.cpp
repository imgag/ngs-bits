#include "BamCoverageTrack.h"
#include "BamTrackDataManager.h"
#include "SharedData.h"

#include <QApplication>
#include <QPainter>

static constexpr int TRACK_HEIGHT = 50;
static constexpr int SPACING_BELOW = 4;

BamCoverageTrack::BamCoverageTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name)
{
	max_coverage_ = 0;
	int max_region_length = SharedData::settings().bam_max_region_len;
	coverage_.fill(BaseCoverage(), max_region_length);
}

BamCoverageTrack* BamCoverageTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
	QSharedPointer<BamTrackData> data = BamTrackDataManager::getOrCreate(file_path);
	if (data)
	{
		auto track = new BamCoverageTrack(parent, file_path, name);
		track->setTrackData(data);
		return track;
	}
	return nullptr;
}

QSize BamCoverageTrack::sizeHint() const
{
	return QSize(parentWidget() ? parentWidget()->width() : 200,
				 TRACK_HEIGHT + SPACING_BELOW);
}

void BamCoverageTrack::setTrackData(QSharedPointer<BamTrackData> track_data)
{
	if(track_data)
	{
		track_data_ = track_data;
		connect(track_data_.get(), SIGNAL(onDataUpdate()), this, SLOT(dataReady()));
		dataReady();
	}
}

void BamCoverageTrack::reloadTrack()
{
	BamTrackDataManager::reload(file_path_);
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

	const QVector<BamAlignmentWrapper>& aligns = track_data_->getAlignments();

	int ref_start =  track_data_->getRefSeqStart();

	//store coverage
	for (const auto& wrapped : aligns)
	{
		const BamAlignment& al = wrapped.alignment;

		if (al.end() < region.start()) continue;
		if (al.start() > region.end()) continue;

		foreach (const auto& data, wrapped.getEvents())
		{
			int base_idx = data.genome_pos - region.start();
			if (data.event == BamAlignmentWrapper::MATCH)
			{
				for (int i =0; i < data.length; ++i)
				{
					int idx = base_idx + i;
					if (idx < 0 || idx >= coverage_.length()) continue;
					BaseCoverage& cov = coverage_[idx];
					char base = data.bases[i] | 32;
					if (!al.isReverseStrand())
					{
						switch (base)
						{
						case 'A': case 'a': ++cov.forward_a; break;
						case 'C': case 'c': ++cov.forward_c; break;
						case 'G': case 'g': ++cov.forward_g; break;
						case 'T': case 't': ++cov.forward_t; break;
						}
					}
					else
					{
						switch (base)
						{
						case 'A': case 'a': ++cov.reverse_a; break;
						case 'C': case 'c': ++cov.reverse_c; break;
						case 'G': case 'g': ++cov.reverse_g; break;
						case 'T': case 't': ++cov.reverse_t; break;
						}
					}
					max_coverage_ = std::max(max_coverage_, cov.total());
				}
			}
			else if (data.event == BamAlignmentWrapper::INSERTION)
			{
				if (base_idx >= 0 && base_idx < coverage_.length()) coverage_[base_idx].insertions ++;
			}
			else if (data.event == BamAlignmentWrapper::DELETION)
			{
				for (int i =0; i < data.length; ++i)
				{
					int idx = base_idx + i;
					if (idx < 0 || idx >= coverage_.length()) continue;
					BaseCoverage& cov = coverage_[idx];
					cov.deletions++;
				}
			}
		}
	}

	const auto& ref_seq = track_data_->getReferenceSeq();
	for (int idx =0; idx < region.length(); ++idx)
	{
		BaseCoverage& cov = coverage_[idx];
		int total_count = cov.total();
		int offset = region.start() - ref_start;
		char ref_base = ref_seq[idx + offset];
		int base_count = 0;
		switch (ref_base)
		{
			case 'A': case 'a': base_count = cov.a(); break;
			case 'C': case 'c': base_count = cov.c(); break;
			case 'G': case 'g': base_count = cov.g(); break;
			case 'T': case 't': base_count = cov.t(); break;
		}
		cov.is_variant = (((double)base_count / total_count) < (1.f - SharedData::settings().coverage_mismatch_threshold));
	}
}

void BamCoverageTrack::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.fillRect(rect(), Qt::white);
	const BedLine& region = SharedData::region();
	drawLabel(painter);
	int max_region_length = SharedData::settings().bam_max_region_len;
	if (region.length() > max_region_length) drawZoomInText(painter);
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

	if (region.length() < 1500) painter.setPen(Qt::white);
	else painter.setPen(Qt::gray);


	for (int idx =0; idx < region.length(); ++idx)
	{
		const BaseCoverage& cov = coverage_[idx];
		int total_count = cov.total();
		if (total_count ==0) continue;

		int pX = chrToScreen(region.start() + idx);
		int endX = chrToScreen(region.start() + idx + 1);
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
			int a_height = bar_h * ((float)cov.a() / total_count);
			int c_height = bar_h * ((float)cov.c() / total_count);
			int g_height = bar_h * ((float)cov.g() / total_count);
			int t_height = bar_h * ((float)cov.t() / total_count);

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


QString BamCoverageTrack::getCoverageText(const BaseCoverage& cov, int coverage_idx)
{
	const BedLine& region = SharedData::region();
	int coverage_pos = region.start() + coverage_idx;

	QString info = QString("%1:%2\nTotal count: %3\n")
					   .arg(region.chr().str())
					   .arg(coverage_pos)
					   .arg(cov.total());

	// Helper struct
	struct BaseInfo {
		char symbol;
		int count;
		int fwd;
		int rev;
	};

	BaseInfo bases[] = {
		{'A', cov.a(), cov.forward_a, cov.reverse_a},
		{'C', cov.c(), cov.forward_c, cov.reverse_c},
		{'G', cov.g(), cov.forward_g, cov.reverse_g},
		{'T', cov.t(), cov.forward_t, cov.reverse_t}
	};

	for (const auto& b : bases) {
		int total = cov.total();
		int percent = (total > 0) ? static_cast<int>(100.0 * b.count / total) : 0;
		info += QString("%1: %2 (%3%, %4+, %5-)\n")
					.arg(b.symbol)
					.arg(b.count)
					.arg(percent)
					.arg(b.fwd)
					.arg(b.rev);
	}

	if (cov.insertions != 0 || cov.deletions != 0)
	{
		info += QString("Insertions: %1\nDeletions: %2\n")
					.arg(cov.insertions)
					.arg(cov.deletions);
	}

	return info;
}

void BamCoverageTrack::handlePopupRequest(QPointF local_pos, QPointF global_pos)
{
	const BedLine& region = SharedData::region();
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;

	if (local_pos.x() < label_width + 2 ||
		local_pos.x() > width() - 2) return;

	float p = (float)(local_pos.x() - label_width - 2)/(total_width);
	int x = region.length() * p;

	if (x < 0 || x >= coverage_.length()) return;

	const BaseCoverage& cov = coverage_[x];
	QString info = getCoverageText(cov, x);
	showInfoPopup(global_pos, info);
}

void BamCoverageTrack::mousePressEvent(QMouseEvent* event)
{
	mouse_press_pos_ = event->pos();
	TrackWidget::mousePressEvent(event);
}

void BamCoverageTrack::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
	{
		TrackWidget::mouseReleaseEvent(event);
		return;
	}
	bool dragging =	((event->pos() - mouse_press_pos_).manhattanLength() >= QApplication::startDragDistance());

	if (!dragging)
	{
		handlePopupRequest(event->pos(), event->globalPosition());
	}

	TrackWidget::mouseReleaseEvent(event);
}
