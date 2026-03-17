#include "TrackPanel.h"
#include "SharedData.h"

#include <QPainter>


TrackPanel::TrackPanel(QWidget* parent, Track track)
	:QWidget(parent), track(track), chrIdx(track.bedfile)
{
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(regionChanged()));
}

void TrackPanel::regionChanged()
{
	update();
}

void TrackPanel::paintEvent(QPaintEvent* /*event*/)
{
	const BedLine& region = SharedData::region();
	QPainter painter(this);

	painter.fillRect(rect(), QColor(250, 250, 250));

	int label_width = SharedData::settings().label_width;

	// draw text
	QRectF text_rect(0, 0, label_width-2, height());

	painter.setPen(Qt::black);
	painter.drawText(text_rect, Qt::AlignLeft, track.name);

	// draw bounding box
	QRectF bounding_rect(label_width + 2, 0, width() - label_width - 4, height() - 2);
	QPen outlinePen(QColor(192, 192, 192));
	outlinePen.setWidth(1);
	outlinePen.setStyle(Qt::SolidLine);
	painter.setPen(outlinePen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(bounding_rect);

	if (track.bedfile.chromosomes().count() != 1) return;

	if (region.chr() == track.bedfile.chromosomes().values()[0])
	{

		int w = width();
		float total_width = w - label_width - 4;
		for (int idx =0; idx < track.bedfile.count(); ++idx)
			{
				int st = std::max(track.bedfile[idx].start(), region.start());
				int en = std::min(track.bedfile[idx].end(), region.end());

				float x_start = map(st, region.start(), region.end(), 0.0f, total_width);
				float width = map((float)en - st, 0.0f, (float)region.length(), 0.0f, total_width);
				QRectF chr_rect(label_width + 2 + x_start, 0.0f, width, height() * .3f);
				painter.setBrush(track.color);
				painter.drawRect(chr_rect);
			}
		// }
	}
}
