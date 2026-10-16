#include "GenePanel.h"
#include "NGSHelper.h"
#include <QPainter>
#include <QMenu>
#include <QApplication>
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include "SharedData.h"

namespace
{
Sequence codingSequence(const Transcript& trans)
{
	Sequence sequence;
	for (int i = 0; i < trans.codingRegions().count(); ++i)
	{
		const BedLine& exon = trans.codingRegions()[i];
		sequence += SharedData::genome().seq(exon.chr(), exon.start(), exon.length());
	}
	if (trans.isMinusStrand()) sequence.reverseComplement();
	return sequence;
}

//One-based position along spliced regions in transcript direction, or zero in an intron.
int splicedPosition(const BedFile& regions, int genome_pos, bool plus)
{
	int offset = 0;
	for (int j = 0; j < regions.count(); ++j)
	{
		const BedLine& exon = regions[plus ? j : regions.count() - 1 - j];
		if (genome_pos >= exon.start() && genome_pos <= exon.end()) return offset + (plus ? genome_pos - exon.start() : exon.end() - genome_pos) + 1;
		offset += exon.length();
	}
	return 0;
}
}

GenePanel::GenePanel(QWidget *parent)
	: QWidget(parent)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
	connect(SharedData::instance(), &SharedData::transcriptsChanged, this, &GenePanel::updateRegion);
	connect(SharedData::instance(), &SharedData::settingsChanged, this, &GenePanel::updateRegion);
	updateTranscriptLayout();
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
	a_show_ensebl_only->setChecked(show_only_gencode_primary);

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
		updateRegion();
	}
	else if (action==a_show_ensebl_only)
	{
		show_only_gencode_primary = !show_only_gencode_primary;
		updateRegion();
	}
}

void GenePanel::updateRegion()
{
	updateTranscriptLayout();
	update();
}

void GenePanel::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	updateTranscriptLayout();
}

void GenePanel::changeEvent(QEvent* event)
{
	QWidget::changeEvent(event);
	if (event->type() == QEvent::FontChange) updateRegion();
}

void GenePanel::updateTranscriptLayout()
{
	const BedLine& region = SharedData::region();
	char_size_ = characterSize(font());
	amino_acid_width_ = 0;
	const QFontMetrics metrics(font());
	for (QChar amino_acid : QString("ACDEFGHIKLMNPQRSTVWY*"))
	{
		amino_acid_width_ = std::max(amino_acid_width_, metrics.horizontalAdvance(amino_acid));
	}
	pixels_per_base_ = (double)(width() - SharedData::settings().label_width - 4) / region.length();
	int y_content_start = 4;
	if (pixels_per_base_ >= 1 && region.chr() != "")
	{
		y_content_start += char_size_.height() * (show_translation_ ? 4 : 1);
	}
	const int trans_height = 2+12+2+char_size_.height()+2;
	trans_positions_.clear();
	QVector<int> indices = SharedData::transcriptsInRegion(region.chr(), region.start(), region.end());
	QVector<int> rest;
	auto place_transcript = [&](const Transcript& trans)
	{
		transcriptY(baseStartX(trans.start(), true), baseEndX(trans.end(), true), y_content_start, trans_height, trans);
		updateCodingAminoAcids(trans_positions_.last());
	};
	for (int i : indices)
	{
		const Transcript& trans = SharedData::transcripts().at(i);
		if (show_only_gencode_primary && !trans.isGencodePrimaryTranscript()) continue;
		if (trans.isPreferredTranscript() || trans.isManePlusClinicalTranscript() || trans.isManeSelectTranscript() || trans.isEnsemblCanonicalTranscript()) place_transcript(trans);
		else rest.append(i);
	}
	for (int i : rest) place_transcript(SharedData::transcripts().at(i));
	int content_height = y_content_start;
	for (const TranscriptPosition& position : trans_positions_)
	{
		content_height = std::max(content_height, (int)position.rect.bottom() + 2);
	}
	setMinimumHeight(content_height);
}

void GenePanel::updateCodingAminoAcids(TranscriptPosition& position)
{
	const Transcript& trans = position.trans;
	if (!trans.isCoding() || 3 * pixels_per_base_ < amino_acid_width_ + 2) return;

	//Concatenate CDS exons before translating, so split codons retain their frame.
	const Sequence coding_sequence = codingSequence(trans);
	const BedLine& region = SharedData::region();
	int prefix = 0;
	for (int i = 0; i < trans.codingRegions().count(); ++i)
	{
		const BedLine& exon = trans.codingRegions()[i];
		const int cds_start = trans.isPlusStrand() ? prefix : coding_sequence.size() - prefix - exon.length();
		prefix += exon.length();
		const int visible_start = std::max(exon.start(), region.start());
		const int visible_end = std::min(exon.end(), region.end());
		if (visible_start > visible_end) continue;
		const int first = cds_start + (trans.isPlusStrand() ? visible_start - exon.start() : exon.end() - visible_end);
		const int last = cds_start + (trans.isPlusStrand() ? visible_end - exon.start() : exon.end() - visible_start);
		for (int codon = first - first % 3; codon <= last && codon + 3 <= coding_sequence.size(); codon += 3)
		{
			const QByteArray triplet = coding_sequence.mid(codon, 3);
			if (triplet.contains('N') || std::any_of(triplet.begin(), triplet.end(), [](char base) { return base != 'A' && base != 'C' && base != 'G' && base != 'T'; })) continue;
			const char amino_acid = NGSHelper::translateCodon(triplet, trans.chr().isM());
			const int fragment_start = std::max(codon, cds_start) - cds_start;
			const int fragment_end = std::min(codon + 2, cds_start + exon.length() - 1) - cds_start;
			const int start = trans.isPlusStrand() ? exon.start() + fragment_start : exon.end() - fragment_end;
			const int end = trans.isPlusStrand() ? exon.start() + fragment_end : exon.end() - fragment_start;
			const double x_start = baseStartX(std::max(start, visible_start), true);
			const double x_end = baseEndX(std::min(end, visible_end), true);
			if (x_end <= x_start) continue;
			position.amino_acids.append({QRectF(x_start, position.rect.top() + 2, x_end - x_start, 12), amino_acid, codon / 3, x_end - x_start >= amino_acid_width_ + 2});
		}
	}
}

void GenePanel::paintEvent(QPaintEvent* /*event*/)
{
	//init
	int h = height();
	int w = width();
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	int y_content_start = 2;

	//backgroud
	painter.fillRect(0,0, w,h, QBrush(Qt::white));

	//paint label region
	painter.drawLine(QPoint(label_width, 0), QPoint(label_width, h));
	painter.drawText(QRect(2,2, label_width-4, label_width-4), "Gene");
	painter.drawText(QRect(2,2, label_width-4, label_width-4), Qt::AlignRight|Qt::AlignTop, strand_forward_ ? "→" : "←");

	//paint sequence (only if at lest one pixel per base is available)
	if (pixels_per_base_ >= 1 && region.chr() != "")
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
					if (triplet.contains('N')) continue;
					if (!strand_forward_) std::reverse(triplet.begin(), triplet.end());
					QChar aa = NGSHelper::translateCodon(triplet, region.chr().isM());

					//draw rectangle
					QRectF rect(label_width + 2 + i*pixels_per_base_, y_content_start, 3*pixels_per_base_, char_size_.height());
					QColor color = aaColor(i, aa);
					painter.setBrush(color);
					painter.drawRect(rect);

					if (rect.width() >= amino_acid_width_ + 2) //same width and margins as exon AAs
					{
						painter.drawText(rect.adjusted(1, 0, -1, 0), Qt::AlignHCenter|Qt::AlignTop, aa);
					}
				}

				y_content_start += char_size_.height();
			}
		}
	}

	//Paint the layout used to determine the scrollable content height.
	for (const TranscriptPosition& position : trans_positions_)
	{
		const Transcript& trans = position.trans;
		const bool important = trans.isPreferredTranscript() || trans.isManePlusClinicalTranscript() || trans.isManeSelectTranscript() || trans.isEnsemblCanonicalTranscript();
		drawTranscript(painter, position, important ? QColor(130, 0, 50) : QColor("#0000B2"));
	}
}

void GenePanel::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) mouse_press_pos_ = event->pos();
	QWidget::mousePressEvent(event);
}

void GenePanel::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && (event->pos() - mouse_press_pos_).manhattanLength() < QApplication::startDragDistance() && pixels_per_base_ > 0)
	{
		const int x0 = SharedData::settings().label_width + 2;
		if (event->pos().x() >= x0 && event->pos().x() < width() - 2)
		{
			for (const TranscriptPosition& position : trans_positions_)
			{
				if (!position.rect.contains(event->pos())) continue;
				const int genome_pos = SharedData::region().start() + (int)((event->pos().x() - x0) / pixels_per_base_);
				auto* popup = new QDialog(this, Qt::Popup);
				popup->setAttribute(Qt::WA_DeleteOnClose);
				auto* label = new QLabel(transcriptText(position.trans, genome_pos), popup);
				label->setTextInteractionFlags(Qt::TextSelectableByMouse);
				auto* layout = new QVBoxLayout(popup);
				layout->setContentsMargins(4, 4, 4, 4);
				layout->addWidget(label);
				popup->move(event->globalPosition().toPoint());
				popup->show();
				break;
			}
		}
	}
	QWidget::mouseReleaseEvent(event);
}

QString GenePanel::transcriptText(const Transcript& trans, int genome_pos) const
{
	const int transcript_pos = splicedPosition(trans.regions(), genome_pos, trans.isPlusStrand());
	const int coding_pos = splicedPosition(trans.codingRegions(), genome_pos, trans.isPlusStrand());
	QString cdna = "N/A (intron)";
	QString aa_position = "N/A";
	QString aa_one = "N/A";
	QString aa_three = "N/A";
	if (transcript_pos > 0)
	{
		if (!trans.isCoding()) cdna = "n." + QString::number(transcript_pos);
		else if (coding_pos > 0) cdna = "c." + QString::number(coding_pos);
		else
		{
			const int coding_start = splicedPosition(trans.regions(), trans.codingStart(), trans.isPlusStrand());
			const int coding_end = splicedPosition(trans.regions(), trans.codingEnd(), trans.isPlusStrand());
			cdna = transcript_pos < coding_start ? "c.-" + QString::number(coding_start - transcript_pos) : "c.*" + QString::number(transcript_pos - coding_end);
		}
	}
	if (coding_pos > 0)
	{
		const int codon_index = (coding_pos - 1) / 3;
		aa_position = QString::number(codon_index + 1);
		const QByteArray triplet = codingSequence(trans).mid(codon_index * 3, 3);
		if (triplet.size() == 3 && std::all_of(triplet.begin(), triplet.end(), [](char base) { return base == 'A' || base == 'C' || base == 'G' || base == 'T'; }))
		{
			const char aa = NGSHelper::translateCodon(triplet, trans.chr().isM());
			aa_one = QString(QChar(aa));
			aa_three = QString::fromUtf8(NGSHelper::threeLetterCode(aa));
		}
	}
	QString text = "<b>Gene:</b> " + QString::fromUtf8(trans.gene()).toHtmlEscaped()
		+ "<br><b>Transcript:</b> " + QString::fromUtf8(trans.name()).toHtmlEscaped();
	const QString tags = trans.flags(false).join(", ");
	if (!tags.isEmpty()) text += "<br><b>Tags:</b> " + tags.toHtmlEscaped();
	QStringList details;
	if (!cdna.startsWith("N/A")) details.append("<b>cDNA position:</b> " + cdna);
	if (aa_position != "N/A") details.append("<b>AA position:</b> " + aa_position);
	if (aa_one != "N/A") details.append("<b>AA (one-letter):</b> " + aa_one);
	if (aa_three != "N/A") details.append("<b>AA (three-letter):</b> " + aa_three);
	if (!details.isEmpty()) text += "<br><br>" + details.join("<br>");
	return text;
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

void GenePanel::drawTranscript(QPainter& painter, const TranscriptPosition& position, QColor color)
{
	const Transcript& trans = position.trans;
	const double x_start = position.rect.left();
	const double x_end = position.rect.right();
	const int y = position.rect.top();

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

	//Function to draw strand chevrons in the given visible interval.
	auto draw_strand_arrows = [&](double start, double end, const QColor& arrow_color)
	{
		if ((!trans.isPlusStrand() && !trans.isMinusStrand()) || end - start < 4.0) return;
		painter.setPen(arrow_color);
		const double direction = trans.isPlusStrand() ? 1.0 : -1.0;
		constexpr double arrow_spacing = 45.0;
		constexpr double arrow_width = 3.0;
		constexpr double arrow_height = 2.5;
		for (double x = start + std::min(arrow_spacing / 2, (end - start) / 2); x + arrow_width / 2 < end; x += arrow_spacing)
		{
			const double tail_x = x - direction * arrow_width / 2;
			const double tip_x = x + direction * arrow_width / 2;
			painter.drawLine(QLineF(tail_x, y_center - arrow_height, tip_x, y_center));
			painter.drawLine(QLineF(tail_x, y_center + arrow_height, tip_x, y_center));
		}
	};

	//Exon boxes cover the intron chevrons below.
	draw_strand_arrows(x_start, x_end, color);

	//draw all exons (8px heigh)
	painter.setPen(Qt::transparent);
	painter.setBrush(color);
	for(int i=0; i<trans.regions().count(); ++i)
	{
		const BedLine& exon = trans.regions()[i];
		double x_start = baseStartX(exon.start(), true);
		double x_end = baseEndX(exon.end(), true);
		if (x_end <= x_start) continue;
		QRectF rect(x_start, y+2+2, std::max(0.5, x_end-x_start), 8);
		painter.drawRect(rect);
	}

	//draw coding exons (12px heigh)
	for(int i=0; i<trans.codingRegions().count(); ++i)
	{
		const BedLine& exon = trans.codingRegions()[i];
		double x_start = baseStartX(exon.start(), true);
		double x_end = baseEndX(exon.end(), true);
		if (x_end <= x_start) continue;
		QRectF rect(x_start, y+2, std::max(0.5, x_end-x_start), 12);
		painter.drawRect(rect);
	}

	//Shade whole codon fragments, even when a fragment is too narrow for its letter.
	const QColor light = color == QColor("#0000B2") ? QColor("#5C5CA4") : color;
	const QColor darker = color == QColor("#0000B2") ? QColor("#0C0C78") : color.darker(130);
	for (const AminoAcidPosition& amino_acid : position.amino_acids)
	{
		QColor background = amino_acid.codon_index % 2 == 0 ? light : darker;
		if ((amino_acid.amino_acid == 'M' && amino_acid.codon_index == 0) || amino_acid.amino_acid == '*') background = aaColor(amino_acid.codon_index, QChar(amino_acid.amino_acid));
		painter.fillRect(amino_acid.rect, background);
	}

	//Wide exons show strand arrows when no amino-acid letters are drawn.
	const bool has_amino_acid_letters = std::any_of(position.amino_acids.begin(), position.amino_acids.end(), [](const AminoAcidPosition& aa) { return aa.draw_letter; });
	if (!has_amino_acid_letters)
	{
		for (int i = 0; i < trans.regions().count(); ++i)
		{
			const BedLine& exon = trans.regions()[i];
			const double start = baseStartX(exon.start(), true);
			const double end = baseEndX(exon.end(), true);
			if (end - start >= 40.0) draw_strand_arrows(start, end, Qt::white);
		}
	}

	//Amino acids are cached with the layout; painting does not re-read the reference.
	painter.save();
	painter.setPen(Qt::white);
	for (const AminoAcidPosition& amino_acid : position.amino_acids)
	{
		if (!amino_acid.draw_letter) continue;
		painter.save();
		painter.setClipRect(amino_acid.rect, Qt::IntersectClip);
		painter.drawText(amino_acid.rect.adjusted(1, 0, -1, 0), Qt::AlignCenter, QString(QChar(amino_acid.amino_acid)));
		painter.restore();
	}
	painter.restore();
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
	trans_positions_.append(TranscriptPosition{trans, row, QRectF(x_start, y_start, x_end-x_start, trans_height), {}});

	return y_start;
}
