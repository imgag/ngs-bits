#include "BedTrack.h"
#include "FileLoader.h"
#include "SharedData.h"

#include <QApplication>
#include <QActionGroup>
#include <QDrag>
#include <QPainter>
#include <QMenu>
#include <QMimeData>
#include <QToolTip>

static constexpr int BLOCK_HEIGHT  = 10;
static constexpr int BLOCK_PADDING = 15;
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

QMap<QString, QVariant> BedTrack::getSettings()
{
	QMap<QString, QVariant> settings;
	settings["draw_mode"] = static_cast<int>(draw_mode_);
	return settings;
}

void BedTrack::loadKeyValueFromXml(QString key, QString value)
{
	bool ok;
	if (key == "draw_mode")
	{
		int draw_mode = value.toInt(&ok);
		if (ok && draw_mode != -1) draw_mode_ = static_cast<DrawMode>(draw_mode);
	}
}

void BedTrack::populateContextMenu(QMenu& menu, const QPoint& local_pos)
{
	QAction* collapsed = menu.addAction("Collapsed");
	QAction* expanded = menu.addAction("Expanded");

	collapsed->setCheckable(true);
	expanded->setCheckable(true);

	switch (draw_mode_)
	{
	case COLLAPSED:
		collapsed->setChecked(true);
		break;
	case EXPANDED:
		expanded->setChecked(true);
		break;
	}

	connect(collapsed, &QAction::triggered, this, [this](){
		draw_mode_ = COLLAPSED;
		updateGeometry(); update();
	});

	connect(expanded, &QAction::triggered, this, [this](){
		draw_mode_ = EXPANDED;
		updateGeometry(); update();
	});

	menu.addSeparator();
	TrackWidget::populateContextMenu(menu, local_pos);
}


void BedTrack::paintEvent(QPaintEvent* /*event*/)
{
	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();
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

	QFont font = painter.font();
	font.setPointSize(8); // fixed screen size
	painter.setFont(font);

	QFontMetrics fm(font);

	float last_label_x = -1.0f;
	float last_label_y = -1.0f;

	// draw bands
	if (bedfile_ && bedfile_->chromosomes().contains(region.chr()))
	{
		float y_start = 0.0f;

		QVector<int> idxes = chr_index_->matchingIndices(region.chr(), region.start(), region.end());
		foreach(int idx, idxes)
		{
			const BedLine& bedline = (*bedfile_)[idx];
			int st = std::max(bedline.start(), region.start());
			int en = std::min(bedline.end(), region.end());

			float x_start = viewport.genomePosToScreen(st);
			float width = viewport.genomeWidthToScreen(en - st + 1);

			if (draw_mode_ == EXPANDED) y_start = row_idxes_[idx] * (BLOCK_HEIGHT + BLOCK_PADDING);

			QRectF chr_rect(x_start, y_start, width, BLOCK_HEIGHT);
			painter.setBrush(color_);
			painter.setPen(outlinePen);
			painter.drawRect(chr_rect);
			// use 4th column as name
			if (!bedline.annotations().isEmpty())
			{
				QString name = bedline.annotations().at(0);

				int text_width = fm.horizontalAdvance(name);

				float text_x = x_start + (width - text_width) / 2.0f;

				if (viewport.isOutOfDrawRegion(text_x)) continue;

				if (text_x > last_label_x || std::abs(y_start - last_label_y) > 1)
				{
					QRectF text_rect(
						text_x,
						y_start + BLOCK_HEIGHT,
						text_width,
						fm.height()
						);

					painter.setPen(Qt::black);
					painter.drawText(text_rect,
									 Qt::AlignCenter,
									 name);

					last_label_x = text_x + text_width + 5;
					last_label_y = y_start;
				}
			}
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
		handlePopupRequest(event->pos(), event->globalPosition());
	}

	TrackWidget::mouseReleaseEvent(event);
}

void BedTrack::handlePopupRequest(QPointF local_pos, QPointF global_pos)
{
	int row = local_pos.y() / (BLOCK_HEIGHT + BLOCK_PADDING);
	const Viewport& viewport = getViewport();

	const BedLine& region = SharedData::region();

	if (viewport.isOutOfDrawRegion(local_pos.x())) return;
	int x = viewport.screenXToGenomePos(local_pos.x());

	QString info = getBandText(region, row, x);

	if (!info.isEmpty()) showInfoPopup(global_pos, info);
}

QString BedTrack::getBandText(const BedLine& region, int row, int x)
{
	if (draw_mode_ == COLLAPSED) return getBandTextCollapsedMode(region, row, x);
	else return getBandTextExpandedMode(region, row, x);
}

QString BedTrack::getBandTextExpandedMode(const BedLine& region, int row, int x)
{
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;
	double bp_per_pixel = (double)region.length() / total_width;
	int padding = std::max(2, static_cast<int>(bp_per_pixel * 2));
	QString collected_info;
	// int resolution = region.length() * .001;
	foreach (const BandData& data, row_store_[row])
	{
		if (x + padding >= data.start &&
			x - padding <= data.end)
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
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;
	double bp_per_pixel = (double)region.length() / total_width;
	int padding = std::max(2, static_cast<int>(bp_per_pixel * 2));

	QString collected_info;

	if (row != 0) return collected_info;

	QVector<int> idxes = chr_index_->matchingIndices(region.chr(), x - padding, x + padding);

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
