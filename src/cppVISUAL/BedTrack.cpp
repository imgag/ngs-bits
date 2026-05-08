#include "BedTrack.h"
#include "FileLoader.h"
#include "SharedData.h"
#include "TrackManager.h"

#include <QApplication>
#include <QActionGroup>
#include <QDrag>
#include <QPainter>
#include <QMenu>
#include <QMimeData>
#include <QToolTip>

static constexpr int BLOCK_HEIGHT  = 10;
static constexpr int BLOCK_PADDING = 5;
static constexpr int SPACING_BELOW = 20;


BedTrack::BedTrack(QWidget* parent, QString file_path, QString name)
	:TrackWidget(parent, file_path, name)
{
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(regionChanged()));
}


void BedTrack::reloadTrack()
{
	if (!load()) emit trackDeleted();
}

void BedTrack::setBedFile(QSharedPointer<BedFile> bedfile)
{
	bedfile_ = bedfile;
	chr_index_ = std::make_unique<ChromosomalIndex<BedFile>>(*bedfile_);
	chr_index_->createIndex();
	calculateNumRows();
	updateGeometry();
	update();
}

bool BedTrack::load()
{
	QSharedPointer<BedFile> bedfile = FileLoader::loadBedFile(file_path_);
	if (bedfile)
	{
		setBedFile(bedfile);
		return true;
	}
	return false;
}

void BedTrack::paintEvent(QPaintEvent* /*event*/)
{
	const BedLine& region = SharedData::region();
	QPainter painter(this);

	painter.fillRect(rect(), QColor(250, 250, 250));

	int label_width = SharedData::settings().label_width;

	// draw text
	drawLabel(painter);

	// draw bounding box
	QRectF bounding_rect(label_width + 2, 0, width() - label_width - 2, height() - 2);
	QPen outlinePen(QColor(192, 192, 192));
	outlinePen.setWidth(1);
	outlinePen.setStyle(Qt::SolidLine);
	painter.setPen(outlinePen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(bounding_rect);

	// draw bands
	if (bedfile_->chromosomes().contains(region.chr()))
	{
		int w = width();
		float total_width = w - label_width - 4;
		float y_start = 0.0f;

		QVector<int> idxes = chr_index_->matchingIndices(region.chr(), region.start(), region.end());
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
	row_store_.clear();

	for (int i =0; i < bedfile_->count(); ++i)
	{
		Chromosome chr = (*bedfile_)[i].chr();

		bool placed = false;
		int start = (*bedfile_)[i].start();
		int end = (*bedfile_)[i].end();

		for (int row =0; row < row_end_positions[chr].count(); ++row)
		{
			if ((*bedfile_)[i].start() >= row_end_positions[chr][row])
			{
				placed = true;
				row_end_positions[chr][row] = (*bedfile_)[i].end();
				row_idxes_ << row;
				row_store_[row] << BandData{start, end, i};
				break;
			}
		}

		if (!placed)
		{
			// create new row
			row_end_positions[chr].append((*bedfile_)[i].end());
			row_idxes_ << row_end_positions[chr].count() - 1;
			row_store_[row_end_positions[chr].count() - 1] << BandData{start, end, i};
		}
	}

	for (auto it = row_end_positions.begin(); it != row_end_positions.end(); ++it)
	{
		num_rows_[it.key()] = it.value().size();
	}
}


void BedTrack::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
	{
		TrackWidget::mousePressEvent(event);
		return;
	}

	mouse_press_pos_ = event->pos();
	TrackWidget::mousePressEvent(event);
}

void BedTrack::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
	{
		TrackWidget::mouseReleaseEvent(event);
		return;
	}

	float dist = (event->pos() - mouse_press_pos_).manhattanLength();
	bool dragging = dist >= QApplication::startDragDistance();

	if (!dragging)
	{
		int row = event->pos().y() / (BLOCK_HEIGHT + BLOCK_PADDING);

		const BedLine& region = SharedData::region();

		int label_width = SharedData::settings().label_width;
		int total_width = width() - 4 - label_width;

		float p = ((float)(event->pos().x() - label_width - 2) / total_width);

		int x = region.start() + (region.end() - region.start()) * p;

		QString info = getBandText(region, row, x);

		if (!info.isEmpty())
		{
			showInfoPopup(event->globalPosition(), info);
		}
	}

	TrackWidget::mouseReleaseEvent(event);
}

QString BedTrack::getBandText(const BedLine& region, int row, int x)
{
	if (draw_mode_ == COLLAPSED) return getBandTextCollapsedMode(region, row, x);
	else return getBandTextExpandedMode(region, row, x);
}

QString BedTrack::getBandTextExpandedMode(const BedLine& region, int row, int x)
{
	QString collected_info;
	// int resolution = region.length() * .001;
	foreach (const BandData& data, row_store_[row])
	{
		if (x >= data.start &&
			x <= data.end)
		{
			const BedLine& bd = (*bedfile_)[data.ind];
			if (bd.chr() == region.chr())
			{
				collected_info += getBandString(bd);
				collected_info += "\n";
			}
		}
	}
	return collected_info;
}

QString BedTrack::getBandTextCollapsedMode(const BedLine& region, int row, int x)
{
	QString collected_info;

	if (row != 0) return collected_info;

	QVector<int> idxes = chr_index_->matchingIndices(region.chr(), x - 20, x + 20);

	foreach (int idx, idxes)
	{
		const BedLine& bd = (*bedfile_)[idx];
		collected_info += getBandString(bd);
		collected_info += "\n";
	}

	return collected_info;
}

QString BedTrack::getBandString(const BedLine& bd)
{
	QString info;

	info += "Location: "
		 + bd.chr().str() + ":"
		 + QString::number(bd.start()) + "-"
		 + QString::number(bd.end());

	foreach (const QByteArray& a, bd.annotations())
	{
		info += "\t" + a;
	}

	return info;
}

BedTrack* BedTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
	QSharedPointer<BedFile> bedfile = FileLoader::loadBedFile(file_path);
	if (bedfile)
	{
		BedTrack* track = new BedTrack(parent, file_path, name);
		track->setBedFile(bedfile);
		return track;
	}
	return nullptr;
}
