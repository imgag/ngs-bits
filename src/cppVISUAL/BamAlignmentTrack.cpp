#include "BamAlignmentTrack.h"
#include "BamTrackDataManager.h"
#include "SharedData.h"

#include <QApplication>
#include <QPainter>
#include <QMenu>
#include "ChromosomeColors.h"

// #define DRAW_TRANSPARENT
// #define ENABLE_ANTIALIASING

// un comment this for FASTER processing (albeit at the cost of some quirky row assignments)
// #define CACHE_ROW_ASSIGNMENTS


//constants
static constexpr int ROW_HEIGHT = 12;
static constexpr int ROW_PADDING = 2;
static constexpr int SPACING_BELOW = 4;
static constexpr int MAX_QUALITY = 41;

BamAlignmentTrack::BamAlignmentTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name, type())
{
	connect(&settings_, &ParameterList::parameterChanged, this, [this]()
	{
		if (track_data_) track_data_->setVisibilityRange(settings_.getInt("visibility range"));
		if (track_data_ && isCurrentRegionValid())
		{
			const DisplaySettings options = displaySettings();
			makePairs(options);
			calculateRows(options);
		}
		updateGeometry();
		update();
	});
	initializeSettings();
	connect(SharedData::instance(), &SharedData::settingsChanged, this, &TrackWidget::regionChanged);
	setMouseTracking(true);
	updateFontCache();
}

QList<Parameter> BamAlignmentTrack::getParameters() const
{
	return {
		Parameter("view as pairs", "Connect read-pairs with a line.", ParameterType::BOOL, {}),
		Parameter("show all bases", "Display all bases, not only ones that are different from the reference base.", ParameterType::BOOL, {}),
		Parameter("show soft-clipped bases", "Display soft-clipped bases.", ParameterType::BOOL, {}),
		Parameter("show secondary alignments", "Display secondary alignments.", ParameterType::BOOL, {}),
		Parameter("group by", "Group reads by haplotype or mate chromosome.", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, QByteArray("none\tphase\tmate chromosome")}}),
		Parameter("color by", "Alignment coloring scheme.", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, QByteArray("none\tinsert size\tread strand")}}),
		Parameter("visibility range", "Maximum genomic region for which content is displayed (in bases).", ParameterType::INT, {{ConstraintType::MIN, 1}})
	};
}

QHash<QByteArray, QVariant> BamAlignmentTrack::getParameterDefaults() const
{
	return {{"visibility range", SharedData::settings().bam_max_region_len}, {"view as pairs", true}, {"show all bases", false}, {"show soft-clipped bases", false}, {"show secondary alignments", true}, {"group by", QByteArray("none")}, {"color by", QByteArray("insert size")}};
}

BamAlignmentTrack::~BamAlignmentTrack()
{
}

BamAlignmentTrack* BamAlignmentTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
	QSharedPointer<BamTrackData> data = BamTrackDataManager::getOrCreate(file_path);
	if (data)
	{
		auto track = new BamAlignmentTrack(parent, file_path, name);
		track->setTrackData(data);
		return track;
	}

	else return nullptr;
}

void BamAlignmentTrack::setTrackData(QSharedPointer<BamTrackData> track_data)
{
	track_data_ = track_data;
	connect(track_data_.get(), SIGNAL(onDataUpdate()), this, SLOT(dataReady()));
	connect(track_data_.get(), SIGNAL(onFullLoad()), this, SLOT(fullLoad()));
	track_data_->bindVisibilityRange(settings_);
	dataReady();
}

void BamAlignmentTrack::fullLoad()
{
	row_idxes_.clear();
	pair_row_idxes_.clear();
	row_stored_with_pair_.clear();

	row_idxes_.squeeze();
	pair_row_idxes_.squeeze();
	row_stored_with_pair_.squeeze();
}

bool BamAlignmentTrack::isCurrentRegionValid()
{
	const BedLine& region = SharedData::region();
	const int max_region_len = settings_.getInt("visibility range");
	return (region.length() <= max_region_len);
}

void BamAlignmentTrack::dataReady()
{
	if (isCurrentRegionValid())
	{
		const DisplaySettings options = displaySettings();
		makePairs(options);
		calculateRows(options);
	}
	updateGeometry();
	update();
}

BamAlignmentTrack::DisplaySettings BamAlignmentTrack::displaySettings() const
{
	return {settings_.getBool("view as pairs"), settings_.getBool("show all bases"),
		settings_.getBool("show soft-clipped bases"), settings_.getBool("show secondary alignments"),
		settings_.getString("group by"), settings_.getString("color by")};
}

void BamAlignmentTrack::calculateRows(const DisplaySettings& options)
{
	read_groups_.clear();
	if (options.group_by != "none")
	{
		calculateGroupedRows(options);
		return;
	}
	const bool view_as_pairs = options.view_as_pairs;
	if (view_as_pairs) calculateRowsPairMode();
	else calculateRowsNormalMode(options);
}

int BamAlignmentTrack::getAlignmentStart(const BamAlignmentWrapper& al, bool show_soft_clip_bases)
{
	if (show_soft_clip_bases) return al.startWithSoftClip();
	return al.start();
}

int BamAlignmentTrack::getAlignmentEnd(const BamAlignmentWrapper& al, bool show_soft_clip_bases)
{
	if (show_soft_clip_bases) return al.endWithSoftClip();
	return al.end();
}

bool BamAlignmentTrack::isAlignmentVisible(const BamAlignmentWrapper& alignment, bool show_secondary_alignments)
{
	return show_secondary_alignments || !alignment.isSecondaryAlignment();
}

void BamAlignmentTrack::calculateRowsNormalMode(const DisplaySettings& options)
{
	const QVector<BamAlignmentWrapper>& alns = track_data_->getAlignments();

	row_packer_.clear(); // this keeps the row_packer small so insertions don't take a long time in the future

	#ifdef CACHE_ROW_ASSIGNMENTS

	QSet<int> restored;

	for (int i =0; i < alns.size(); ++i)
	{
		const BamAlignmentWrapper& w = alns[i];
		if (!isAlignmentVisible(w, options.show_secondary_alignments)) continue;
		const auto& aln_id = w.id;
		if (row_idxes_.contains(aln_id) && row_packer_.canRestore(row_idxes_[aln_id],
																  getAlignmentStart(w, options.show_soft_clip_bases),
																  getAlignmentEnd(w, options.show_soft_clip_bases)))
		{
			row_packer_.restore(row_idxes_[aln_id], getAlignmentStart(w, options.show_soft_clip_bases) ,
								getAlignmentEnd(w, options.show_soft_clip_bases), i);
			restored.insert(i);
		}
	}
	#else
	row_idxes_.clear();
	#endif

	for (int i =0; i < alns.size(); ++i)
	{
		if (!isAlignmentVisible(alns[i], options.show_secondary_alignments)) continue;
		#ifdef CACHE_ROW_ASSIGNMENTS
		if (restored.contains(i)) continue;
		#endif
		int row = row_packer_.insert(getAlignmentStart(alns[i], options.show_soft_clip_bases), getAlignmentEnd(alns[i], options.show_soft_clip_bases), i);
		row_idxes_[alns[i].id] = row;
	}

	// if (num_rows_ < row_packer_.rowCount()) num_rows_ = row_packer_.rowCount();
	num_rows_ = std::max(ROW_HEIGHT, row_packer_.rowCount());
}

void BamAlignmentTrack::calculateRowsPairMode()
{
	const QVector<BamAlignmentWrapper>& alns = track_data_->getAlignments();

	row_packer_.clear();
	// restore the ones we already have (only the pairs)
	#ifdef CACHE_ROW_ASSIGNMENTS

	QSet<int> restored;

	for (int i =0; i < read_pairs_.count(); ++i)
	{
		const auto& read_pair = read_pairs_[i];
		const auto& al = alns[read_pair.first];
		int row = pair_row_idxes_.value(al.id, -1);
		if (row != -1 && row_stored_with_pair_.value(al.id, false) &&
			row_packer_.canRestore(row, read_pair.start, read_pair.end))
		{
			row_packer_.restore(row, read_pair.start, read_pair.end, i);
			restored.insert(i);
		}
	}
	#endif

	// Insert new rows for pairs if necessary
	for (int i =0; i < read_pairs_.count(); ++i)
	{
		const ReadPair& read_pair = read_pairs_[i];
		const auto& al1 = alns[read_pair.first];

		#ifdef CACHE_ROW_ASSIGNMENTS
		if (restored.contains(i)) continue;
		#endif

		int row = row_packer_.insert(read_pair.start, read_pair.end, i);

		if (read_pair.first != -1) pair_row_idxes_[al1.id] = row;
		if (read_pair.second != -1)
		{
			pair_row_idxes_[al1.id] = row;
			row_stored_with_pair_[al1.id] = true;
		}
	}
	num_rows_ = std::max(1, row_packer_.rowCount());
}

void BamAlignmentTrack::calculateGroupedRows(const DisplaySettings& options)
{
	const auto& alns = track_data_->getAlignments();
	const bool pairs = options.view_as_pairs;
	row_packer_.clear();
	row_idxes_.clear();
	pair_row_idxes_.clear();
	row_stored_with_pair_.clear();

	const bool by_phase = options.group_by == "phase";
	const QStringList labels = by_phase ? QStringList{"phase 1", "phase 2", "unphased"} : QStringList{"differing", "same"};
	const Chromosome& chromosome = SharedData::region().chr();
	const auto differing_mate = [&chromosome](const BamAlignmentWrapper& alignment)
	{
		// Reads without a mate chromosome stay in the default (same) group.
		return alignment.mate_chr.isValid() && alignment.mate_chr != chromosome;
	};
	for (int group_index = 0; group_index < labels.count(); ++group_index)
	{
		RowPacker group;
		// Reserve a label row, plus a separator row between non-empty groups.
		const int offset = row_packer_.rowCount() == 0 ? 1 : row_packer_.rowCount() + 2;
		const int count = pairs ? read_pairs_.count() : alns.count();
		for (int i = 0; i < count; ++i)
		{
			const auto& al = alns[pairs ? read_pairs_[i].first : i];
			if (!isAlignmentVisible(al, options.show_secondary_alignments)) continue;
			int read_group;
			if (by_phase)
			{
				int read_phase = al.phase();
				if (pairs && read_pairs_[i].second != -1)
				{
					const int mate_phase = alns[read_pairs_[i].second].phase();
					// Use the assigned mate when the other is unassigned; conflicting mates are unassigned.
					if (read_phase == 0) read_phase = mate_phase;
					else if (mate_phase != 0 && mate_phase != read_phase) read_phase = 0;
				}
				read_group = read_phase == 1 ? 0 : read_phase == 2 ? 1 : 2;
			}
			else
			{
				bool differing = differing_mate(al);
				if (pairs && read_pairs_[i].second != -1) differing |= differing_mate(alns[read_pairs_[i].second]);
				read_group = differing ? 0 : 1;
			}
			if (read_group != group_index) continue;
			const int start = pairs ? read_pairs_[i].start : getAlignmentStart(al, options.show_soft_clip_bases);
			const int end = pairs ? read_pairs_[i].end : getAlignmentEnd(al, options.show_soft_clip_bases);
			const int row = offset + group.insert(start, end, i);
			row_packer_.restore(row, start, end, i);
			if (pairs)
			{
				pair_row_idxes_[al.id] = row;
				row_stored_with_pair_[al.id] = read_pairs_[i].second != -1;
			}
			else row_idxes_[al.id] = row;
		}
		if (group.rowCount() > 0) read_groups_.append({labels[group_index], offset});
	}
	num_rows_ = std::max(1, row_packer_.rowCount());
}

QSize BamAlignmentTrack::sizeHint() const
{
	return QSize(parentWidget() ? parentWidget()->width() : 200,
				 num_rows_ * (ROW_HEIGHT * ROW_PADDING) + SPACING_BELOW);
}

void BamAlignmentTrack::reloadTrack()
{
	BamTrackDataManager::reload(file_path_);
}

void BamAlignmentTrack::paintEvent(QPaintEvent*)
{
	const DisplaySettings options = displaySettings();
	QPainter painter(this);

	#ifdef ENABLE_ANTIALIASING
	painter.setRenderHint(QPainter::Antialiasing);
	#endif

	painter.fillRect(rect(), Qt::white);
	drawLabel(painter);
	const BedLine& region = SharedData::region();
	const int max_region_len = settings_.getInt("visibility range");
	if (region.length() > max_region_len) drawZoomInText(painter);
	else
	{
		if (options.view_as_pairs) drawPairMode(painter, region, options);
		else drawNormalMode(painter, options);
		if (options.group_by != "none") drawGroups(painter);
	}
}

void BamAlignmentTrack::drawGroups(QPainter& painter)
{
	painter.save();
	const Viewport viewport = getViewport();
	const QRectF plot(viewport.x0, 0, viewport.usable_width, height());
	painter.setClipRect(plot);
	QFont label_font = painter.font();
	label_font.setPixelSize(10);
	painter.setFont(label_font);
	const QFontMetrics metrics(label_font);
	const QPen separator(Qt::gray, 1, Qt::DashLine);
	for (int i = 0; i < read_groups_.count(); ++i)
	{
		const ReadGroup& group = read_groups_[i];
		const int y = (group.first_row - 1) * (ROW_HEIGHT + ROW_PADDING);
		if (i > 0)
		{
			painter.setPen(separator);
			painter.drawLine(QPointF(plot.left(), y - (ROW_HEIGHT + ROW_PADDING) / 2),
				QPointF(plot.right(), y - (ROW_HEIGHT + ROW_PADDING) / 2));
		}
		const QString& label = group.label;
		const QRectF box(plot.left(), y, metrics.horizontalAdvance(label) + 8, ROW_HEIGHT + ROW_PADDING - 1);
		painter.setPen(Qt::gray);
		painter.setBrush(Qt::white);
		painter.drawRect(box);
		painter.setPen(Qt::black);
		painter.drawText(box, Qt::AlignCenter, label);
	}
	painter.restore();
}

void BamAlignmentTrack::drawAlignmentAndMismatches(QPainter& painter, const BamAlignmentWrapper& al, int row_y, const DisplaySettings& options)
{
	if (row_y < 0)
	{
		qDebug() << __FILE__ << __LINE__ << ": Bug: draw received negative row_y" << Qt::endl;
		return;
	}

	const BedLine& region = SharedData::region();

	if (getAlignmentEnd(al, options.show_soft_clip_bases) < region.start() ||
		getAlignmentStart(al, options.show_soft_clip_bases) > region.end()) return;

	drawAlignment(painter, al, row_y, options);
	if (!options.show_all_bases) drawMismatches(painter, al, row_y);
	drawAllBases(painter, al, row_y, options);
}

void BamAlignmentTrack::drawNormalMode(QPainter& painter, const DisplaySettings& options)
{
	const QVector<BamAlignmentWrapper>& alns = track_data_->getAlignments();
	for (int i =0; i < alns.size(); ++i)
	{
		const BamAlignmentWrapper& al_w = alns[i];
		if (!isAlignmentVisible(al_w, options.show_secondary_alignments)) continue;
		int row_y = row_idxes_.value(alns[i].id, -1) * (ROW_HEIGHT + ROW_PADDING);
		drawAlignmentAndMismatches(painter, al_w, row_y, options);
	}
}

void BamAlignmentTrack::drawPairMode(QPainter& painter, const BedLine& region, const DisplaySettings& options)
{
	const QVector<BamAlignmentWrapper>& alns = track_data_->getAlignments();
	const Viewport& viewport = getViewport();

	foreach (const ReadPair& read_pair, read_pairs_)
	{
		const int row_y = pair_row_idxes_.value(alns[read_pair.first].id, -1) * (ROW_HEIGHT + ROW_PADDING);
		if (read_pair.first != -1)
		{
			const BamAlignmentWrapper& al_w = alns[read_pair.first];
			drawAlignmentAndMismatches(painter, al_w, row_y, options);
		}

		if (read_pair.second != -1)
		{
			const BamAlignmentWrapper& al_w = alns[read_pair.second];
			drawAlignmentAndMismatches(painter, al_w, row_y, options);
		}

		//draw line
		if(read_pair.first != -1 && read_pair.second != -1)
		{
			const BamAlignmentWrapper& al1 = alns[read_pair.first];
			const BamAlignmentWrapper& al2 = alns[read_pair.second];

			int st = std::clamp(getAlignmentEnd(al1, options.show_soft_clip_bases) + 1, region.start(), region.end() + 1);
			int en = std::clamp(getAlignmentStart(al2, options.show_soft_clip_bases), region.start(), region.end() + 1);

			float p0 = viewport.genomePosToScreen(st);
			float p1 = viewport.genomePosToScreen(en);

			painter.setPen(Qt::gray);
			painter.setBrush(Qt::gray);

			painter.drawLine(p0, row_y + ROW_HEIGHT / 2.0f, p1, row_y + ROW_HEIGHT / 2.0f);
		}
	}
}

void BamAlignmentTrack::drawZoomInText(QPainter& painter)
{
	QFont font = painter.font();
	font.setPointSize(8);
	painter.setFont(font);
	QPen pen = painter.pen(); // store pen
	painter.setPen(Qt::black);
	painter.drawText(rect(), Qt::AlignHCenter, "Zoom In To See Aligments");
	painter.setPen(pen); // restore pen
}

QColor BamAlignmentTrack::strandColor(bool is_reversed)
{
	return is_reversed ? QColor(175, 175, 235, 200) : QColor(235, 175, 175, 200);
}

QColor BamAlignmentTrack::insertSizeColor(const BamAlignmentWrapper& al_w)
{
	Chromosome mate_chr = al_w.mate_chr;
	// Missing mates are unexpected in paired-end files, but normal for long reads.
	if (!al_w.isPaired() || !mate_chr.isValid()) return track_data_->isPairedEnd() ? QColor(0, 0, 0) : QColor(202, 202, 202, 150);
	if (!al_w.isMateMapped()) return QColor(202, 202, 202, 150);

	const BedLine& region = SharedData::region();
	if (mate_chr == region.chr())
	{
		InsertSizeStats i_stats = track_data_->getInsertSizeStats();
		int i_size = std::abs(al_w.insertSize());
		if (i_size > i_stats.insert_size_max) return QColor(200, 0, 0);
		if (i_size < i_stats.insert_size_min) return QColor(0, 0, 150);
		return QColor(202, 202, 202, 150); //gray
	}
	return ChromosomeColors::get(mate_chr.str());
}

QColor BamAlignmentTrack::getAlignmentColor(const BamAlignmentWrapper& al_w, const QByteArray& coloring_scheme)
{
	//selected always takes priority
	if (!selected_name_.isEmpty() && al_w.name() == selected_name_) return QColor(255, 100, 0);

	if (coloring_scheme == "read strand") return strandColor(al_w.isReverseStrand());

	else if (coloring_scheme == "insert size") return insertSizeColor(al_w);

	return QColor(202, 202, 202, 150); //gray
}

void BamAlignmentTrack::drawAlignment(QPainter& painter, const BamAlignmentWrapper& al_w, int row_y, const DisplaySettings& options)
{
	const BedLine& region = SharedData::region();
	const bool show_soft_clip_bases = options.show_soft_clip_bases;
	Viewport viewport = getViewport();
	int last_x = -1.0f;

	QColor color = getAlignmentColor(al_w, options.coloring_scheme);

	// first pass for matches and deletions
	foreach (const auto& data, al_w.getEvents())
	{
		int st = std::max(data.genome_pos, region.start());
		int en = std::min(data.genome_pos + data.length - 1 , region.end() + 1);

		if (en <= region.start()) continue;

		int x_start = viewport.genomePosToScreen(st);
		int width = viewport.genomeWidthToScreen(en - st + 1);

		if (width < 0) continue;

		if (data.event == BamAlignmentWrapper::MATCH || (show_soft_clip_bases && data.event == BamAlignmentWrapper::SOFT_CLIP))
		{
			QRectF match_rect(x_start, row_y, width, ROW_HEIGHT);
			last_x = x_start + width;
			#ifdef DRAW_TRANSPARENT
			painter.setPen(color);
			painter.setBrush(Qt::NoBrush);
			#else
			painter.setPen(Qt::NoPen);
			painter.setBrush(color);
			#endif
			painter.drawRect(match_rect);
		}

		else if (data.event == BamAlignmentWrapper::DELETION)
		{
			painter.setPen(Qt::blue);
			int mid = row_y + ROW_HEIGHT / 2;
			painter.drawLine(x_start, mid, x_start + width, mid);
			if (viewport.pixels_per_base >= cached_char_size_.width())
			{
				QRectF text_rect(x_start, row_y, width, ROW_HEIGHT);
				painter.setPen(Qt::darkCyan);
				painter.setFont(cached_font_);
				painter.drawText(text_rect, Qt::AlignCenter, QString::number(data.length));
			}
		}
	}

	// second pass for insertions so that they are on top
	foreach (const auto& data, al_w.getEvents())
	{
		if (data.event == BamAlignmentWrapper::INSERTION)
		{
			int st = std::max(data.genome_pos, region.start());
			int en = std::min(data.genome_pos + data.length - 1 , region.end() + 1);

			if (en <= region.start()) continue;

			float x_start = viewport.genomePosToScreen(st);
			float width = viewport.genomeWidthToScreen(en - st + 1);

			if (width < 0) continue;

			float marker_w = std::clamp(
				(float)(viewport.pixels_per_base * 0.5f),
				4.0f,   // minimum
				12.0f   // maximum before expansion mode
				);
			QRectF rect(x_start - marker_w / 2.0f, row_y, marker_w, ROW_HEIGHT);
			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor(140, 0, 200));
			painter.drawRect(rect);

			if (viewport.pixels_per_base >= cached_char_size_.width())
			{
				painter.setPen(Qt::white);
				painter.setFont(cached_font_);

				QRectF text_rect(
					x_start - marker_w/2.0f,
					row_y,
					marker_w,
					ROW_HEIGHT
					);

				painter.drawText(
					text_rect,
					Qt::AlignCenter,
					QString::number(data.length)
					);
			}
		}
	}

	//draw the arrows
	int st = std::max(getAlignmentStart(al_w, options.show_soft_clip_bases), region.start());
	int en = std::min(getAlignmentEnd(al_w, options.show_soft_clip_bases), region.end());
	int x_start = viewport.genomePosToScreen(st);
	int width = viewport.genomeWidthToScreen(en - st + 1);

	painter.setPen(Qt::NoPen);
	painter.setBrush(color);

	int mid = row_y + ROW_HEIGHT / 2;
	// int tri_w = std::max(1, ROW_HEIGHT / 2);

	int tri_w = std::clamp(ROW_HEIGHT / 2, 1, std::max(1, width / 15));

	if (!al_w.isReverseStrand() && last_x > 0)
	{
		QPolygon arrow;
		arrow << QPoint(last_x + tri_w, mid)
		<< QPoint(last_x, row_y)
		<< QPoint(last_x, row_y + ROW_HEIGHT);
		painter.drawPolygon(arrow);
	}
	else if (!viewport.isOutOfDrawRegion(x_start - tri_w))
	{
		QPolygon arrow;
		arrow << QPoint(x_start - tri_w, mid)
		<< QPoint(x_start, row_y)
		<< QPoint(x_start, row_y + ROW_HEIGHT);
		painter.drawPolygon(arrow);
	}

}

QColor BamAlignmentTrack::baseColor(QChar base)
{
	if (base=='A' || base=='a') return QColor(0, 150, 0);
	if (base=='C' || base=='c') return QColor(0, 0, 255);
	if (base=='G' || base=='g') return QColor(209, 113, 5);
	if (base=='T' || base=='t') return QColor(255, 0, 0);
	if (base=='N' || base=='n') return QColor(128, 128, 128);

	return Qt::black;
}

QSize BamAlignmentTrack::characterSize(QFont font)
{
	QFontMetrics fm(font);

	int w = -1;
	w = std::max(w, fm.boundingRect("A").width());
	w = std::max(w, fm.boundingRect("C").width());
	w = std::max(w, fm.boundingRect("G").width());
	w = std::max(w, fm.boundingRect("T").width());
	w = std::max(w, fm.boundingRect("N").width());

	int h = -1;
	h = std::max(h, fm.boundingRect("C").height());
	h = std::max(h, fm.boundingRect("A").height());
	h = std::max(h, fm.boundingRect("G").height());
	h = std::max(h, fm.boundingRect("T").height());
	h = std::max(h, fm.boundingRect("N").height());

	return QSize(w, h);
}

void BamAlignmentTrack::updateFontCache()
{
	QFont font;
	font.setPixelSize(ROW_HEIGHT);
	font.setBold(true);

	cached_font_ = font;
	cached_char_size_ = characterSize(font);
}

void BamAlignmentTrack::drawBase(QPainter& painter, const Viewport& viewport, int genome_pos, char base, int qual, int row_y)
{
	int x_start = viewport.genomePosToScreen(genome_pos);
	int end_x = viewport.genomePosToScreen(genome_pos + 1);

	if (viewport.isOutOfDrawRegion(x_start) || viewport.isOutOfDrawRegion(end_x)) return;

	int dX = std::max(1, end_x - x_start);

	QColor color = baseColor(base);
	color = QColor(color.red(), color.green(), color.blue(), ((float)qual/MAX_QUALITY)*255);

	if (viewport.pixels_per_base >= cached_char_size_.width())
	{
		painter.setFont(cached_font_);
		QRectF text_rect(x_start, row_y - 2, dX, ROW_HEIGHT + 2);
		painter.setPen(color);
		painter.drawText(text_rect, Qt::AlignHCenter, QString(base).toUpper());
	}
	else
	{
		painter.setPen(Qt::NoPen);
		painter.setBrush(color);
		QRectF rect(x_start, row_y, dX, ROW_HEIGHT);
		painter.drawRect(rect);
	}
}

void BamAlignmentTrack::drawMismatches(QPainter& painter, const BamAlignmentWrapper& al, int row_y)
{
	Viewport viewport = getViewport();

	const auto& mismatches = al.getMismatches();
	foreach (const auto& mismatch_data, mismatches)
	{
		drawBase(painter, viewport, mismatch_data.genomic_pos, mismatch_data.base, mismatch_data.quality, row_y);
	}
}

void BamAlignmentTrack::drawAllBases(QPainter& painter, const BamAlignmentWrapper& al, int row_y, const DisplaySettings& options)
{
	const bool show_all_bases = options.show_all_bases;
	const bool show_soft_clip_bases = options.show_soft_clip_bases;
	const Viewport& viewport = getViewport();
	foreach (const auto& event_data, al.getEvents())
	{
		if ((show_all_bases && event_data.event == BamAlignmentWrapper::MATCH) ||
			(show_soft_clip_bases && event_data.event == BamAlignmentWrapper::SOFT_CLIP))
		{
			for (int i =0; i < event_data.length; ++i)
			{	if (i >= event_data.bases.length() || i >= event_data.qualities.length())
				{
					qDebug() << __FILE__ << __LINE__ << "Bug: data.length does not match bases or qualities length";
					continue;
				}
				drawBase(painter, viewport, event_data.genome_pos + i, event_data.bases[i], event_data.qualities[i], row_y);
			}
		}
	}
}

void BamAlignmentTrack::makePairs(const DisplaySettings& options)
{
	read_pairs_.clear();

	QHash<QString, int> pending;

	const auto& alns = track_data_->getAlignments();

	for (int i =0; i < alns.count(); ++i)
	{
		const BamAlignmentWrapper& al = alns[i];
		if (!isAlignmentVisible(al, options.show_secondary_alignments)) continue;
		QString name = al.name();
		if (!al.isSecondaryAlignment() && pending.contains(name))
		{
			int j = pending.take(name);

			read_pairs_[j].second = i;
			read_pairs_[j].start = std::min(read_pairs_[j].start, getAlignmentStart(al, options.show_soft_clip_bases));
			read_pairs_[j].end = std::max(read_pairs_[j].end, getAlignmentEnd(al, options.show_soft_clip_bases));
		}
		else
		{
			ReadPair p;
			p.first = i;
			p.second = -1;
			p.start = getAlignmentStart(al, options.show_soft_clip_bases);
			p.end = getAlignmentEnd(al, options.show_soft_clip_bases);
			// Secondary mappings remain separate from the primary read pair.
			if (!al.isSecondaryAlignment()) pending[name] = read_pairs_.count();
			read_pairs_.append(p);
		}
	}
}

void BamAlignmentTrack::addAlignmentOptionsToCtxtMenu(QMenu& menu, const QPoint& local_pos)
{
	const bool view_as_pairs = settings_.getBool("view as pairs");
	int aln_idx = getAlnIndexFromLocalPos(local_pos);
	// add go_to_mate action, select/deselect action
	if (aln_idx != -1)
	{
		QAction* go_to_mate_action = menu.addAction("Go to mate");
		const auto& alns = track_data_->getAlignments();

		Chromosome mate_chr;
		int mate_start;

		if (view_as_pairs)
		{
			// check which pair was clicked
			const ReadPair& rp = read_pairs_[aln_idx];
			aln_idx = rp.first;
			mate_chr = alns[aln_idx].mate_chr;
			mate_start = alns[aln_idx].mateStart();

			// if the other part in the pair does not exist, only the first one was picked

			if (rp.second > 0)
			{
				// check if rp.second was clicked
				const BamAlignmentWrapper& aln = alns[rp.second];
				const Viewport& viewport = getViewport();
				int g_pos = viewport.screenXToGenomePos(local_pos.x());
				if (g_pos >= aln.start() && g_pos <= aln.end())
				{
					//set mate_chr, mate_start to rp.first's corresponding properties
					mate_chr = aln.mate_chr;
					mate_start = aln.mateStart();
				}
			}
		}
		else //non-pair mode
		{
			mate_chr = alns[aln_idx].mate_chr;
			mate_start = alns[aln_idx].mateStart();
		}

		go_to_mate_action->setEnabled(mate_chr.isValid());

		if (mate_chr.isValid())
		{
			connect(go_to_mate_action, &QAction::triggered, this, [mate_chr, mate_start](){
				SharedData::setRegion(mate_chr, mate_start - 100, mate_start + 100);
			});
		}

		QString name = alns[aln_idx].name();
		bool deselect = (!selected_name_.isEmpty() && name == selected_name_);
		QAction* select_action = menu.addAction(deselect ? "Deselect" : "Select");
		connect(select_action, &QAction::triggered, this, [this, deselect, name](){
			if (deselect) selected_name_ = "";
			else selected_name_ = name;
			update();
		});
	}
}


void BamAlignmentTrack::populateContextMenu(QMenu& menu, const QPoint& local_pos)
{
	if (isCurrentRegionValid())
	{
		addAlignmentOptionsToCtxtMenu(menu, local_pos);
	}
	TrackWidget::populateContextMenu(menu, local_pos);
}

void BamAlignmentTrack::mousePressEvent(QMouseEvent* event)
{
	mouse_press_pos_ = event->pos();
	TrackWidget::mousePressEvent(event);
}

int BamAlignmentTrack::getAlnIndexFromLocalPos(QPoint local_pos)
{
	const Viewport& viewport = getViewport();
	int y = local_pos.y();
	int row = y / (ROW_HEIGHT + ROW_PADDING);

	int x = local_pos.x();
	if (viewport.isOutOfDrawRegion(x)) return -1;

	int genome_pos = viewport.screenXToGenomePos(x);
	int aln_idx = row_packer_.find(row, genome_pos);
	return aln_idx;
}

QString BamAlignmentTrack::getBamAlignmentText(const BamAlignmentWrapper& al_w, int genome_pos, bool show_soft_clip_bases)
{
	// Using simple HTML tags for layout and styling
	QString text = QString("<b>%1</b><br/>"
						   "<b>Start:</b> %2 &nbsp; <b>End:</b> %3<br/>"
						   "<b>Strand:</b> %4<br/>")
					   .arg(al_w.name())
					   .arg(al_w.start())
					   .arg(al_w.end())
					   .arg(al_w.isReverseStrand() ? "Reverse" : "Forward");

	if (genome_pos < getAlignmentStart(al_w, show_soft_clip_bases) || genome_pos > getAlignmentEnd(al_w, show_soft_clip_bases))
	{
		return text;
	}

	//mate info
	if (al_w.isPaired())
	{
		text += "<hr>";
		text += QString("Mate is mapped = %1<br/>").arg(al_w.isMateMapped() ? "yes" : "no");
		text += QString("Mate start = %1:%2<br/>").arg(al_w.mate_chr.str(), QString::number(al_w.mateStart()));
		text += QString("Insert size = %1<br/>").arg(al_w.insertSize());
	}

	// base info
	const BedLine& region = SharedData::region();
	text += "<hr>";
	text += QString("<b>Location</b> = %1:  %2<br/>").arg(region.chr().str(), QString::number(genome_pos));

	foreach (const auto& event_data, al_w.getEvents())
	{
		// in the region defined by the cigar event
		if (genome_pos >= event_data.genome_pos && genome_pos < event_data.genome_pos + event_data.length)
		{
			if (event_data.event == BamAlignmentWrapper::MATCH)
			{
				char base = event_data.bases[genome_pos - event_data.genome_pos];
				int qual = event_data.qualities[genome_pos - event_data.genome_pos];

				text += QString("<b>Base:</b> <span style='color:green;'>%1</span> @ QV %2<br/>")
							.arg(QString(base).toUpper(), QString::number(qual));
			}
			else if (event_data.event == BamAlignmentWrapper::SOFT_CLIP)
			{
				char base = event_data.bases[genome_pos - event_data.genome_pos];
				int qual = event_data.qualities[genome_pos - event_data.genome_pos];

				text += QString("<b>Soft Clip Base:</b> <span style='color:orange;'>%1</span> @ QV %2<br/>")
							.arg(QString(base).toUpper(), QString::number(qual));
			}
			else if (event_data.event == BamAlignmentWrapper::INSERTION && genome_pos == event_data.genome_pos)
			{
				text += QString("<b>Insertion:</b> <span style='color:blue;'>%1</span><br/>")
				.arg(event_data.bases.toUpper());
			}
			else if (event_data.event == BamAlignmentWrapper::DELETION)
			{
				text += QString("<b>Deletion:</b> <span style='color:red;'>%1 bp</span><br/>")
				.arg(event_data.length);
			}
		}
	}
	return text;
}

void BamAlignmentTrack::handlePopupRequest(QPoint local_pos, QPointF global_pos)
{
	const DisplaySettings options = displaySettings();
	const bool view_as_pairs = options.view_as_pairs;
	const Viewport& viewport = getViewport();
	int y = local_pos.y();
	int row = y / (ROW_HEIGHT + ROW_PADDING);

	int x = local_pos.x();
	if (viewport.isOutOfDrawRegion(x)) return;

	int genome_pos = viewport.screenXToGenomePos(x);
	int aln_idx = row_packer_.find(row, genome_pos);

	const auto& alns = track_data_->getAlignments();

	if (aln_idx != -1 && !alns.empty())
	{
		if (view_as_pairs)
		{
			const ReadPair& rp = read_pairs_[aln_idx];

			QString info;

			if (rp.first != -1)
			{
				info += QString("Read 1: %1").arg(getBamAlignmentText(alns[rp.first], genome_pos, options.show_soft_clip_bases));
			}

			if (rp.second != -1)
			{
				info += QString("Read 2: %1").arg(getBamAlignmentText(alns[rp.second], genome_pos, options.show_soft_clip_bases));
			}

			showInfoPopup(global_pos, info);
		}
		else if (!view_as_pairs)
		{
			QString info = QString("Read: %1").arg(getBamAlignmentText(alns[aln_idx], genome_pos, options.show_soft_clip_bases));
			showInfoPopup(global_pos, info);
		}
	}
}

void BamAlignmentTrack::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
	{
		TrackWidget::mouseReleaseEvent(event);
		return;
	}

	bool dragging = (event->pos() - mouse_press_pos_).manhattanLength() >= QApplication::startDragDistance();

	if (!dragging && isCurrentRegionValid())
	{
		handlePopupRequest(event->pos(), event->globalPosition());
	}

	TrackWidget::mouseReleaseEvent(event);
}

void BamAlignmentTrack::mouseMoveEvent(QMouseEvent *event)
{
	SharedData::mouseMoved(event->pos().x(), width());

	TrackWidget::mouseMoveEvent(event);
}
