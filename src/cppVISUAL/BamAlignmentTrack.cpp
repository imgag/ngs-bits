#include "BamAlignmentTrack.h"
#include "BamTrackDataManager.h"
#include "SharedData.h"

#include <QApplication>
#include <QPainter>
#include <QMenu>

// #define DRAW_TRANSPARENT

//constants
static constexpr int ROW_HEIGHT = 10;
static constexpr int ROW_PADDING = 2;
static constexpr int SPACING_BELOW = 4;

BamAlignmentTrack::BamAlignmentTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name)
{
	updateFontCache();
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

QMap<QString, QVariant> BamAlignmentTrack::getSettings()
{
	auto settings = TrackWidget::getSettings();
	settings["view_as_pairs"] = view_as_pairs_;
	return settings;
}

void BamAlignmentTrack::loadKeyValueFromXml(QString key, const QDomElement& item)
{
	if (key == "view_as_pairs") view_as_pairs_ = (item.attribute("value") == "true");
	if (view_as_pairs_)
	{
		calculateRows();
		updateGeometry();
		update();
	}
}

void BamAlignmentTrack::setTrackData(QSharedPointer<BamTrackData> track_data)
{
	track_data_ = track_data;
	connect(track_data_.get(), SIGNAL(onDataUpdate()), this, SLOT(dataReady()));
	dataReady();
}

void BamAlignmentTrack::dataReady()
{
	makePairs();
	calculateRows();
	updateGeometry();
	update();
}

void BamAlignmentTrack::calculateRows()
{
	if (view_as_pairs_) calculateRowsPairMode();
	else calculateRowsNormalMode();
}

void BamAlignmentTrack::calculateRowsNormalMode()
{
	const QVector<BamAlignmentWrapper>& alns = track_data_->getAlignments();

	row_packer_.clear(); // this keeps the row_packer small so insertions don't take a long time in the future

	QSet<int> restored;

	for (int i =0; i < alns.size(); ++i)
	{
		const BamAlignmentWrapper& w = alns[i];
		if (row_idxes_.contains(w) && row_packer_.canRestore(row_idxes_[w], w.alignment.start(), w.alignment.end()))
		{
			row_packer_.restore(row_idxes_[w], w.alignment.start(), w.alignment.end(), i);
			restored.insert(i);
		}
	}

	for (int i =0; i < alns.size(); ++i)
	{
		if (restored.contains(i)) continue;
		int row = row_packer_.insert(alns[i].alignment.start(), alns[i].alignment.end(), i);
		row_idxes_[alns[i]] = row;
	}

	if (num_rows_ < row_packer_.rowCount()) num_rows_ = row_packer_.rowCount();
}

void BamAlignmentTrack::calculateRowsPairMode()
{
	/*
	 * TODO: this has a bug, if there are no pairs
	 * then the unpaired strands are always assigned a new row
	 *
	 */
	const QVector<BamAlignmentWrapper>& alns = track_data_->getAlignments();

	row_packer_.clear();
	QSet<int> restored;
	// restore the ones we already have (only the pairs)
	for (int i =0; i < read_pairs_.count(); ++i)
	{
		const auto& read_pair = read_pairs_[i];
		const auto& al = alns[read_pair.first].alignment;
		int row = pair_row_idxes_.value(al.name(), -1);
		if (row != -1 && row_stored_with_pair_.value(al.name(), false) &&
			row_packer_.canRestore(row, read_pair.start, read_pair.end))
		{
			row_packer_.restore(row, read_pair.start, read_pair.end, i);
			restored.insert(i);
		}
	}

	// Insert new rows for pairs if necessary
	for (int i =0; i < read_pairs_.count(); ++i)
	{
		const ReadPair& read_pair = read_pairs_[i];
		const auto& al1 = alns[read_pair.first].alignment;

		if (restored.contains(i)) continue;

		int row = row_packer_.insert(read_pair.start, read_pair.end, i);

		if (read_pair.first != -1) pair_row_idxes_[al1.name()] = row;
		if (read_pair.second != -1)
		{
			pair_row_idxes_[al1.name()] = row;
			row_stored_with_pair_[al1.name()] = true;
		}
	}
	if (num_rows_ < row_packer_.rowCount()) num_rows_ = row_packer_.rowCount();
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
	QPainter painter(this);
	painter.fillRect(rect(), Qt::white);
	drawLabel(painter);
	const BedLine& region = SharedData::region();
	int max_region_len = SharedData::settings().bam_max_region_len;
	if (region.length() > max_region_len) drawZoomInText(painter);
	else
	{
		if (view_as_pairs_) drawPairMode(painter, region);
		else drawNormalMode(painter, region);
	}
}

void BamAlignmentTrack::drawAlignmentAndMismatches(QPainter& painter, const BamAlignmentWrapper& al,
												 int row_y, int x0, int total_width)
{
	if (row_y < 0)
	{
		qDebug() << __FILE__ << __LINE__ << ": Bug: draw received negative row_y" << Qt::endl;
		return;
	}

	const BedLine& region = SharedData::region();

	if (al.alignment.end() < region.start() ||
		al.alignment.start() > region.end()) return;

	drawAlignment(painter, al, row_y);
	if (!show_all_bases_) drawMismatches(painter, al, row_y, x0, total_width);
	else drawAllBases(painter, al, row_y, x0, total_width);
}

void BamAlignmentTrack::drawNormalMode(QPainter& painter, const BedLine&)
{
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;
	int x0 = label_width + 2;

	const QVector<BamAlignmentWrapper>& alns = track_data_->getAlignments();
	for (int i =0; i < alns.size(); ++i)
	{
		const BamAlignmentWrapper& al_w = alns[i];
		int row_y = row_idxes_.value(alns[i], -1) * (ROW_HEIGHT + ROW_PADDING);
		drawAlignmentAndMismatches(painter, al_w, row_y, x0, total_width);
	}
}

void BamAlignmentTrack::drawPairMode(QPainter& painter, const BedLine& region)
{
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;
	int x0 = label_width + 2;

	const QVector<BamAlignmentWrapper>& alns = track_data_->getAlignments();

	foreach (const ReadPair& read_pair, read_pairs_)
	{
		if (read_pair.first != -1)
		{
			const BamAlignmentWrapper& al_w = alns[read_pair.first];
			int row_y = pair_row_idxes_.value(al_w.alignment.name(), -1) * (ROW_HEIGHT + ROW_PADDING);
			drawAlignmentAndMismatches(painter, al_w, row_y, x0, total_width);
		}

		if (read_pair.second != -1)
		{
			const BamAlignmentWrapper& al_w = alns[read_pair.second];
			const BamAlignment& al = alns[read_pair.second].alignment;
			int row_y = pair_row_idxes_.value(al.name(), -1) * (ROW_HEIGHT + ROW_PADDING);
			drawAlignmentAndMismatches(painter, al_w, row_y, x0, total_width);
		}

		//draw line
		if(read_pair.first != -1 && read_pair.second != -1)
		{
			const BamAlignment& al1 = alns[read_pair.first].alignment;
			const BamAlignment& al2 = alns[read_pair.second].alignment;

			int st = std::clamp(al1.end() + 1, region.start(), region.end() + 1);
			int en = std::clamp(al2.start(), region.start(), region.end() + 1);

			float p0 = chrToScreen(st);
			float p1 = chrToScreen(en);

			painter.setPen(Qt::gray);
			painter.setBrush(Qt::gray);
			int row_y = pair_row_idxes_.value(al1.name(), -1) * (ROW_HEIGHT + ROW_PADDING);

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
	// return is_reversed ? QColor(210, 150, 150) : QColor(150, 180, 210);
	return is_reversed ? QColor(175, 175, 235, 200) : QColor(235, 175, 175, 200);
}

void BamAlignmentTrack::drawAlignment(QPainter& painter, const BamAlignmentWrapper& al_w, int row_y)
{
	const BedLine& region = SharedData::region();
	const BamAlignment& al = al_w.alignment;

	foreach (const auto& data, al_w.getEvents())
	{
		int st = std::max(data.genome_pos, region.start());
		int en = std::min(data.genome_pos + data.length - 1 , region.end() + 1);

		if (en <= region.start()) continue;

		float x_start = chrToScreen(st);
		float width = chrWidthToScreen(en - st + 1);

		if (width < 0) continue;

		if (data.event == BamAlignmentWrapper::MATCH)
		{
			#ifdef DRAW_TRANSPARENT
			painter.setPen(strandColor(al.isReverseStrand()));
			painter.setBrush(Qt::NoBrush);
			#else
			painter.setPen(Qt::NoPen);
			painter.setBrush(strandColor(al.isReverseStrand()));
			#endif
			painter.drawRect(x_start, row_y, width, ROW_HEIGHT);
		}

		else if (data.event == BamAlignmentWrapper::DELETION)
		{
			painter.setPen(Qt::blue);
			int mid = row_y + ROW_HEIGHT / 2;
			painter.drawLine(x_start, mid, x_start + width, mid);
		}
	}

	// second pass for insertions
	foreach (const auto& data, al_w.getEvents())
	{
		int st = std::max(data.genome_pos, region.start());
		int en = std::min(data.genome_pos + data.length - 1 , region.end() + 1);

		if (en <= region.start()) continue;

		float x_start = chrToScreen(st);
		float width = chrWidthToScreen(en - st + 1);

		if (width < 0) continue;

		if (data.event == BamAlignmentWrapper::INSERTION)
		{
			const float marker_w = 4.0f;
			QRectF rect(x_start - marker_w / 2.0f, row_y, marker_w, ROW_HEIGHT);
			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor(140, 0, 200));
			painter.drawRect(rect);
			// painter.drawText(text_rect, Qt::AlignHCenter, "I");
			// painter.drawRect(x_start, row_y, 2, ROW_HEIGHT);
		}
	}

	int st = std::max(al.start(), region.start());
	int en = std::min(al.end(), region.end() + 1);

	float x_start = chrToScreen(st);
	float width = chrWidthToScreen(en - st + 1);

	float tri_w = std::min(6.f, width / 3);
	QColor body = strandColor(al.isReverseStrand());
	painter.setBrush(body.darker(130));
	QPolygon arrow;
	int mid = row_y + ROW_HEIGHT / 2;
	if (!al.isReverseStrand())
	{
		arrow << QPoint(x_start + width, mid)
		<< QPoint(x_start + width - tri_w, row_y)
		<< QPoint(x_start + width - tri_w, row_y + ROW_HEIGHT);
	}
	else
	{
		arrow << QPoint(x_start, mid)
		<< QPoint(x_start + tri_w, row_y)
		<< QPoint(x_start + tri_w, row_y + ROW_HEIGHT);
	}
	painter.drawPolygon(arrow);
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
	font.setPointSize(ROW_HEIGHT);
	font.setBold(true);

	cached_font_ = font;
	cached_char_size_ = characterSize(font);
}

void BamAlignmentTrack::drawMismatches(QPainter& painter, const BamAlignmentWrapper& al, int row_y, int x0, int total_width)
{
	const auto& region = SharedData::region();
	double pixels_per_base = (double)(total_width) / (double)region.length();

	const auto& mismatches = al.getMismatches();
	foreach (const auto& variant_data, mismatches)
	{
		int idx = variant_data.genomic_pos - region.start();
		int x_start = chrToScreen(region.start() + idx);
		int end_x = chrToScreen(region.start() + idx + 1);

		if (x_start < x0 || end_x > x0 + total_width) continue;

		int dX = std::max(1, end_x - x_start);
		QColor color = baseColor(variant_data.base);
		color = QColor(color.red(), color.green(), color.blue(), ((float)variant_data.quality/41)*255);

		if (pixels_per_base >= cached_char_size_.width())
		{
			painter.setFont(cached_font_);
			// int w = cached_char_size_.width();
			painter.setPen(color);
			QRectF text_rect(x_start, row_y, dX, ROW_HEIGHT);
			painter.drawText(text_rect, Qt::AlignCenter, QString(variant_data.base).toUpper());
		}
		else
		{
			painter.setBrush(color);
			QRectF rect(x_start, row_y, dX, ROW_HEIGHT);
			painter.drawRect(rect);
		}
	}
}

void BamAlignmentTrack::drawAllBases(QPainter& painter, const BamAlignmentWrapper& al, int row_y, int x0, int total_width)
{
	const auto& region = SharedData::region();
	double pixels_per_base = (double)(total_width) / (double)region.length();

	foreach (const auto& event_data, al.getEvents())
	{
		if (event_data.event == BamAlignmentWrapper::MATCH)
		{
			for (int i =0; i < event_data.length; ++i)
			{
				int idx = event_data.genome_pos - region.start() + i;
				int x_start = chrToScreen(region.start() + idx);
				int end_x = chrToScreen(region.start() + idx + 1);

				if (x_start < x0 || end_x > x0 + total_width) continue;

				int dX = std::max(1, end_x - x_start);

				char base = event_data.bases[i];
				int qual = event_data.qualities[i];
				if (i >= event_data.bases.length()) qDebug("i > length of bases, bug.");
				QColor color = baseColor(base);
				color = QColor(color.red(), color.green(), color.blue(), ((float)qual/41)*255);

				if (pixels_per_base >= cached_char_size_.width())
				{
					painter.setFont(cached_font_);
					painter.setPen(color);
					QRectF text_rect(x_start, row_y - 5, dX, ROW_HEIGHT + 10);
					painter.drawText(text_rect, Qt::AlignHCenter, QString(base).toUpper());
				}
				else
				{
					painter.setBrush(color);
					QRectF rect(x_start, row_y, dX, ROW_HEIGHT);
					painter.drawRect(rect);
				}
			}
		}
	}
}

void BamAlignmentTrack::makePairs()
{
	read_pairs_.clear();

	QHash<QString, int> pending;

	const auto& alns = track_data_->getAlignments();

	for (int i =0; i < alns.count(); ++i)
	{
		const BamAlignment& al = alns[i].alignment;
		QString name = al.name();
		if (pending.contains(name))
		{
			int j = pending.take(name);

			read_pairs_[j].second = i;
			read_pairs_[j].start = std::min(read_pairs_[j].start, al.start());
			read_pairs_[j].end = std::max(read_pairs_[j].end, al.end());
		}
		else
		{
			ReadPair p;
			p.first = i;
			p.second = -1;
			p.start = al.start();
			p.end = al.end();
			pending[name] = read_pairs_.count();
			read_pairs_.append(p);
		}
	}
}

void BamAlignmentTrack::populateContextMenu(QMenu& menu)
{
	pairs_action_ = menu.addAction("View As Pairs");
	pairs_action_->setCheckable(true);
	pairs_action_->setChecked(view_as_pairs_);

	all_bases_action_ = menu.addAction("Show all bases");
	all_bases_action_->setCheckable(true);
	all_bases_action_->setChecked(show_all_bases_);
	TrackWidget::populateContextMenu(menu);
}

void BamAlignmentTrack::handleContextMenuAction(QAction* action)
{
	if (action == pairs_action_)
	{
		view_as_pairs_ = !view_as_pairs_;
		// trigger update
		calculateRows();
		updateGeometry();
		update();
	}
	else if (action == all_bases_action_)
	{
		show_all_bases_ = !show_all_bases_;
		update();
	}
	else TrackWidget::handleContextMenuAction(action);
}

void BamAlignmentTrack::mousePressEvent(QMouseEvent* event)
{
	mouse_press_pos_ = event->pos();
	TrackWidget::mousePressEvent(event);
}

QString BamAlignmentTrack::getBamAlignmentText(const BamAlignment& al)
{
	return QString("%1\nStart: %2  End: %3\nStrand: %4\n")
	.arg(al.name())
		.arg(al.start())
		.arg(al.end())
		.arg(al.isReverseStrand() ? "Reverse" : "Forward");
}

void BamAlignmentTrack::handlePopupRequest(QPoint local_pos, QPointF global_pos)
{
	int y = local_pos.y();
	int row = y / (ROW_HEIGHT + ROW_PADDING);

	int x = local_pos.x();
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;

	if (x < label_width + 2 || x > width() - 2) return;

	const BedLine& region = SharedData::region();

	float p = (float)(x - label_width - 2) / total_width;
	int genome_pos = region.start() + p * (region.end() - region.start());

	int aln_idx = row_packer_.find(row, genome_pos);

	const auto& alns = track_data_->getAlignments();

	if (aln_idx != -1)
	{
		if (view_as_pairs_)
		{
			const ReadPair& rp = read_pairs_[aln_idx];

			QString info;

			if (rp.first != -1)
			{
				const BamAlignment& aln1 = alns[rp.first].alignment;
				info += QString("Read 1: %1").arg(getBamAlignmentText(aln1));
			}

			if (rp.second != -1)
			{
				const BamAlignment& aln2 = alns[rp.second].alignment;
				info += QString("Read 2: %1").arg(getBamAlignmentText(aln2));
			}

			showInfoPopup(global_pos, info);
		}
		else if (!view_as_pairs_)
		{
			const BamAlignment& aln = alns[aln_idx].alignment;

			QString info = QString("Read: %1").arg(getBamAlignmentText(aln));

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

	if (!dragging)
	{
		handlePopupRequest(event->pos(), event->globalPosition());
	}

	TrackWidget::mouseReleaseEvent(event);
}
