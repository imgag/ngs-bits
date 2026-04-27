#include "BamAlignmentTrack.h"
#include "SharedData.h"

#include <QPainter>

BamAlignmentTrack::BamAlignmentTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name)
{
}

void BamAlignmentTrack::setTrackData(QSharedPointer<BamTrackData> track_data)
{
	track_data_ = track_data;
	connect(track_data_.get(), SIGNAL(onDataUpdate()), this, SLOT(dataReady()));
}

void BamAlignmentTrack::dataReady()
{
	calculateRows();
	updateGeometry();
	update();
}

void BamAlignmentTrack::calculateRows()
{
	const QVector<BamAlignmentWrapper>& alns = track_data_->getAlignments();

	row_packer_.clear();
	foreach (const BamAlignmentWrapper& w, alns)
	{
		if (!row_idxes_.contains(w)) continue;
		row_packer_.restore(row_idxes_[w], w.alignment.start(), w.alignment.end());
	}

	foreach (const BamAlignmentWrapper& w, alns)
	{
		if (row_idxes_.contains(w)) continue;
		int row = row_packer_.insert(w.alignment.start(), w.alignment.end());
		row_idxes_[w] = row;
	}

	if (num_rows_ < row_packer_.rowCount()) num_rows_ = row_packer_.rowCount();
}

QSize BamAlignmentTrack::sizeHint() const
{
	return QSize(parentWidget() ? parentWidget()->width() : 200,
				 num_rows_ * (ROW_HEIGHT * ROW_PADDING) + SPACING_BELOW);
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
		int label_width = SharedData::settings().label_width;
		int total_width = width() - label_width - 4;
		int x0 = label_width + 2;

		const QVector<BamAlignmentWrapper>& alns = track_data_->getAlignments();
		for (int i =0; i < alns.size(); ++i)
		{
			const BamAlignment& al = alns[i].alignment;
			if (al.end() < region.start()) continue;
			if (al.start() > region.end()) continue;

			int row_y = row_idxes_[alns[i]] * (ROW_HEIGHT + ROW_PADDING);
			drawAlignment(painter, al, row_y, x0, total_width);
			drawVariants(painter, al, row_y, x0, total_width);
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

void BamAlignmentTrack::drawAlignment(QPainter& painter, const BamAlignment& al, int row_y, int x0, int total_width)
{
	const BedLine& region = SharedData::region();

	int st = std::max(al.start(), region.start());
	int en = std::min(al.end(), region.end() + 1);

	// float x_start = map(st, region.start(), region.end() + 1, 0.0f, total_width);
	float x_start = ((float)(st - region.start()))/region.length() * total_width + x0;
	float width = ((float)(en - st))/region.length() * total_width;
	// float width = map(en - st, 0.f, region.length(), 0.f, total_width);

	painter.setBrush(strandColor(al.isReverseStrand()));
	painter.setPen(Qt::NoPen);
	painter.drawRect(x_start, row_y, width, ROW_HEIGHT);
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

void BamAlignmentTrack::drawVariants(QPainter& painter, const BamAlignment& al, int row_y, int x0, int total_width)
{
	const auto& seq = track_data_->getReferenceSeq();
	const auto& region = SharedData::region();
	float scale = (float)region.length() / total_width;
	//TODO: this can be optimized
	double pixels_per_base = (double)(total_width) / (double)region.length();
	for (int pos = al.start(); pos < al.end(); ++pos)
	{
		int idx = pos - region.start();
		if (idx < 0 || idx >= region.length()) continue;

		char ref_base = seq[idx] | 32;
		auto [base, qual] = al.extractBaseByCIGAR(pos);
		base = base | 32;
		if (ref_base != base && base != '-')
		{
			int x_start = x0 + (int)((float)idx / scale);
			int endX = x0 + (int)((float)(idx + 1) / scale);
			int dX = std::max(1, endX - x_start);
			QColor color = baseColor(base);
			color = QColor(color.red(), color.green(), color.blue(), ((float)qual/40)*255);
			QFont font = painter.font();
			font.setPointSize(ROW_HEIGHT);
			font.setBold(true);
			QSize char_size = characterSize(font);

			if (pixels_per_base >= char_size.width())
			{
				painter.setFont(font);
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
