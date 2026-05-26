#include "BafTrack.h"
#include "FileLoader.h"
#include "GenomeVisualizationWidget.h"
#include "SharedData.h"

#include <QPainter>

static constexpr int TRACK_HEIGHT = 200;


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

	const QVector<int>& idxes = chr_index_->matchingIndices(region.chr(), region.start(), region.end());
	foreach (int idx, idxes)
	{
		const BedLine& bd = (*bed_file_)[idx];
		if (bd.annotations().count() <= baf_idx_) continue;
		bool ok;
		float baf = bd.annotations()[baf_idx_].toFloat(&ok);
		if (!ok) continue;

		int pos = (bd.start() + bd.end()) / 2;
		float norm = (float) (pos - region.start())/(region.length());
		int px = x0 + (int)(norm * total_width);
		int py = bafToY(baf, TRACK_HEIGHT);

		painter.setBrush(Qt::blue);
		painter.drawEllipse(QPoint(px, py), 2, 2);
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
