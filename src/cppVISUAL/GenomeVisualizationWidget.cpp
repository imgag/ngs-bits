#include "ui_GenomeVisualizationWidget.h"
#include "GenomeVisualizationWidget.h"
#include "BedFile.h"

#include <QDebug>
#include <QMessageBox>

GenomeVisualizationWidget::GenomeVisualizationWidget(QWidget* parent, const FastaFileIndex& genome_idx)
	: QWidget(parent)
	, ui_(new Ui::GenomeVisualizationWidget)
	, settings_()
	, genome_idx_(genome_idx)
	, valid_chrs_()
	, current_reg_()
{
	ui_->setupUi(this);

	//init chromosome list (ordered correctly)
	valid_chrs_ = genome_idx_.names();
	std::sort(valid_chrs_.begin(), valid_chrs_.end(), [](const QString& s1, const QString& s2){ Chromosome c1(s1); Chromosome c2(s2); return (c1.num()<1004 || c2.num()<1004) ? c1<c2 : s1<s2;  });
	ui_->chr_selector->addItems(valid_chrs_);

	//connect signals and slots
	connect(ui_->chr_selector, SIGNAL(currentTextChanged(QString)), this, SLOT(setChromosomeRegion(QString)));
	connect(ui_->search, SIGNAL(editingFinished()), this, SLOT(search()));
	connect(ui_->zoomin_btn, SIGNAL(clicked(bool)), this, SLOT(zoomIn()));
	connect(ui_->zoomout_btn, SIGNAL(clicked(bool)), this, SLOT(zoomOut()));
	connect(this, SIGNAL(regionChanged(BedLine)), this, SLOT(updateRegionWidgets(BedLine)));
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
		qDebug() << "MATCH: CHR";
		setChromosomeRegion(text);
		return;
	}

	//chromosomal region
	BedLine region = BedLine::fromString(text);
	if (region.isValid())
	{
		qDebug() << "MATCH: REG";
		setRegion(region.chr(), region.start(), region.end());
		return;
	}

	qDebug() << "NO MATCH!";
	//TODO: gene

	//TODO: transcript
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
	qDebug() << "NEW REGION" << reg.toString(true) << reg.length();

	ui_->chr_selector->blockSignals(true);
	ui_->chr_selector->setCurrentText(reg.chr().strNormalized(true));
	ui_->chr_selector->blockSignals(false);

	ui_->search->blockSignals(true);
	ui_->search->setText(reg.toString(true));
	ui_->search->blockSignals(false);
}

