#include "BedTrack.h"
#include "SharedData.h"
#include "TrackManager.h"

#include <QApplication>
#include <QActionGroup>
#include <QDrag>
#include <QPainter>
#include <QMenu>
#include <QMimeData>


BedTrack::BedTrack(QWidget* parent, QString file_path, QString name, QSharedPointer<BedFile> bedfile)
	:TrackWidget(parent, file_path, name), bedfile_(bedfile), chr_index_(*bedfile)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)) );
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(regionChanged()));
	chr_index_.createIndex();

	calculateNumRows();
}


void BedTrack::paintEvent(QPaintEvent* /*event*/)
{
	const BedLine& region = SharedData::region();
	QPainter painter(this);

	painter.fillRect(rect(), QColor(250, 250, 250));

	int label_width = SharedData::settings().label_width;

	// draw text
	QRectF text_rect(0, 0, label_width-2, height());

	painter.setPen(Qt::black);
	painter.drawText(text_rect, Qt::AlignLeft, name_);

	// draw bounding box
	QRectF bounding_rect(label_width + 2, 0, width() - label_width - 2, height() - 2);
	QPen outlinePen(QColor(192, 192, 192));
	outlinePen.setWidth(1);
	outlinePen.setStyle(Qt::SolidLine);
	painter.setPen(outlinePen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(bounding_rect);

	if (bedfile_->chromosomes().contains(region.chr()))
	{
		int w = width();
		float total_width = w - label_width - 4;
		float y_start = 0.0f;

		QVector<int> idxes = chr_index_.matchingIndices(region.chr(), region.start(), region.end());
		foreach(int idx, idxes)
			{
				int st = std::max((*bedfile_)[idx].start(), region.start());
				int en = std::min((*bedfile_)[idx].end(), region.end());

				float x_start = map(st, region.start(), region.end(), 0.0f, total_width);
				float width = map(en - st, 0.0f, region.length(), 0.0f, total_width);


				if (draw_mode_ == EXPANDED) y_start = row_idxes_[idx] * (BLOCK_HEIGHT + BLOCK_PADDING);

				QRectF chr_rect(label_width + 2 + x_start, y_start, width, BLOCK_HEIGHT);

				painter.setBrush(color_);
				painter.drawRect(chr_rect);
			}
	}
}


QSize BedTrack::sizeHint() const {
	int rowHeight = BLOCK_HEIGHT + BLOCK_PADDING;
	int rowCount = 1;

	if (draw_mode_ == EXPANDED && bedfile_->chromosomes().contains(SharedData::region().chr()))
	{
		rowCount = num_rows_[SharedData::region().chr()];
	}
	return QSize(parentWidget() ? parentWidget()->width() : 200, rowCount * rowHeight + SPACING_BELOW);
}


void BedTrack::calculateNumRows()
{
	QHash<Chromosome, QVector<int>> row_end_positions;
	row_idxes_.clear();

	for (int i =0; i < bedfile_->count(); ++i)
	{
		Chromosome chr = (*bedfile_)[i].chr();

		bool placed = false;

		for (int row =0; row < row_end_positions[chr].count(); ++row)
		{
			if ((*bedfile_)[i].start() >= row_end_positions[chr][row])
			{
				placed = true;
				row_end_positions[chr][row] = (*bedfile_)[i].end();
				row_idxes_ << row;
				break;
			}
		}

		if (!placed)
		{
			// create new row
			row_end_positions[chr].append((*bedfile_)[i].end());
			row_idxes_ << row_end_positions[chr].count() - 1;
		}
	}

	for (auto it = row_end_positions.begin(); it != row_end_positions.end(); ++it)
	{
		num_rows_[it.key()] = it.value().size();
	}
}
