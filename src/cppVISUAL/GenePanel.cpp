#include "GenePanel.h"
#include "NGSHelper.h"
#include <QPainter>
#include <QMenu>
#include <QToolTip>
#include "SharedData.h"

GenePanel::GenePanel(QWidget *parent)
	: QWidget(parent)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	setMouseTracking(true);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
}

void GenePanel::contextMenu(QPoint pos)
{
	//create menu
	QMenu menu(this);
	QAction* a_flip_strand = menu.addAction("Flip strand");
	QAction* a_show_translation = menu.addAction("Show translation");
	a_show_translation->setCheckable(true);
	a_show_translation->setChecked(show_translation_);
	QAction* a_show_ensebl_only = menu.addAction("Show GENCODE primary only");
	a_show_ensebl_only->setCheckable(true);
	a_show_ensebl_only->setChecked(SharedData::settings().show_only_primary);

	//show menu
	QAction* action = menu.exec(mapToGlobal(pos));

	//perform action
	if (action==a_flip_strand)
	{
		strand_forward_ = !strand_forward_;
		update();
	}
	else if (action==a_show_translation)
	{
		show_translation_ = !show_translation_;
		update();
	}
	else if (action==a_show_ensebl_only)
	{
		GlobalSettings settings = SharedData::settings();
		settings.show_only_primary = !settings.show_only_primary;
		SharedData::setSettings(settings);
	}
}

void GenePanel::updateRegion()
{
	update();
}

void GenePanel::paintEvent(QPaintEvent* /*event*/)
{
	//init
	int h = height();
	int w = width();
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();
	QPainter painter(this);
	char_size_ = characterSize(painter.font());
	pixels_per_base_ = (double)(w-label_width-4) / (double)region.length();
	int y_content_start = 2;

	//backgroud
	painter.fillRect(0,0, w,h, QBrush(Qt::white));

	//paint label region
	painter.drawLine(QPoint(label_width, 0), QPoint(label_width, h));
	painter.drawText(QRect(2,2, label_width-4, label_width-4), "Gene");
	painter.drawText(QRect(2,2, label_width-4, label_width-4), Qt::AlignRight|Qt::AlignTop, strand_forward_ ? "→" : "←");

	//paint sequence (only if at lest one pixel per base is available)
	if (pixels_per_base_ >= 1)
	{
		Sequence seq = SharedData::genome().seq(region.chr(), region.start(), region.length());
		if (!strand_forward_) seq.complement();
		painter.setPen(Qt::transparent);

		for(int i=0; i<seq.length(); ++i)
		{
			QChar base = seq.at(i);
			QColor color = baseColor(base);
			QRectF rect(label_width + 2 + i*pixels_per_base_, y_content_start, pixels_per_base_, char_size_.height());
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

		y_content_start += char_size_.height();

		//paint tranlations
		if (show_translation_)
		{
			painter.setPen(Qt::white);
			for (int offset=0; offset<3; ++offset)
			{
				for(int i=offset; i<seq.length(); i+=3)
				{
					QByteArray triplet = seq.mid(i, 3);
					if(triplet.length()<3) continue; //right border
					if (triplet.contains('N')) continue; //N region
					if (!strand_forward_) std::reverse(triplet.begin(), triplet.end());
					QChar aa = NGSHelper::translateCodon(triplet, region.chr().isM());

					//draw rectangle
					QRectF rect(label_width + 2 + i*pixels_per_base_, y_content_start, 3*pixels_per_base_, char_size_.height());
					QColor color = aaColor(i, aa);
					painter.setBrush(color);
					painter.drawRect(rect);

					if (pixels_per_base_*3>=char_size_.width()) //show AA character
					{
						painter.drawText(rect, Qt::AlignHCenter|Qt::AlignTop, aa);
					}
				}

				y_content_start += char_size_.height();
			}
		}
	}

	//init for painting transcripts
	y_content_start += 2;
	trans_positions_.clear();

    //paint important transcripts on top
	QVector<int> trans_indices = SharedData::transcriptsInRegion(region.chr(), region.start(), region.end());
    QSet<int> rest;
	foreach(int i, trans_indices)
	{
		const Transcript& trans = SharedData::transcripts().at(i);

        //only GENCODE primary
		if (SharedData::settings().show_only_primary && !trans.isGencodePrimaryTranscript()) continue;

        if (trans.isPreferredTranscript() || trans.isManePlusClinicalTranscript() || trans.isManeSelectTranscript() || trans.isEnsemblCanonicalTranscript())
        {
            drawTranscript(painter, trans, y_content_start, QColor(130, 0, 50));
        }
        else
        {
            rest << i;
        }
	}

    //paint other transcripts
    foreach(int i, rest)
	{
		const Transcript& trans = SharedData::transcripts().at(i);
		drawTranscript(painter, trans, y_content_start, QColor(0, 0, 178));
	}
}

void GenePanel::mouseMoveEvent(QMouseEvent* event)
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

	event->ignore();
}

bool GenePanel::event(QEvent* event)
{
	if (event->type()==QEvent::ToolTip)
	{
		QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
		QPoint pos = helpEvent->pos();
		bool tooltip_shown = false;
		foreach(const TranscriptPosition& trans_pos, trans_positions_)
		{
			if (trans_pos.rect.contains(pos))
			{
                QString text = "<nobr><b>"+trans_pos.trans.gene()+" ("+trans_pos.trans.name()+")</b></nobr>";
                QString tags = trans_pos.trans.flags(false).join(", ").trimmed();
                if (!tags.isEmpty()) text += "<br><nobr>Tags: "+tags+"</nobr>";
                QToolTip::showText(helpEvent->globalPos(), text);
				tooltip_shown = true;
				break;
			}
		}
		if (!tooltip_shown)
		{
			QToolTip::hideText();
			event->ignore();
		}

		return true;
	}

	return QWidget::event(event);
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
	int label_width = SharedData::settings().label_width;

	double x = label_width + 2 + (pos-SharedData::region().start())*pixels_per_base_;

	if (restrict_to_content_area)
	{
		x = BasicStatistics::bound(x, (double)(label_width + 2), (double)(w - 2));
	}

	return x;
}

double GenePanel::baseEndX(int pos, bool restrict_to_content_area) const
{
	int w = width();
	int label_width = SharedData::settings().label_width;

	double x = label_width + 2 + ((pos+1)-SharedData::region().start())*pixels_per_base_;

	if (restrict_to_content_area)
	{
		x = BasicStatistics::bound(x, (double)(label_width + 2), (double)(w - 2));
	}

	return x;
}

void GenePanel::drawTranscript(QPainter& painter, const Transcript& trans, int y_content_start, QColor color)
{
	//determine x start/end
	double x_start = baseStartX(trans.start(), true);
	double x_end = baseEndX(trans.end(), true);

	//determine where to paint in y-axis
	int trans_height = 2+12+2+char_size_.height()+2; //margin, transcript, margin, label, margin
	int y = transcriptY(x_start, x_end, y_content_start, trans_height, trans);

	//draw gene name (at the horizontal center of the visual part of the transcript, only if not larger than transcript itself)
	painter.setPen(Qt::black);
	QRectF rect(x_start, y+2+12+2, x_end-x_start, char_size_.height());
	if  (trans.gene().length()*char_size_.width()<rect.width())
	{
		painter.drawText(rect, Qt::AlignCenter, trans.gene());
	}

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
	for(int i=0; i<trans.codingRegions().count(); ++i)
	{
		const BedLine& exon = trans.codingRegions()[i];
		double x_start = baseStartX(exon.start(), true);
		double x_end = baseEndX(exon.end(), true);
		QRectF rect(x_start, y+2, x_end-x_start, 12);
		painter.drawRect(rect);
	}

}

int GenePanel::transcriptY(double x_start, double x_end, int y_content_start, int trans_height, const Transcript& trans)
{
	//determine first row index without overlap with previously painted transcripts
	int row = -1;
	bool overlap_exists = true;
	while (overlap_exists)
	{
		++row;

		//check if overlap exists
		overlap_exists = false;
		foreach(const TranscriptPosition& trans_pos, trans_positions_)
		{
			if (trans_pos.row==row && x_start<=(trans_pos.rect.x()+trans_pos.rect.width()) && x_end>=trans_pos.rect.x())
			{
				overlap_exists = true;
			}
		}
	}

	//calculate y-coord start
	double y_start = y_content_start + trans_height * row;

	//store transcript position
	trans_positions_.append(TranscriptPosition{trans, row, QRectF(x_start, y_start, x_end-x_start, trans_height)});

	return y_start;
}
