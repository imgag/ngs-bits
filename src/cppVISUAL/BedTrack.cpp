#include "BedTrack.h"
#include "FileLoader.h"
#include "SharedData.h"
#include "ColorNames.h"
#include <QApplication>
#include <QDrag>
#include <QPainter>
#include <QMimeData>
#include <QToolTip>

static constexpr int BLOCK_HEIGHT  = 10;
static constexpr int BLOCK_PADDING = 15;

BedTrack::BedTrack(QWidget* parent, QString file_path, QString name)
	:TrackWidget(parent, file_path, name, type())
{
	setMinimumHeight(25);
	connect(&settings_, &ParameterList::parameterChanged, this, [this]()
	{
		updateGeometry();
		update();
	});
	initializeSettings();
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(regionChanged()));
}

QList<Parameter> BedTrack::getParameters() const
{
	return {
		Parameter("color", "Region color.", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, ColorNames::names().join('\t').toUtf8()}}),
		Parameter("draw mode", "How regions are drawn:\ncollapsed: overlapping regions are drawn as one\nexpanded: ", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, QByteArray("collapsed\texpanded")}}),
		Parameter("show region names", "Display region names from the fourth BED column.", ParameterType::BOOL, {})
	};
}

QHash<QByteArray, QVariant> BedTrack::getParameterDefaults() const
{
	return {
		{"draw mode", QByteArray("expanded")},
		{"color", QByteArray("dark blue")},
		{"show region names", true}
	};
}

BedTrack::~BedTrack()
{
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
	const QByteArray draw_mode = settings_.getString("draw mode");
	const bool show_names = settings_.getBool("show region names");
	const int row_height = rowHeight();
	const QColor color = ColorNames::color(QString::fromUtf8(settings_.getString("color")));
	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();

	//fill background
	QPainter painter(this);
	painter.fillRect(rect(), QColor(250, 250, 250));

	//draw label
	drawLabel(painter);

	//set up font
	QFont font = painter.font();
	font.setPointSize(8); // fixed screen size
	painter.setFont(font);
	QFontMetrics fm(font);


	//draw regions
	float last_label_x = -1.0f;
	float last_label_y = -1.0f;
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

			if (draw_mode == "expanded") y_start = row_idxes_[idx] * row_height;

			QRectF chr_rect(x_start, y_start, width, BLOCK_HEIGHT);
			painter.setBrush(color);
			painter.setPen(color);
			painter.drawRect(chr_rect);

			// use 4th column as name
			if (show_names && !bedline.annotations().isEmpty())
			{
				QString name = bedline.annotations().at(0);

				int text_width = fm.horizontalAdvance(name);

				float text_x = x_start + (width - text_width) / 2.0f;

				if (viewport.isOutOfDrawRegion(text_x)) continue;

				if (text_x > last_label_x || std::abs(y_start - last_label_y) > 1)
				{
					painter.setPen(Qt::black);
					QRectF text_rect(text_x, y_start + BLOCK_HEIGHT, text_width, fm.height());
					painter.drawText(text_rect, Qt::AlignCenter, name);
					last_label_x = text_x + text_width + 5;
					last_label_y = y_start;
				}
			}
		}
	}
}

int BedTrack::rowHeight() const
{
	return BLOCK_HEIGHT + (settings_.getBool("show region names") ? BLOCK_PADDING : 2);
}

QSize BedTrack::sizeHint() const
{
	int rowCount = 1;

	if (settings_.getString("draw mode") == "expanded" && bedfile_ && bedfile_->chromosomes().contains(SharedData::region().chr()))
	{
		rowCount = num_rows_[SharedData::region().chr()];
	}
	return QSize(parentWidget() ? parentWidget()->width() : 200, std::max(25, rowCount * rowHeight()));
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
				row_store_[row] << RegionData{start, end, i};
				break;
			}
		}

		if (!placed)
		{
			// create new row
			row_end_positions[chr].append((*bedfile_)[i].end());
			row_idxes_ << row_end_positions[chr].count() - 1;
			row_store_[row_end_positions[chr].count() - 1] << RegionData{start, end, i};
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
	int row = local_pos.y() / rowHeight();
	const Viewport& viewport = getViewport();

	const BedLine& region = SharedData::region();

	if (viewport.isOutOfDrawRegion(local_pos.x())) return;
	int x = viewport.screenXToGenomePos(local_pos.x());

	QString info = getRegionText(region, row, x);

	if (!info.isEmpty()) showInfoPopup(global_pos, info);
}

QString BedTrack::getRegionText(const BedLine& region, int row, int x)
{
	if (settings_.getString("draw mode") == "collapsed") return getRegionTextCollapsedMode(region, row, x);
	else return getRegionTextExpandedMode(region, row, x);
}

QString BedTrack::getRegionTextExpandedMode(const BedLine& region, int row, int x)
{
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;
	double bp_per_pixel = (double)region.length() / total_width;
	int padding = std::max(2, static_cast<int>(bp_per_pixel * 2));
	QString collected_info;
	// int resolution = region.length() * .001;
	foreach (const RegionData& data, row_store_[row])
	{
		if (x + padding >= data.start &&
			x - padding <= data.end)
		{
			const BedLine& bd = (*bedfile_)[data.ind];
			if (bd.chr() == region.chr())
			{
				collected_info += regionTooltip(bd);
				collected_info += "\n";
			}
		}
	}
	return collected_info;
}

QString BedTrack::getRegionTextCollapsedMode(const BedLine& region, int row, int x)
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
		collected_info += regionTooltip(bd);
		collected_info += "\n";
	}

	return collected_info;
}

QString BedTrack::regionTooltip(const BedLine& region)
{
	QString info;
	info += "Location: " + region.toString(true);

	const auto& annotations = region.annotations();
	if (!annotations.isEmpty())
	{
		info += "\nName: " + annotations[0];
		for (int i = 1; i < annotations.size(); ++i)
		{
			info += "\ncolumn " + QString::number(i + 4) + ": " + annotations[i];
		}
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
