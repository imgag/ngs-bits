#include "BedTrack.h"
#include "FileLoader.h"
#include "SharedData.h"
#include "ColorNames.h"
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <algorithm>

static constexpr int BLOCK_HEIGHT  = 10;

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
	connect(SharedData::instance(), &SharedData::regionChanged, this, [this]()
	{
		calculateRows();
		regionChanged();
	});
}

QList<Parameter> BedTrack::getParameters() const
{
	return {
		Parameter("color", "Region color.", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, ColorNames::names().join('\t').toUtf8()}}),
		Parameter("draw mode", "How regions are drawn:\ncollapsed: overlapping regions share one row\nexpanded: overlapping regions occupy separate rows", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, QByteArray("collapsed\texpanded")}}),
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

void BedTrack::reloadTrack()
{
	if (!load()) emit trackDeleted();
}

void BedTrack::setBedFile(QSharedPointer<BedFile> bedfile)
{
	if (!bedfile) THROW(ArgumentException, "Cannot set a null BED file!");
	//Build first so invalid input leaves the previous file and index intact.
	auto index = std::make_unique<ChromosomalIndex<BedFile>>(*bedfile);
	chr_index_ = std::move(index);
	bedfile_ = bedfile;
	calculateRows();
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
	painter.fillRect(rect(), Qt::white);

	//draw label
	drawLabel(painter);
	if (!bedfile_ || viewport.usable_width <= 0 || region.length() <= 0) return;
	painter.setClipRect(QRect(viewport.x0, 0, viewport.usable_width, height()));

	//set up font
	painter.setFont(regionFont());
	QFontMetrics fm(painter.font());


	//draw regions
	QHash<int, float> last_label_x;
	if (bedfile_ && bedfile_->chromosomes().contains(region.chr()))
	{
		float y_start = 0.0f;

		for (int idx : visible_indices_)
		{
			const BedLine& bedline = (*bedfile_)[idx];
			int st = std::max(bedline.start(), region.start());
			int en = std::min(bedline.end(), region.end());

			float x_start = viewport.genomePosToScreen(st);
			float width = viewport.genomeWidthToScreen(en - st + 1);

			const int row = draw_mode == "expanded" ? row_idxes_.value(idx) : 0;
			y_start = row * row_height;

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

				if (text_x > last_label_x.value(row, -1.0f))
				{
					painter.setPen(Qt::black);
					QRectF text_rect(text_x, y_start + BLOCK_HEIGHT, text_width, fm.height());
					painter.drawText(text_rect, Qt::AlignCenter, name);
					last_label_x[row] = text_x + text_width + 5;
				}
			}
		}
	}
}

QFont BedTrack::regionFont() const
{
	QFont result = font();
	result.setPointSize(8);
	return result;
}

int BedTrack::rowHeight() const
{
	return BLOCK_HEIGHT + (settings_.getBool("show region names") ? QFontMetrics(regionFont()).height() : 0) + 2;
}

QSize BedTrack::sizeHint() const
{
	int rowCount = 1;

	if (settings_.getString("draw mode") == "expanded")
	{
		rowCount = num_rows_;
	}
	return QSize(parentWidget() ? parentWidget()->width() : 200, std::max(25, rowCount * rowHeight()));
}

void BedTrack::calculateRows()
{
	visible_indices_.clear();
	row_idxes_.clear();
	num_rows_ = 0;
	if (!chr_index_) return;
	const BedLine& region = SharedData::region();
	visible_indices_ = chr_index_->matchingIndices(region.chr(), region.start(), region.end());
	QVector<int> row_end_positions;
	for (int idx : visible_indices_)
	{
		const BedLine& bedline = (*bedfile_)[idx];
		int row = 0;
		while (row < row_end_positions.size() && bedline.start() <= row_end_positions[row]) ++row;
		if (row == row_end_positions.size()) row_end_positions.append(bedline.end());
		else row_end_positions[row] = bedline.end();
		row_idxes_.insert(idx, row);
	}
	num_rows_ = row_end_positions.size();
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

	if (!chr_index_ || viewport.usable_width <= 0 || region.length() <= 0 || viewport.isOutOfDrawRegion(local_pos.x())) return;
	int x = viewport.screenXToGenomePos(local_pos.x());

	QString info = getRegionText(region, row, x);

	if (!info.isEmpty()) showInfoPopup(global_pos, info);
}

QString BedTrack::getRegionText(const BedLine& region, int row, int x)
{
	const int total_width = width() - SharedData::settings().label_width - 4;
	if (!chr_index_ || total_width <= 0 || region.length() <= 0 || row < 0) return {};
	const bool expanded = settings_.getString("draw mode") == "expanded";
	if ((!expanded && row != 0) || (expanded && row >= num_rows_)) return {};
	double bp_per_pixel = (double)region.length() / total_width;
	int padding = std::max(2, static_cast<int>(bp_per_pixel * 2));
	QString collected_info;
	//Keep lookup within the visible region, matching the packed rows.
	const int start = std::max(region.start(), x - padding);
	const int end = std::min(region.end(), x + padding);
	if (start > end) return {};
	for (int idx : chr_index_->matchingIndices(region.chr(), start, end))
	{
		if (expanded && row_idxes_.value(idx, -1) != row) continue;
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
