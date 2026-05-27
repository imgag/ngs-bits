#include "BafTrack.h"
#include "FileLoader.h"
#include "GenomeVisualizationWidget.h"
#include "SharedData.h"

#include <QPainter>
#include <QMenu>

static constexpr int TRACK_HEIGHT = 100;


BafTrack::BafTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name)
{
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(regionChanged()));
}

BafTrack* BafTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
	QSharedPointer<BedFile> bed_file = FileLoader::loadBafFile(file_path);
	if (bed_file)
	{
		int idx = findBafIdx(bed_file);
		if (idx < 0)
		{
			GenomeVisualizationWidget::displayError("Could not find BAF index in headers or index is inavlid in " + file_path);
			return nullptr;
		}
		BafTrack* baf_track = new BafTrack(parent, file_path, name);
		baf_track->setBedFile(bed_file);
		baf_track->setBafIdx(idx);
		return baf_track;
	}
	else return nullptr;
}

QSize BafTrack::sizeHint() const
{
	return QSize( parentWidget() ? parentWidget()->width() : 200, TRACK_HEIGHT);
}

void BafTrack::setBedFile(QSharedPointer<BedFile> bed_file)
{
	if (bed_file)
	{
		bed_file_ = bed_file;
		chr_index_ = std::make_unique<ChromosomalIndex<BedFile>>(*bed_file);
		chr_index_->createIndex();
		update();
	}
}

int BafTrack::findBafIdx(QSharedPointer<BedFile> bed_file)
{
	foreach (const QByteArray& header, bed_file->headers())
	{
		if (header.startsWith("#")) continue;
		QList<QByteArray> columns = header.split('\t');
		for (int i =0; i < columns.count(); ++i)
		{
			if (columns[i].contains("BAF")) return i - 3; // -3 for Chr, Start, End
		}
	}
	return -1;
}

void BafTrack::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.fillRect(rect(), Qt::white);
	drawLabel(painter);
	if (bed_file_ && baf_idx_ >= 0) drawPlot(painter);
}

void BafTrack::drawPlot(QPainter& painter)
{
	const BedLine& region = SharedData::region();
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;
	int x0 = label_width + 2;

	drawReferenceLine(painter, 0.f, x0, total_width, TRACK_HEIGHT);
	drawReferenceLine(painter, 0.5f, x0, total_width, TRACK_HEIGHT);
	drawReferenceLine(painter, 1.0f, x0, total_width, TRACK_HEIGHT);

	int padding = region.length() / 3;
	int start = std::min(0, region.start() - padding);
	int end = region.end() + padding;
	const QVector<int>& idxes = chr_index_->matchingIndices(region.chr(), start, end);

	switch (graph_mode_)
	{
		case HEATMAP:
			drawHeatMap(painter, idxes);
			break;
		case BAR_CHART:
			drawBarChart(painter, idxes);
			break;
		case POINTS:
			drawPoints(painter, idxes);
			break;
		case LINE_PLOT:
			drawLinePlot(painter, idxes);
			break;
	}
}

void BafTrack::drawPoints(QPainter& painter, const QVector<int>& idxes)
{
	const BedLine& region = SharedData::region();
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;
	int x0 = label_width + 2;

	foreach (int idx, idxes)
	{
		const BedLine& bd = (*bed_file_)[idx];
		if (bd.annotations().count() <= baf_idx_) continue;
		bool ok;
		float baf = bd.annotations()[baf_idx_].toFloat(&ok);
		if (!ok) continue;

		int pos = (bd.start() + bd.end()) / 2;

		if (pos < region.start()) continue;

		float norm = (float) (pos - region.start())/(region.end() - region.start());
		int px = x0 + (int)(norm * total_width);
		int py = bafToY(baf, TRACK_HEIGHT);

		painter.setBrush(Qt::blue);
		painter.drawEllipse(QPoint(px, py), 2, 2);
	}
}

void BafTrack::drawLinePlot(QPainter& painter, const QVector<int>& idxes)
{
	painter.setRenderHint(QPainter::Antialiasing);
	const BedLine& region = SharedData::region();
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;
	int x0 = label_width + 2;

	for (int i =1; i < idxes.count(); ++i)
	{
		const BedLine& bd1 = (*bed_file_)[idxes[i-1]];
		const BedLine& bd2 = (*bed_file_)[idxes[i]];
		if (bd1.annotations().count() <= baf_idx_ ||
			bd2.annotations().count() <= baf_idx_) continue;
		bool ok;

		float baf1 = bd1.annotations()[baf_idx_].toFloat(&ok);
		if (!ok) continue;
		float baf2 = bd2.annotations()[baf_idx_].toFloat(&ok);
		if (!ok) continue;

		int pos = (bd1.start() + bd1.end()) / 2;
		int pos2 = (bd2.start() + bd2.end()) / 2;

		float norm = (float) (pos - region.start())/(region.end() - region.start());
		float norm2 = (float) (pos2 - region.start())/(region.end() - region.start());

		if ((norm < 0 && norm2 < 0) ||
			(norm > 1 && norm2 > 1)) continue;

		int px1 = x0 + (int)(norm * total_width);
		int py1 = bafToY(baf1, TRACK_HEIGHT);

		int px2 = x0 + (int)(norm2 * total_width);
		int py2 = bafToY(baf2, TRACK_HEIGHT);

		if (norm < 0 && norm2 > 0)
		{
			// y - y1 = (y2 - y1)/(x2 - x1)(x - x1)
			// y - y1 = m(x0-x1) --> y = m(x0 - x1) + y1
			if (px1 != px2)
			{
				float m = (py2 - py1)/(float)(px2 - px1);
				py1 = m*(x0 - px1) + py1;
			}
			px1 = x0;
		}

		painter.setPen(QPen(Qt::blue, 2, Qt::SolidLine));
		painter.drawLine(px1, py1, px2, py2);
	}
}

void BafTrack::drawHeatMap(QPainter& painter, const QVector<int>& idxes)
{
	painter.setRenderHint(QPainter::Antialiasing);

	const BedLine& region = SharedData::region();
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;
	int x0 = label_width + 2;
	float scale = (float)region.length() / total_width;

	painter.fillRect(x0, 0, total_width, TRACK_HEIGHT, Qt::gray);
	QColor c1(122, 122, 214); // low baf
	QColor c2(0, 0, 255);     // high baf
	foreach (int idx, idxes)
	{
		const BedLine& bd = (*bed_file_)[idx];
		if (bd.annotations().count() <= baf_idx_) continue;
		bool ok;
		float baf = bd.annotations()[baf_idx_].toFloat(&ok);
		if (!ok) continue;

		int pos = (bd.start() + bd.end()) / 2;

		if (pos < region.start()) continue;

		// float norm = (float) (pos - region.start())/(region.end() - region.start());
		// int px = x0 + (int)(norm * total_width);

		int id = pos - region.start();
		float px = x0 + (float)((float)id / scale);
		float endx = x0 + (float)((float)(id + 1) / scale);
		float dx = endx - px;

		float t = std::clamp(baf, 0.0f, 1.0f);

		int r = c1.red()   + t * (c2.red()   - c1.red());
		int g = c1.green() + t * (c2.green() - c1.green());
		int b = c1.blue()  + t * (c2.blue()  - c1.blue());

		QColor color(r, g, b);

		painter.setPen(color);
		painter.setBrush(color);
		painter.drawRect(px, 0, dx, TRACK_HEIGHT);
		// painter.drawLine(px, 0, px, TRACK_HEIGHT);
	}
}

void BafTrack::drawBarChart(QPainter& painter, const QVector<int>& idxes)
{
	painter.setRenderHint(QPainter::Antialiasing);

	const BedLine& region = SharedData::region();
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;
	int x0 = label_width + 2;
	float scale = (float)region.length() / total_width;

	foreach (int idx, idxes)
	{
		const BedLine& bd = (*bed_file_)[idx];
		if (bd.annotations().count() <= baf_idx_) continue;
		bool ok;
		float baf = bd.annotations()[baf_idx_].toFloat(&ok);
		if (!ok) continue;

		int pos = (bd.start() + bd.end()) / 2;

		if (pos < region.start()) continue;

		int id = pos - region.start();
		float px = x0 + (float)((float)id / scale);
		float endx = x0 + (float)((float)(id + 1) / scale);
		float dx = endx - px;

		float t = std::clamp(baf, 0.0f, 1.0f);

		painter.setPen(Qt::blue);
		painter.setBrush(Qt::blue);
		int py = bafToY(t, TRACK_HEIGHT);
		int pend = bafToY(0.f, TRACK_HEIGHT);
		int height = pend - py;
		painter.drawRect(px, py, dx, height);
	}
}


void BafTrack::drawReferenceLine(QPainter& painter, float baf_value, int x0, int total_width, int total_height)
{
	painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
	int y = bafToY(baf_value, total_height);
	painter.drawLine(x0, y, x0 + total_width, y);
}

int BafTrack::bafToY(float baf, int draw_height)
{
	constexpr int margin = 4;
	return margin + (int)((1.0f - baf) * (draw_height - 2 * margin));
}

void BafTrack::populateContextMenu(QMenu& menu)
{
	QMenu* sub_menu = menu.addMenu("Type Of Graph");
	opts[0] = sub_menu->addAction("Heatmap");
	opts[1] = sub_menu->addAction("Bar Chart");
	opts[2] = sub_menu->addAction("Points");
	opts[3] = sub_menu->addAction("Line Plot");
	TrackWidget::populateContextMenu(menu);
}

void BafTrack::handleContextMenuAction(QAction* action)
{
	if (action == opts[0]) graph_mode_ = HEATMAP;
	else if (action == opts[1]) graph_mode_ = BAR_CHART;
	else if (action == opts[2]) graph_mode_ = POINTS;
	else if (action == opts[3]) graph_mode_ = LINE_PLOT;
	else TrackWidget::handleContextMenuAction(action);
	update();
}
