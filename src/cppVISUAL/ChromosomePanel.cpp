#include "ChromosomePanel.h"
#include "SharedData.h"
#include "NGSHelper.h"
#include <QPainter>
#include <QMouseEvent>

ChromosomePanel::ChromosomePanel(QWidget* parent)
	: QWidget(parent)
{
	setMouseTracking(true);
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
}

void ChromosomePanel::updateRegion()
{
	update();
}


QColor cytobandColor(const QString& stain)
{
	if (stain == "gneg") return QColor(255, 255, 255);
	if (stain == "gpos25") return QColor(200, 200, 200);
	if (stain == "gpos50") return QColor(150, 150, 150);
	if (stain == "gpos75") return QColor(100, 100, 100);
	if (stain == "gpos100") return QColor(0, 0 ,0);
	if (stain == "acen") return QColor(217, 47, 39);
	if (stain == "stalk") return QColor(100, 127, 164);
	if (stain == "gvar") return QColor(180, 180, 255);

	return QColor(220);
}

void ChromosomePanel::paintEvent(QPaintEvent* /*event*/)
{
	//init
	int h = height();
	int w = width();
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();
	static const int text_height = 20;

	pixels_per_base_ = (double)(w-label_width-4) / (double)region.length();

	QPainter painter(this);
	//bands:
	//NGSHelper::cytoBands
	//TODO add band colors

	const BedFile& bands = NGSHelper::cytoBands(GenomeBuild::HG38);

	painter.fillRect(rect(), Qt::white);

	/*
	 * TODO: it's much better to build a hash map per chromosome
	 * that maps to a list
	 */

	int num_bands =0;
	for (int i =0; i < bands.count(); ++i){
		if (bands[i].chr() == region.chr()){
			num_bands++;
		}
	}

	if (num_bands >= 1)
	{
		double band_width = (double)(w-label_width-4) / (double)num_bands;
		QTextStream(stdout) << region.chr().str() << ' ' << num_bands << ' ' << bands[0].annotations().size() << Qt::endl;
		int current_band_idx = 0;
		static char prev_arm = ' ';
		char current_arm;
		for (int i =0; i < bands.count(); ++i)
		{
			//draw chromosome of only current region
			if (bands[i].chr() == region.chr())
			{
				current_arm = bands[i].annotations()[0].at(0);

				QRectF rect(label_width + 2 + current_band_idx * band_width, 2, band_width, h-text_height);
				if (bands[i].annotations().size() > 1)
				{
					painter.setBrush(cytobandColor(bands[i].annotations()[1]));
				}
				else
				{
					painter.setBrush((i % 2 == 0) ? Qt::black : Qt::darkGray);
				}

				if (current_arm == 'p' && i < bands.count()-1 && bands[i+1].annotations()[0].at(0) == 'q')
				{
					float p_start_x = label_width + 2 + (current_band_idx) * band_width;
					QPolygonF p_tri;
					p_tri << QPointF(p_start_x, 2)
						  << QPointF(p_start_x + band_width, h/2.0 - text_height/2.0)
						  << QPointF(p_start_x, h - text_height);
					painter.drawPolygon(p_tri);
				}
				else if (prev_arm == 'p' && current_arm == 'q')
				{

					float q_start_x = label_width + 2 + (current_band_idx) * band_width;
					QPolygonF q_tri;
					q_tri << QPointF(q_start_x + band_width, 2)
						  << QPointF(q_start_x, h/2.0 - text_height/2.0)
						  << QPointF(q_start_x + band_width, h - text_height);
					painter.drawPolygon(q_tri);
				}

				else
				{
					painter.drawRect(rect);
				}

				prev_arm = current_arm;

				QRectF text_rect(label_width + 2 + current_band_idx * band_width, 2 + h - text_height, band_width, text_height);
				painter.setPen(Qt::black);
				painter.drawText(text_rect, Qt::AlignCenter, bands[i].annotations()[0]);

				current_band_idx++;
			}
		}
	}
}

void ChromosomePanel::mouseMoveEvent(QMouseEvent* event)
{
	//init
	int x = event->pos().x();
	int w = width();
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();

	//show
	if (x>label_width + 2 && x<w - 2)
	{
		int coordinate = region.start() + std::floor((double)(x-label_width - 2) / pixels_per_base_);
		emit mouseCoordinate(region.chr().strNormalized(true) + ":" + QString::number(coordinate));
	}
	else
	{
		emit mouseCoordinate("");
	}
}
