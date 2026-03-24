#include "ui_GenomeVisualizationWidget.h"
#include "GenomeVisualizationWidget.h"
#include "BedFile.h"
#include "GUIHelper.h"
#include "SharedData.h"
#include "ErrorHandler.h"

#include <QToolTip>
#include <QMessageBox>

GenomeVisualizationWidget::GenomeVisualizationWidget(QWidget* parent)
	: QWidget(parent)
	, ui_(new Ui::GenomeVisualizationWidget)
{
	ui_->setupUi(this);
	GUIHelper::styleSplitter(ui_->panel_manager);

	//connect signals and slots
	connect(ui_->chr_selector, SIGNAL(currentTextChanged(QString)), this, SLOT(setChromosomeRegion(QString)));
	connect(ui_->search, SIGNAL(editingFinished()), this, SLOT(search()));
	connect(ui_->zoomin_btn, SIGNAL(clicked(bool)), this, SLOT(zoomIn()));
	connect(ui_->zoomout_btn, SIGNAL(clicked(bool)), this, SLOT(zoomOut()));
	connect(SharedData::instance(), SIGNAL(transcriptsChanged()), this, SLOT(updateIndices()));
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
	connect(ui_->gene_panel, SIGNAL(mouseCoordinate(QString)), this, SLOT(updateCoordinateLabel(QString)));
	connect(ui_->chr_panel, SIGNAL(mouseCoordinate(QString)), this, SLOT(updateCoordinateLabel(QString)));
	connect(ErrorHandler::instance(), SIGNAL(displayErrorReq(QString)), this, SLOT(displayErrorReq(QString)));
	connect(this, SIGNAL(loadFile()), ui_->panel_manager, SLOT(loadFile()));
}


void GenomeVisualizationWidget::updateIndices()
{
	//init chromosome list (ordered correctly)
	ui_->chr_selector->blockSignals(true);
	ui_->chr_selector->clear();
	foreach(const Chromosome& chr, SharedData::genome().chromosomes())
	{
		valid_chrs_ << chr.str();
	}
	ui_->chr_selector->addItems(valid_chrs_);
	ui_->chr_selector->blockSignals(false);

	//init gene and transcript list
	for(int i=0; i<SharedData::transcripts().size(); ++i)
	{
		const Transcript& trans = SharedData::transcripts()[i];

		if (trans.source()!=Transcript::ENSEMBL) continue;

		gene_to_trans_indices_[trans.gene()] << i;
		trans_to_index_[trans.name()] = i;
	}
}

void GenomeVisualizationWidget::setChromosomeRegion(QString chr)
{
	Chromosome c(chr);
	if (!c.isValid())
	{
		QMessageBox::warning(this, __FUNCTION__, "Could not convert chromosome string '" + chr + "' to valid chromosome!");
	}

	SharedData::setRegion(chr, 1, SharedData::genome().lengthOf(c));
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
		SharedData::setRegion(region.chr(), region.start(), region.end());
		return;
	}

	//gene
	if (gene_to_trans_indices_.contains(text.toUtf8()))
	{
		BedFile roi;
		foreach(int index, gene_to_trans_indices_[text.toUtf8()])
		{
			const Transcript& trans = SharedData::transcripts()[index];
			if (SharedData::settings().show_only_primary && !trans.isGencodePrimaryTranscript()) continue;
			roi.append(BedLine(trans.chr(), trans.start(), trans.end()));
		}
		roi.extend(SharedData::settings().transcript_padding);
		roi.merge();
		if (roi.count()>1)
		{
			QToolTip::showText(ui_->search->mapToGlobal(QPoint(0, 0)), "Gene has several transcript regions, using the first one!\nUse transcript identifiers to select a specific transcript of the gene!" + text);
		}

		SharedData::setRegion(roi[0].chr(), roi[0].start(), roi[0].end());
		return;
	}

	//transcript
	if (trans_to_index_.contains(text.toUtf8()))
	{
		int index = trans_to_index_[text.toUtf8()];
		const Transcript& trans = SharedData::transcripts()[index];
		SharedData::setRegion(trans.chr(), trans.start()-SharedData::settings().transcript_padding, trans.end()+SharedData::settings().transcript_padding);
		return;
	}

	QToolTip::showText(ui_->search->mapToGlobal(QPoint(0, 0)), "Could not find locus or feature: " + text);
}

void GenomeVisualizationWidget::zoomIn()
{
	const BedLine& reg = SharedData::region();
	SharedData::setRegion(reg.chr(), reg.start()+reg.length()/4, reg.end()-reg.length()/4);
}

void GenomeVisualizationWidget::zoomOut()
{
	const BedLine& reg = SharedData::region();
	SharedData::setRegion(reg.chr(), reg.start()-reg.length()/2, reg.end()+reg.length()/2);
}

void GenomeVisualizationWidget::updateRegion()
{
	const BedLine& reg = SharedData::region();

	//update region selectors at top
	ui_->chr_selector->blockSignals(true);
	ui_->chr_selector->setCurrentText(reg.chr().strNormalized(true));
	ui_->chr_selector->blockSignals(false);
	ui_->search->blockSignals(true);
	ui_->search->setText(reg.toString(true));
	ui_->search->blockSignals(false);

	//update size label in toolbar
	ui_->label_region_size->setText(QString::number(reg.length()));
}

void GenomeVisualizationWidget::updateCoordinateLabel(QString text)
{
	ui_->label_coordinate->setText(text);
}


void GenomeVisualizationWidget::displayErrorReq(QString msg)
{
	QMessageBox::critical(this, "Error", msg);
}
