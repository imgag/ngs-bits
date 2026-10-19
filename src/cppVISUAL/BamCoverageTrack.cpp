#include "BamCoverageTrack.h"
#include "BamTrackDataManager.h"
#include "SharedData.h"

#include <QApplication>
#include <QPainter>

static constexpr int SPACING_BELOW = 4;

BamCoverageTrack::BamCoverageTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name, type())
{
	initializeSettings(&fast_settings_);
	storeCoverage();
	connect(&settings_, &ParameterList::parameterChanged, this, [this]()
	{
		if (track_data_) track_data_->setVisibilityRange(fast_settings_.visibility_range);
		storeCoverage();
		updateGeometry();
		update();
	});
	connect(SharedData::instance(), &SharedData::settingsChanged, this, [this]()
	{
		//Attached BAM data refreshes coverage through onDataUpdate after reloading.
		if (!track_data_) storeCoverage();
		updateGeometry();
		update();
	});
}

QList<Parameter> BamCoverageTrack::getParameters() const
{
	return {
		Parameter("show soft-clipped bases", "Include soft-clipped bases in coverage.", ParameterType::BOOL, {}),
		Parameter("show secondary alignments", "Include secondary alignments in coverage.", ParameterType::BOOL, {}),
		Parameter("min MQ", "Minimum mapping quality of displayed reads.", ParameterType::INT, {{ConstraintType::MIN, 0}, {ConstraintType::MAX, 255}}),
		Parameter("min display AF for SNP", "Minimum SNP allele frequency to color coverage bar.", ParameterType::DOUBLE, {{ConstraintType::MIN, 0.0}, {ConstraintType::MAX, 1.0}, {ConstraintType::DISPLAY_DIGITS, 2}}),
		Parameter("track height", "Height of track in pixels.", ParameterType::INT, {{ConstraintType::MIN, 1}}),
		Parameter("visibility range", "Maximum genomic region for which content is displayed (in bases).", ParameterType::INT, {{ConstraintType::MIN, 1}})
	};
}

QHash<QByteArray, QVariant> BamCoverageTrack::getParameterDefaults() const
{
	return {{"visibility range", SharedData::settings().bam_max_visibility_region}, {"show soft-clipped bases", false}, {"show secondary alignments", true}, {"min MQ", 0}, {"min display AF for SNP", 0.2}, {"track height", 50}};
}

BamCoverageTrack::~BamCoverageTrack()
{
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
				 fast_settings_.track_height + SPACING_BELOW);
}

void BamCoverageTrack::setTrackData(QSharedPointer<BamTrackData> track_data)
{
	if(track_data)
	{
		track_data_ = track_data;
		connect(track_data_.get(), SIGNAL(onDataUpdate()), this, SLOT(dataReady()));
		track_data_->bindVisibilityRange(settings_);
		track_data_->bindReadFilters(settings_);
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
	max_coverage_ = 10;
	coverage_.clear();

	const BedLine& region = SharedData::region();
	if (region.length() > fast_settings_.visibility_range) return;
	coverage_.fill(BaseCoverage(), region.length());
	if (!track_data_) return;

	const QVector<BamAlignmentWrapper>& aligns = track_data_->getAlignments();

	int ref_start =  track_data_->getRefSeqStart();

	//store coverage
	for (const auto& al : aligns)
	{
		if (!fast_settings_.show_secondary_alignments && al.isSecondaryAlignment()) continue;
		if (al.mappingQuality() < fast_settings_.min_mq) continue;

		if ((fast_settings_.show_soft_clipped_bases ? al.endWithSoftClip() : al.end()) < region.start()) continue;
		if ((fast_settings_.show_soft_clipped_bases ? al.startWithSoftClip() : al.start()) > region.end()) continue;

		foreach (const auto& data, al.getEvents())
		{
			int base_idx = data.genome_pos - region.start();
			if (data.event == BamAlignmentWrapper::MATCH || (fast_settings_.show_soft_clipped_bases && data.event == BamAlignmentWrapper::SOFT_CLIP))
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
						case 'N': case 'n': ++cov.forward_n; break;
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
						case 'N': case 'n': ++cov.reverse_n; break;
						}
					}
					max_coverage_ = std::max(max_coverage_, cov.total());
				}
			}
			else if (data.event == BamAlignmentWrapper::INSERTION)
			{
				if (base_idx >= 0 && base_idx < coverage_.length()) coverage_[base_idx].insertions++;
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
		if (total_count == 0) cov.is_variant = false;
		else cov.is_variant = (((double)base_count / total_count) < (1.0 - fast_settings_.min_display_af_for_snp));
	}
}

void BamCoverageTrack::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.fillRect(rect(), Qt::white);
	const BedLine& region = SharedData::region();
	drawLabel(painter);
	if (region.length() > fast_settings_.visibility_range) drawZoomInText(painter);
	else
	{
		drawCoverage(painter);
		drawCurrentMaxText(painter);
	}
}

void BamCoverageTrack::drawZoomInText(QPainter& painter)
{
	painter.setPen(Qt::gray);
	painter.drawText(rect(), Qt::AlignCenter, "Zoom In To See Coverage");
}

void BamCoverageTrack::drawCurrentMaxText(QPainter& painter)
{
	Viewport viewport = getViewport();

	painter.setPen(Qt::black);
	QRect rec(viewport.x0, 0, width(), height());
	painter.drawText(rec, Qt::AlignLeft, "[0," + QString::number(max_coverage_) + "]");
}

void BamCoverageTrack::drawCoverage(QPainter& painter)
{
	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();

	int draw_height = height();

	//White borders need enough room to leave a visible bar interior.
	const bool draw_base_borders = viewport.pixels_per_base >= 3.0f;
	if (draw_base_borders) painter.setPen(Qt::white);
	else painter.setPen(Qt::gray);


	// first pass for normal
	for (int idx =0; idx < region.length(); ++idx)
	{
		const BaseCoverage& cov = coverage_[idx];
		int total_count = cov.total();
		if (total_count ==0) continue;

		int pX = viewport.genomePosToScreen(region.start() + idx);
		int endX = viewport.genomePosToScreen(region.start() + idx + 1);
		int dX = std::max(1, endX - pX);
		float norm = (float)total_count/max_coverage_;
		int bar_h = norm * draw_height;

		if (!cov.is_variant)
		{
			painter.setBrush(Qt::gray);
			painter.drawRect(pX, draw_height - bar_h, dX, bar_h);
		}
	}

	// second pass for variants s.t. they are always drawn on top
	for (int idx =0; idx < region.length(); ++idx)
	{
		const BaseCoverage& cov = coverage_[idx];
		int total_count = cov.total();
		if (total_count ==0) continue;

		int pX = viewport.genomePosToScreen(region.start() + idx);
		int endX = viewport.genomePosToScreen(region.start() + idx + 1);
		int dX = std::max(1, endX - pX);
		float norm = (float)total_count/max_coverage_;
		int bar_h = norm * draw_height;

		if (cov.is_variant)
		{
			QPen pen = painter.pen();
			int a_height = bar_h * ((float)cov.a() / total_count);
			int c_height = bar_h * ((float)cov.c() / total_count);
			int g_height = bar_h * ((float)cov.g() / total_count);
			int t_height = bar_h * ((float)cov.t() / total_count);

			int y_start = draw_height - a_height;
			if (!draw_base_borders) painter.setPen(QColor(0, 150, 0));
			painter.setBrush(QColor(0, 150, 0)); //a
			painter.drawRect(pX, y_start, dX, a_height);

			y_start -= c_height;
			if (!draw_base_borders) painter.setPen(Qt::blue);
			painter.setBrush(Qt::blue); //c
			painter.drawRect(pX, y_start, dX, c_height);

			y_start -= g_height;
			if (!draw_base_borders) painter.setPen(QColor(209, 113, 5));
			painter.setBrush(QColor(209, 113, 5)); //g
			painter.drawRect(pX, y_start, dX, g_height);

			y_start -= t_height;
			if (!draw_base_borders) painter.setPen(Qt::red);
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

	QString info = QString("Pos: %1:%2\nDepth: %3\n")
					   .arg(region.chr().str())
					   .arg(coverage_pos)
					   .arg(cov.total());

	// Helper struct
	struct BaseInfo {
		char base;
		int count;
		int fwd;
		int rev;
	};

	BaseInfo bases[] = {
		{'A', cov.a(), cov.forward_a, cov.reverse_a},
		{'C', cov.c(), cov.forward_c, cov.reverse_c},
		{'G', cov.g(), cov.forward_g, cov.reverse_g},
		{'T', cov.t(), cov.forward_t, cov.reverse_t},
		{'N', cov.n(), cov.forward_n, cov.reverse_n}
	};

	for (const auto& b : bases)
	{
		int total = cov.total();
		int percent = (total > 0) ? static_cast<int>(100.0 * b.count / total) : 0;
		info += QString("%1: %2 (%3%, %4+, %5-)\n")
					.arg(b.base)
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
	const Viewport& viewport = getViewport();
	if (viewport.isOutOfDrawRegion(local_pos.x())) return;

	const BedLine& region = SharedData::region();
	int x = viewport.screenXToGenomePos(local_pos.x()) - region.start();
	if (x < 0 || x >= coverage_.length()) return;

	QString info = getCoverageText(coverage_[x], x);
	showInfoPopup(global_pos, info);
}

void BamCoverageTrack::mousePressEvent(QMouseEvent* event)
{
	mouse_press_pos_ = event->pos();
	TrackWidget::mousePressEvent(event);
}

bool BamCoverageTrack::isCurrentRegionValid()
{
	const BedLine& region = SharedData::region();
	return (region.length() <= fast_settings_.visibility_range);
}

void BamCoverageTrack::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
	{
		TrackWidget::mouseReleaseEvent(event);
		return;
	}
	bool dragging =	((event->pos() - mouse_press_pos_).manhattanLength() >= QApplication::startDragDistance());

	if (!dragging && isCurrentRegionValid())
	{
		handlePopupRequest(event->pos(), event->globalPosition());
	}

	TrackWidget::mouseReleaseEvent(event);
}
