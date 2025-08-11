#include "ui_GenomeVisualizationWidget.h"
#include "GenomeVisualizationWidget.h"
#include "BedFile.h"
#include "GUIHelper.h"
#include <QToolTip>
#include <QDebug>
#include <QMessageBox>

GenomeVisualizationWidget::GenomeVisualizationWidget(QWidget* parent, const FastaFileIndex& genome_idx, const TranscriptList& transcripts)
	: QWidget(parent)
	, ui_(new Ui::GenomeVisualizationWidget)
	, settings_()
	, genome_idx_(genome_idx)
	, transcripts_(transcripts)
	, valid_chrs_()
	, gene_to_trans_indices_()
	, trans_to_index_()
	, current_reg_()
{
	ui_->setupUi(this);
	GUIHelper::styleSplitter(ui_->splitter_gene_panel);

	//init panels
	ui_->gene_panel->setDependencies(genome_idx_, transcripts_);

	//init chromosome list (ordered correctly)
	foreach(const Chromosome& chr, genome_idx_.chromosomes())
	{
		valid_chrs_ << chr.str();
	}
	ui_->chr_selector->addItems(valid_chrs_);

	//init gene and transcript list
	for(int i=0; i<transcripts_.size(); ++i)
	{
		const Transcript& trans = transcripts_[i];

		if (trans.source()!=Transcript::ENSEMBL) continue;

		gene_to_trans_indices_[trans.gene()] << i;
		trans_to_index_[trans.name()] = i;
	}

	//connect signals and slots
	connect(ui_->chr_selector, SIGNAL(currentTextChanged(QString)), this, SLOT(setChromosomeRegion(QString)));
	connect(ui_->search, SIGNAL(editingFinished()), this, SLOT(search()));
	connect(ui_->zoomin_btn, SIGNAL(clicked(bool)), this, SLOT(zoomIn()));
	connect(ui_->zoomout_btn, SIGNAL(clicked(bool)), this, SLOT(zoomOut()));
	connect(this, SIGNAL(regionChanged(BedLine)), this, SLOT(updateRegionWidgets(BedLine)));
	connect(this, SIGNAL(regionChanged(BedLine)), ui_->gene_panel, SLOT(setRegion(BedLine)));
	connect(ui_->gene_panel, SIGNAL(mouseCoordinate(QString)), this, SLOT(updateCoordinateLabel(QString)));
}

void GenomeVisualizationWidget::setRegion(const Chromosome& chr, int start, int end)
{
	//extend region to minimal region size
	int size = end-start+1;
	if (size<settings_.min_window_size)
	{
		int missing = settings_.min_window_size-size;
		start -= missing/2;
		end += missing/2;
		if (missing%2!=0)
		{
			start -= 1;
			end += 1;
		}
		size = end-start+1;
	}

	//make sure the region is in chromosome boundaries
	if (start<1)
	{
		start = 1;
		end = start + size - 1;
	}
	if (end>genome_idx_.lengthOf(chr))
	{
		end = genome_idx_.lengthOf(chr);
		start = end - size + 1;
		if (start<1) start = 1; //if size is bigger than chromosome, this can happen
	}

	//check new region is different from old
	BedLine new_reg = BedLine(chr, start, end);
	if (current_reg_==new_reg) return;

	current_reg_ = new_reg;
	emit regionChanged(current_reg_);
}

void GenomeVisualizationWidget::setChromosomeRegion(QString chr)
{
	Chromosome c(chr);
	if (!c.isValid())
	{
		QMessageBox::warning(this, __FUNCTION__, "Could not convert chromosome string '" + chr + "' to valid chromosome!");
	}

	setRegion(chr, 1, genome_idx_.lengthOf(c));
}

void GenomeVisualizationWidget::search()
{
	QString text = ui_->search->text().trimmed();

	//chromosome
	if (valid_chrs_.contains(text) || (!text.startsWith("chr") && valid_chrs_.contains("chr"+text)))
	{
		setChromosomeRegion(text);
		return;
	}

	//chromosomal region
	BedLine region = BedLine::fromString(text);
	if (region.isValid())
	{
		setRegion(region.chr(), region.start(), region.end());
		return;
	}

	//gene
	if (gene_to_trans_indices_.contains(text.toUtf8()))
	{
		BedFile roi;
		foreach(int index, gene_to_trans_indices_[text.toUtf8()])
		{
			const Transcript& trans = transcripts_[index];
			roi.append(BedLine(trans.chr(), trans.start(), trans.end()));
		}
		roi.extend(settings_.transcript_padding);
		roi.merge();
		if (roi.count()>1)
		{
			QToolTip::showText(ui_->search->mapToGlobal(QPoint(0, 0)), "Gene has several transcript regions, using the first one!\nUse transcript identifiers to select a specific transcript of the gene!" + text);
		}

		setRegion(roi[0].chr(), roi[0].start(), roi[0].end());
		return;
	}

	//transcript
	if (trans_to_index_.contains(text.toUtf8()))
	{
		int index = trans_to_index_[text.toUtf8()];
		const Transcript& trans = transcripts_[index];
		setRegion(trans.chr(), trans.start()-settings_.transcript_padding, trans.end()+settings_.transcript_padding);
		return;
	}

	QToolTip::showText(ui_->search->mapToGlobal(QPoint(0, 0)), "Could not find locus or feature: " + text);
}

void GenomeVisualizationWidget::zoomIn()
{
	int size = current_reg_.length();
	setRegion(current_reg_.chr(), current_reg_.start()+size/4, current_reg_.end()-size/4);
}

void GenomeVisualizationWidget::zoomOut()
{
	int size = current_reg_.length();
	setRegion(current_reg_.chr(), current_reg_.start()-size/2, current_reg_.end()+size/2);
}

void GenomeVisualizationWidget::updateRegionWidgets(const BedLine& reg)
{
	ui_->chr_selector->blockSignals(true);
	ui_->chr_selector->setCurrentText(reg.chr().strNormalized(true));
	ui_->chr_selector->blockSignals(false);

	ui_->search->blockSignals(true);
	ui_->search->setText(reg.toString(true));
	ui_->search->blockSignals(false);

	ui_->label_region_size->setText(QString::number(reg.length()));
}

void GenomeVisualizationWidget::updateCoordinateLabel(QString text)
{
	ui_->label_coordinate->setText(text);
}

