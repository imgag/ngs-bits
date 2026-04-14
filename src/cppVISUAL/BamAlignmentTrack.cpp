#include "BamAlignmentTrack.h"
#include "SharedData.h"

#include <QPainter>
#include <_mingw_mac.h>


static int MAX_REGION_LENGTH = 30'000;

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
	row_idxes_.clear();
	QVector<int> row_ends;

	const QVector<BamAlignment>& alns = track_data_->getAlignments();

	for (int i =0; i < alns.count(); ++i)
	{
		int aln_start = alns[i].start();
		int aln_end   = alns[i].end();

		bool placed = false;

		for (int row =0; row < row_ends.count(); ++row)
		{
			if (aln_start >= row_ends[row])
			{
				row_ends[row] = aln_end;
				row_idxes_ << row;
				placed = true;
				break;
			}
		}

		if (!placed)
		{
			row_ends.append(aln_end);
			row_idxes_ << row_ends.count() - 1;
		}
	}
	if (row_ends.count() > num_rows_) num_rows_ = row_ends.count();
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
	if (region.length() > MAX_REGION_LENGTH) drawZoomInText(painter);
	else
	{
		int label_width = SharedData::settings().label_width;
		int total_width = width() - label_width - 4;
		int x_max = width() - 2;
		int x0 = label_width + 2;
		float scale = (float)region.length()/total_width;

		QRectF bounds(x0, 0, total_width, height());

		painter.setClipRect(bounds);

		const QVector<BamAlignment>& alns = track_data_->getAlignments();
		for (int i =0; i < alns.size(); ++i)
		{
			int row_y = row_idxes_[i] * (ROW_HEIGHT + ROW_PADDING);
			drawAlignment(painter, alns[i], row_y, scale, x0, x_max);
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
	painter.drawText(rect(), Qt::AlignCenter, "Zoom In To See Aligments");
	painter.setPen(pen); // restore pen
}

QColor BamAlignmentTrack::strandColor(bool is_reversed)
{
	return is_reversed ? QColor(210, 150, 150) : QColor(150, 180, 210);
}

void BamAlignmentTrack::drawAlignment(QPainter& painter, const BamAlignment& al, int row_y, float scale, int x0, int x_max)
{
	const BedLine& region = SharedData::region();
	int total_width = x_max - x0;

	int st = std::max(al.start(), region.start());
	int en = std::min(al.end(), region.end());

	float x_start = map(st, region.start(), region.end(), 0.0f, total_width);
	float width = map(en - st, 0.f, region.length(), 0.f, total_width);

	painter.setBrush(strandColor(al.isReverseStrand()));
	painter.setPen(Qt::NoPen);
	painter.drawRect(x0 + x_start, row_y, width, ROW_HEIGHT);
	{
		int tri_w = std::min(6.f, width / 3);
		if (tri_w >= 2)
		{
			QColor body = strandColor(al.isReverseStrand());
			painter.setBrush(body.darker(130));
			QPolygon arrow;
			int mid = row_y + ROW_HEIGHT / 2;
			if (!al.isReverseStrand())
			{
				arrow << QPoint(x0 + x_start + width, mid)
					  << QPoint(x0 + x_start + width - tri_w, row_y)
					  << QPoint(x0 + x_start + width - tri_w, row_y + ROW_HEIGHT);
			}
			else
			{
				arrow << QPoint(x0 + x_start, mid)
				<< QPoint(x0 + x_start + tri_w, row_y)
				<< QPoint(x0 + x_start + tri_w, row_y + ROW_HEIGHT);
			}
			painter.drawPolygon(arrow);
		}
	}
}
