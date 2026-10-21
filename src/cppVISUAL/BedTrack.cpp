#include "BedTrack.h"
#include "FileLoader.h"
#include "SharedData.h"
#include "Helper.h"
#include "ColorNames.h"
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QMessageBox>
#include <algorithm>
#include <limits>

static constexpr int BLOCK_HEIGHT  = 10;

namespace
{
QSharedPointer<BedFile> queryBed(const TabixIndexedFile& tabix, const BedLine& region)
{
	auto bed = QSharedPointer<BedFile>::create();
	if (!region.isValid())
	{
		const BedFile& chromosomes = SharedData::wholeGenomeRegions();
		for (int c = 0; c < chromosomes.count(); ++c)
		{
			const auto queried = queryBed(tabix, chromosomes[c]);
			for (int i = 0; i < queried->count(); ++i) bed->append((*queried)[i]);
		}
		bed->sort();
		return bed;
	}
	for (const QByteArray& line : tabix.getMatchingLines(region.chr(), region.start(), region.end(), true))
	{
		const QByteArrayList fields = line.split('\t');
		if (fields.count() < 3) THROW(FileParseException, "Indexed BED line has fewer than three columns: " + line);
		const int start = Helper::toInt(fields[1], "BED start");
		const int end = Helper::toInt(fields[2], "BED end");
		if (start < 0 || start == std::numeric_limits<int>::max()) THROW(FileParseException, "Invalid indexed BED start: " + line);
		//BED is zero-based, half-open; BedLine uses one-based inclusive coordinates.
		//Keep annotations local to this query instead of retaining them in the global string pool.
		bed->append(BedLine(Chromosome(fields[0]), start + 1, end, fields.mid(3)));
	}
	bed->sort();
	return bed;
}
}

BedTrack::BedTrack(QWidget* parent, QString file_path, QString name)
	:TrackWidget(parent, file_path, name, type())
{
	initializeSettings(&fast_settings_);
	setMinimumHeight(SharedData::region().isValid() ? 25 : 50);
	connect(&settings_, &ParameterList::parameterChanged, this, [this]()
	{
		updateGeometry();
		update();
	});
	connect(SharedData::instance(), &SharedData::regionChanged, this, [this]()
	{
		setMinimumHeight(SharedData::region().isValid() ? 25 : 50);
		if (tabix_)
		{
			try
			{
				setBedData(queryBed(*tabix_, SharedData::region()));
				return;
			}
			catch (const Exception& e)
			{
				//Discard stale intervals, but allow the track to recover on the next region change.
				chr_index_.reset();
				bedfile_.clear();
				load_error_ = e.message();
			}
		}
		calculateRows();
		regionChanged();
	});
}

QList<Parameter> BedTrack::getParameters() const
{
	return {
		Parameter("color", "Region color.", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, (ColorNames::names() + QStringList{"transcript blue"}).join('\t').toUtf8()}}),
		Parameter("draw mode", "How regions are drawn:\ncollapsed: overlapping regions share one row\nexpanded: overlapping regions occupy separate rows", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, QByteArray("collapsed\texpanded")}}),
		Parameter("show region names", "Display region names from the fourth BED column.", ParameterType::BOOL, {})
	};
}

QHash<QByteArray, QVariant> BedTrack::getParameterDefaults() const
{
	return {
		{"draw mode", QByteArray("expanded")},
		{"color", QByteArray("transcript blue")},
		{"show region names", true}
	};
}

void BedTrack::reloadTrack()
{
	if (!load()) emit trackDeleted();
}

void BedTrack::setBedFile(QSharedPointer<BedFile> bedfile)
{
	setBedData(bedfile);
	tabix_.reset();
}

void BedTrack::setBedData(QSharedPointer<BedFile> bedfile)
{
	if (!bedfile) THROW(ArgumentException, "Cannot set a null BED file!");
	//Build first so invalid input leaves the previous file and index intact.
	auto index = std::make_unique<ChromosomalIndex<BedFile>>(*bedfile);
	chr_index_ = std::move(index);
	bedfile_ = bedfile;
	load_error_.clear();
	calculateRows();
	updateGeometry();
	update();
}

bool BedTrack::load()
{
	if (getDisplayNameFromFilePath(file_path_).endsWith(".bed.gz", Qt::CaseInsensitive))
	{
		try
		{
			auto tabix = std::make_unique<TabixIndexedFile>();
			tabix->load(file_path_.toUtf8());
			setBedData(queryBed(*tabix, SharedData::region()));
			tabix_ = std::move(tabix);
			return true;
		}
		catch (const Exception& e)
		{
			QMessageBox::critical(QApplication::activeWindow(), "Error loading indexed BED", e.message());
			return false;
		}
	}
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
	const int row_height = rowHeight();
	const QColor color = fast_settings_.color == "transcript blue" ? QColor("#0000B2") : ColorNames::color(QString::fromUtf8(fast_settings_.color));
	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();

	//fill background
	QPainter painter(this);
	painter.fillRect(rect(), Qt::white);

	//draw label
	drawLabel(painter);
	if (!load_error_.isEmpty())
	{
		painter.setPen(Qt::red);
		painter.drawText(QRect(viewport.x0, 0, std::max(0, viewport.usable_width), height()), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, load_error_);
		return;
	}
	if (!bedfile_ || viewport.usable_width <= 0) return;
	painter.setClipRect(QRect(viewport.x0, 0, viewport.usable_width, height()));
	if (!region.isValid())
	{
		drawOverview(painter, color);
		return;
	}
	if (region.length() <= 0) return;

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

			const int row = fast_settings_.draw_mode == "expanded" ? row_idxes_.value(idx) : 0;
			y_start = row * row_height;

			QRectF chr_rect(x_start, y_start, width, BLOCK_HEIGHT);
			painter.setBrush(color);
			painter.setPen(color);
			painter.drawRect(chr_rect);

			// use 4th column as name
			if (fast_settings_.show_region_names && !bedline.annotations().isEmpty())
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

void BedTrack::drawOverview(QPainter& painter, const QColor& color)
{
	const Viewport viewport = getViewport();
	QVector<int> counts(viewport.usable_width, 0);
	int maximum = 0;
	for (const auto& plot : plotViewports())
	{
		const BedLine& region = plot.first;
		const Viewport& chromosome = plot.second;
		//Difference counts avoid walking every pixel covered by every region.
		QVector<int> differences(chromosome.usable_width + 1, 0);
		for (int idx : chr_index_->matchingIndices(region.chr(), region.start(), region.end()))
		{
			const BedLine& line = (*bedfile_)[idx];
			const qint64 start = std::max(line.start(), region.start()) - region.start();
			const qint64 end = std::min(line.end(), region.end()) - region.start() + 1;
			const int first = start * chromosome.usable_width / region.length();
			const int last = (end * chromosome.usable_width - 1) / region.length();
			++differences[first];
			--differences[last + 1];
		}
		int count = 0;
		for (int x = 0; x < chromosome.usable_width; ++x)
		{
			count += differences[x];
			counts[chromosome.x0 - viewport.x0 + x] = count;
			maximum = std::max(maximum, count);
		}
	}
	const int bottom = height() - 4;
	const int plot_height = std::max(1, height() - 8);
	if (maximum > 0)
	{
		for (int x = 0; x < counts.size(); ++x)
		{
			if (counts[x] == 0) continue;
			const int bar_height = std::max(1, qRound(double(counts[x]) / maximum * plot_height));
			painter.fillRect(viewport.x0 + x, bottom - bar_height, 1, bar_height, color);
		}
	}
	painter.setPen(Qt::black);
	painter.drawText(QRect(viewport.x0, 0, viewport.usable_width, height()), Qt::AlignLeft | Qt::AlignTop,
		QString("[0,%1]").arg(maximum));
}

QFont BedTrack::regionFont() const
{
	QFont result = font();
	result.setPointSize(8);
	return result;
}

int BedTrack::rowHeight() const
{
	return BLOCK_HEIGHT + (fast_settings_.show_region_names ? QFontMetrics(regionFont()).height() : 0) + 2;
}

QSize BedTrack::sizeHint() const
{
	if (!SharedData::region().isValid()) return QSize(parentWidget() ? parentWidget()->width() : 200, 50);
	int rowCount = 1;

	if (fast_settings_.draw_mode == "expanded")
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
	if (!region.isValid()) return;
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

	if (!region.isValid() || !chr_index_ || viewport.usable_width <= 0 || region.length() <= 0 || viewport.isOutOfDrawRegion(local_pos.x())) return;
	int x = viewport.screenXToGenomePos(local_pos.x());

	const int total_width = width() - SharedData::settings().label_width - 4;
	if (!chr_index_ || total_width <= 0 || region.length() <= 0 || row < 0) return;

	bool expanded = fast_settings_.draw_mode == "expanded";
	if ((!expanded && row != 0) || (expanded && row >= num_rows_)) return;

	double bp_per_pixel = (double)region.length() / total_width;
	int padding = std::max(2, static_cast<int>(bp_per_pixel * 2));

	QString info;
	//Keep lookup within the visible region, matching the packed rows.
	const int start = std::max(region.start(), x - padding);
	const int end = std::min(region.end(), x + padding);
	if (start > end) return;

	for (int idx : chr_index_->matchingIndices(region.chr(), start, end))
	{
		if (expanded && row_idxes_.value(idx, -1) != row) continue;
		const BedLine& bd = (*bedfile_)[idx];

		info += "Region: " + bd.toString(true);
		const auto& annotations = bd.annotations();
		if (!annotations.isEmpty())
		{
			info += "\nName: " + annotations[0];
			for (int i = 1; i < annotations.size(); ++i)
			{
				info += "\ncolumn " + QString::number(i + 4) + ": " + annotations[i];
			}
		}
		info += "\n";
	}

	if (!info.isEmpty()) showInfoPopup(global_pos, info);
}

BedTrack* BedTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
	auto track = std::make_unique<BedTrack>(parent, file_path, name);
	if (track->load()) return track.release();
	return nullptr;
}
