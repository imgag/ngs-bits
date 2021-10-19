#include "GenePanel.h"
#include "Helper.h"
#include "NGSHelper.h"
#include <QDebug>
#include <QPainter>
#include <QMenu>

//TODO:
// - show current position of cursor (genomic coordinate) and interval size in status bar
// - add translation table for mitochondria: https://www.ncbi.nlm.nih.gov/Taxonomy/Utils/wprintgc.cgi

GenePanel::GenePanel(QWidget *parent)
	: QWidget(parent)
	, strand_forward_(true)
	, show_translation_(false)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));
}

void GenePanel::setDependencies(const FastaFileIndex& genome_idx, const TranscriptList& transcripts)
{
	genome_idx_ = &genome_idx;
	transcripts_ = & transcripts;
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
	a_show_translation->setChecked(show_translation_);

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
}

void GenePanel::paintEvent(QPaintEvent* /*event*/)
{
	//check
	if (genome_idx_==nullptr || transcripts_==nullptr) THROW(ProgrammingException, "Dependencies not set!");

	//init
	int h = height();
	int w = width();
	int w_label = 165;
	int w_content = w-w_label-4;
	int w_content_start = w_label + 2;
	int h_start_content = 2;
	QPainter painter(this);
	QSize char_size = characterSize(painter.font());

	//backgroud
	painter.fillRect(0,0, w,h, QBrush(Qt::white));

	//paint label region
	painter.drawLine(QPoint(w_label, 0), QPoint(w_label, h));
	painter.drawText(QRect(2,2, w_label-4, w_label-4), "Gene");
	painter.drawText(QRect(2,2, w_label-4, w_label-4), Qt::AlignRight|Qt::AlignTop, strand_forward_ ? "→" : "←");

	//paint sequence (only if at lest one pixel per base is available)
	if (reg_.length()<=w_content)
	{
		Sequence seq = genome_idx_->seq(reg_.chr(), reg_.start(), reg_.length());
		if (!strand_forward_) seq.complement();
		double pixels_per_base = (double)w_content / (double)seq.length();
		painter.setPen(Qt::transparent);

		for(int i=0; i<seq.length(); ++i)
		{
			QChar base = seq.at(i);
			QColor color = baseColor(base);
			QRectF rect(w_content_start + i*pixels_per_base, h_start_content, pixels_per_base, char_size.height());
			if (pixels_per_base>=char_size.width()) //show base characters
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

		h_start_content += char_size.height();

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
					if (!strand_forward_) std::reverse(triplet.begin(), triplet.end());
					QChar aa = NGSHelper::translateCodon(triplet);

					//draw rectangle
					QRectF rect(w_content_start + i*pixels_per_base, h_start_content, 3*pixels_per_base, char_size.height());
					QColor color = aaColor(i, aa);
					painter.setBrush(color);
					painter.drawRect(rect);

					if (pixels_per_base*3>=char_size.width()) //show AA character
					{
						painter.drawText(rect, Qt::AlignHCenter|Qt::AlignTop, aa);
					}
				}

				h_start_content += char_size.height();
			}
		}
	}

	//paint genes (preferred transcripts first)

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

