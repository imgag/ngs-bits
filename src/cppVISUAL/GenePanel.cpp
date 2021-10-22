#include "GenePanel.h"
#include "Helper.h"
#include "NGSHelper.h"
#include <QDebug>
#include <QPainter>
#include <QMenu>

GenePanel::GenePanel(QWidget *parent)
	: QWidget(parent)
	, settings_()
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	setMouseTracking(true);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));
}

void GenePanel::setDependencies(const FastaFileIndex& genome_idx, const TranscriptList& transcripts)
{
	genome_idx_ = &genome_idx;
	transcripts_ = & transcripts;
	transcripts_idx_ = new ChromosomalIndex<TranscriptList>(*transcripts_);
}

void GenePanel::setRegion(const BedLine& region)
{
	reg_ = region;
	update();
}

void GenePanel::contextMenu(QPoint pos)
{
	//create menu
	QMenu menu(this);
	QAction* a_flip_strand = menu.addAction("Flip strand");
	QAction* a_show_translation = menu.addAction("Show translation");
	a_show_translation->setCheckable(true);
	a_show_translation->setChecked(settings_.show_translation_);

	//show menu
	QAction* action = menu.exec(mapToGlobal(pos));

	//perform action
	if (action==a_flip_strand)
	{
		settings_.strand_forward_ = !settings_.strand_forward_;
		update();
	}
	else if (action==a_show_translation)
	{
		settings_.show_translation_ = !settings_.show_translation_;
		update();
	}
}

void GenePanel::paintEvent(QPaintEvent* /*event*/)
{
	//check
	if (genome_idx_==nullptr || transcripts_==nullptr) THROW(ProgrammingException, "Dependencies not set!");

	//init
	int h = height();
	int w = width();
	QPainter painter(this);
	char_size_ = characterSize(painter.font());
	pixels_per_base_ = (double)(w-settings_.label_width-4) / (double)reg_.length();
	int h_start_content = 2;

	//backgroud
	painter.fillRect(0,0, w,h, QBrush(Qt::white));

	//paint label region
	painter.drawLine(QPoint(settings_.label_width, 0), QPoint(settings_.label_width, h));
	painter.drawText(QRect(2,2, settings_.label_width-4, settings_.label_width-4), "Gene");
	painter.drawText(QRect(2,2, settings_.label_width-4, settings_.label_width-4), Qt::AlignRight|Qt::AlignTop, settings_.strand_forward_ ? "→" : "←");

	//paint sequence (only if at lest one pixel per base is available)
	if (pixels_per_base_ >= 1)
	{
		Sequence seq = genome_idx_->seq(reg_.chr(), reg_.start(), reg_.length());
		if (!settings_.strand_forward_) seq.complement();
		painter.setPen(Qt::transparent);

		for(int i=0; i<seq.length(); ++i)
		{
			QChar base = seq.at(i);
			QColor color = baseColor(base);
			QRectF rect(settings_.label_width + 2 + i*pixels_per_base_, h_start_content, pixels_per_base_, char_size_.height());
			if (pixels_per_base_>=char_size_.width()) //show base characters
			{
				painter.setPen(color);
				painter.drawText(rect, Qt::AlignHCenter|Qt::AlignTop, base);
			}
			else //show bases as colored line
			{
				painter.setBrush(color);
				painter.drawRect(rect);
			}
		}

		h_start_content += char_size_.height();

		//paint tranlations
		if (settings_.show_translation_)
		{
			painter.setPen(Qt::white);
			for (int offset=0; offset<3; ++offset)
			{
				for(int i=offset; i<seq.length(); i+=3)
				{
					QByteArray triplet = seq.mid(i, 3);
					if(triplet.length()<3) continue; //right border
					if (triplet.contains('N')) continue; //N region
					if (!settings_.strand_forward_) std::reverse(triplet.begin(), triplet.end());
					QChar aa = NGSHelper::translateCodon(triplet, reg_.chr().isM());

					//draw rectangle
					QRectF rect(settings_.label_width + 2 + i*pixels_per_base_, h_start_content, 3*pixels_per_base_, char_size_.height());
					QColor color = aaColor(i, aa);
					painter.setBrush(color);
					painter.drawRect(rect);

					if (pixels_per_base_*3>=char_size_.width()) //show AA character
					{
						painter.drawText(rect, Qt::AlignHCenter|Qt::AlignTop, aa);
					}
				}

				h_start_content += char_size_.height();
			}
		}
	}
	h_start_content += 2;

	//paint preferred transcripts;
	QVector<int> trans_indices = transcripts_idx_->matchingIndices(reg_.chr(), reg_.start(), reg_.end());
	foreach(int i, trans_indices)
	{
		const Transcript& trans = transcripts_->at(i);
		if (!trans.isPreferredTranscript()) continue;
		drawTranscript(painter, trans, h_start_content, QColor(130, 0, 50));
	}

	//paint other transcripts
	//TODO
	/*
	int trans_height = 2+12+2+char_size_.height()+2;

	QVector<int> trans_indices = transcripts_idx_->matchingIndices(reg_.chr(), reg_.start(), reg_.end());
	foreach(int i, trans_indices)
	{
		const Transcript& trans = transcripts_->at(i);
		if (!trans.isPreferredTranscript()) continue;
		drawTranscript(painter, trans, h_start_content, QColor());
	}
	*/
}

void GenePanel::mouseMoveEvent(QMouseEvent* event)
{
	int x = event->pos().x();

	int w = width();

	//show
	if (x>settings_.label_width + 2 && x<w - 2)
	{
		int coordinate = reg_.start() + std::floor((double)(x-settings_.label_width - 2) / pixels_per_base_);
		emit mouseCoordinate(reg_.chr().strNormalized(true) + ":" + QString::number(coordinate));
	}
	else
	{
		emit mouseCoordinate("");
	}
}

QSize GenePanel::characterSize(QFont font)
{
	QFontMetrics fm(font);

	int w = -1;
	w = std::max(w, fm.boundingRect("A").width());
	w = std::max(w, fm.boundingRect("C").width());
	w = std::max(w, fm.boundingRect("G").width());
	w = std::max(w, fm.boundingRect("T").width());
	w = std::max(w, fm.boundingRect("N").width());

	int h = -1;
	h = std::max(h, fm.boundingRect("C").height());
	h = std::max(h, fm.boundingRect("A").height());
	h = std::max(h, fm.boundingRect("G").height());
	h = std::max(h, fm.boundingRect("T").height());
	h = std::max(h, fm.boundingRect("N").height());

	return QSize(w, h);
}

QColor GenePanel::baseColor(QChar base)
{
	if (base=='A' || base=='a') return QColor(0, 150, 0);
	if (base=='C' || base=='c') return QColor(0, 0, 255);
	if (base=='G' || base=='g') return QColor(209, 113, 5);
	if (base=='T' || base=='t') return QColor(255, 0, 0);
	if (base=='N' || base=='n') return QColor(128, 128, 128);

	return Qt::black;
}

QColor GenePanel::aaColor(int start_index, QChar aa)
{
	//special color start/stop
	if (aa=='*') return QColor(255, 0, 0);
	if (aa=='M') return QColor(0, 255, 0);

	//alternating light/dark grey
	return (start_index%2==0 ? QColor(128, 128, 128) : QColor(170, 170, 170));
}

double GenePanel::baseStartX(int pos, bool restrict_to_content_area) const
{
	int w = width();

	double x = settings_.label_width + 2 + (pos-reg_.start())*pixels_per_base_;

	if (restrict_to_content_area)
	{
		x = BasicStatistics::bound(x, (double)(settings_.label_width + 2), (double)(w - 2));
	}

	return x;
}

double GenePanel::baseEndX(int pos, bool restrict_to_content_area) const
{
	int w = width();

	double x = settings_.label_width + 2 + ((pos+1)-reg_.start())*pixels_per_base_;

	if (restrict_to_content_area)
	{
		x = BasicStatistics::bound(x, (double)(settings_.label_width + 2), (double)(w - 2));
	}

	return x;
}

void GenePanel::drawTranscript(QPainter& painter, const Transcript& trans, int y, QColor color)
{

	//draw gene name (at the horizontal center of the visual part of the transcript)
	double x_start = baseStartX(trans.start(), true);
	double x_end = baseEndX(trans.end(), true);
	painter.setPen(Qt::black);
	QRectF rect(x_start, y+2+12+2, x_end-x_start, char_size_.height());
	painter.drawText(rect, Qt::AlignCenter, trans.gene());

	//paint center line
	painter.setPen(color);
	double y_center = y+2+6;
	painter.drawLine(QLineF(x_start+1.0, y_center, x_end-1.0, y_center));

	//draw all exons (8px heigh)
	painter.setPen(Qt::transparent);
	painter.setBrush(color);
	for(int i=0; i<trans.regions().count(); ++i)
	{
		const BedLine& exon = trans.regions()[i];
		double x_start = baseStartX(exon.start(), true);
		double x_end = baseEndX(exon.end(), true);
		QRectF rect(x_start, y+2+2, x_end-x_start, 8);
		painter.drawRect(rect);
	}

	//draw coding exons (12px heigh)
	for(int i=0; i<trans.regions().count(); ++i)
	{
		const BedLine& exon = trans.codingRegions()[i];
		double x_start = baseStartX(exon.start(), true);
		double x_end = baseEndX(exon.end(), true);
		QRectF rect(x_start, y+2, x_end-x_start, 12);
		painter.drawRect(rect);
	}

}
