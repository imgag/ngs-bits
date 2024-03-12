#include "RepeatExpansionWidget.h"
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMenu>
#include "Helper.h"
#include "GUIHelper.h"
#include "TsvFile.h"
#include "VcfFile.h"
#include "GlobalServiceProvider.h"
#include "IgvSessionManager.h"
#include "GeneInfoDBs.h"
#include "ClientHelper.h"
#include "Log.h"

RepeatExpansionWidget::RepeatExpansionWidget(QWidget* parent, QString vcf)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);

	connect(ui_.repeat_expansions,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));
	connect(ui_.repeat_expansions,SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellDoubleClicked(int, int)));

	loadDataFromVCF(vcf);
}

void RepeatExpansionWidget::showContextMenu(QPoint pos)
{
	// determine selected row
	QItemSelection selection = ui_.repeat_expansions->selectionModel()->selection();
	if(selection.count() != 1) return;
	int row = selection.at(0).indexes().at(0).row();

	//get image
	QString locus = ui_.repeat_expansions->item(row,3)->text().split('_').at(0);
	FileLocation image_loc = GlobalServiceProvider::fileLocationProvider().getRepeatExpansionImage(locus);

    //create menu
	QMenu menu(ui_.repeat_expansions);
	QAction* a_show_svg = menu.addAction("Show image of repeat");
	a_show_svg->setEnabled(image_loc.exists);
	QAction* a_omim = menu.addAction(QIcon(":/Icons/OMIM.png"), "Open OMIM page");
	menu.addSeparator();
	QAction* a_copy = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy all");
	QAction* a_copy_sel = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy selection");

    //execute menu
	QAction* action = menu.exec(ui_.repeat_expansions->viewport()->mapToGlobal(pos));
	if (action==a_show_svg)
    {
        //open SVG in browser
		QString filename = image_loc.filename;
		if (!ClientHelper::isClientServerMode()) filename = QFileInfo(image_loc.filename).absoluteFilePath();

		QDesktopServices::openUrl(QUrl(filename));
	}
	else if (action==a_copy)
	{
		GUIHelper::copyToClipboard(ui_.repeat_expansions);
	}
	else if (action==a_copy_sel)
	{
		GUIHelper::copyToClipboard(ui_.repeat_expansions, true);
	}
	else if (action==a_omim)
	{
		QString repeat_id = ui_.repeat_expansions->item(row, 3)->text();
		QString gene = repeat_id.contains("_") ? repeat_id.left(repeat_id.indexOf("_")) : repeat_id;
		GeneInfoDBs::openUrl("OMIM", gene);
	}
}

void RepeatExpansionWidget::cellDoubleClicked(int row, int /*col*/)
{
	QString region = ui_.repeat_expansions->item(row, 0)->text() + ":" + ui_.repeat_expansions->item(row, 1)->text() + "-" + ui_.repeat_expansions->item(row, 2)->text();
    IgvSessionManager::get(0).gotoInIGV(region, true);
}

void RepeatExpansionWidget::keyPressEvent(QKeyEvent* event)
{
	if(event->matches(QKeySequence::Copy))
	{
		GUIHelper::copyToClipboard(ui_.repeat_expansions, true);
		event->accept();
		return;
	}

	QWidget::keyPressEvent(event);
}

QTableWidgetItem* RepeatExpansionWidget::setCell(int row, QString column, QString value)
{
	//make alle-specific information more readable
	if (value=="-" || value=="-/-") value = "";
	value.replace("/", " / ");

	//determine column index
	int col = GUIHelper::columnIndex(ui_.repeat_expansions, column);

	//set item
	QTableWidgetItem* item = GUIHelper::createTableItem(value);
	ui_.repeat_expansions->setItem(row, col, item);

	return item;
}

void RepeatExpansionWidget::loadDataFromVCF(QString vcf)
{
	//load VCF file
	VcfFile repeat_expansions;
	repeat_expansions.load(vcf);

	// check that there is exactly one sample
	const QByteArrayList& samples = repeat_expansions.sampleIDs();
	if (samples.count()!=1)
	{
		THROW(ArgumentException, "Repeat expansion VCF file '" + vcf + "' does not contain exactly one sample!");
	}

	// fill table widget with variants/repeat expansions
	ui_.repeat_expansions->setRowCount(repeat_expansions.count());
	for(int row_idx=0; row_idx<repeat_expansions.count(); ++row_idx)
	{
		const VcfLine& re = repeat_expansions[row_idx];

		//repeat ID
		QByteArray repeat_id = re.info("REPID").trimmed();
		setCell(row_idx, "repeat ID", repeat_id);

		//region
		QString region = re.chr().strNormalized(true) + ":" + QString::number(re.start()) + "-" + re.info("END").trimmed();
		setCell(row_idx, "region", region);

		//repreat unit
		QByteArray repeat_unit = re.info("RU").trimmed();
		setCell(row_idx, "repeat unit", repeat_unit);

		//filters
		QString filters = re.filters().join(",");
		if (filters=="PASS") filters = "";
		QTableWidgetItem* item = setCell(row_idx, "filters", filters);
		if (filters!="") item->setBackgroundColor(orange_);

		//genotype
		QString genotype = re.formatValueFromSample("REPCN").trimmed().replace(".", "-");
		setCell(row_idx, "genotype", genotype);

		//genotype CI
		QByteArray genotype_ci = re.formatValueFromSample("REPCI").trimmed().replace(".", "-");
		setCell(row_idx, "genotype CI", genotype_ci);

		//local coverage
		double coverage = Helper::toDouble(re.formatValueFromSample("LC").trimmed());
		setCell(row_idx, "locus coverage", QString::number(coverage, 'f', 2));

		//reads flanking
		QByteArray reads_flanking = re.formatValueFromSample("ADFL").trimmed().replace(".", "-");
		setCell(row_idx, "reads flanking", reads_flanking);

		//reads in repeat
		QByteArray read_in_repeat = re.formatValueFromSample("ADIR").trimmed().replace(".", "-");
		setCell(row_idx, "reads in repeat", read_in_repeat);

		//reads flanking
		QByteArray reads_spanning = re.formatValueFromSample("ADSP").trimmed().replace(".", "-");
		setCell(row_idx, "reads spanning", reads_spanning);
	}

	GUIHelper::resizeTableCells(ui_.repeat_expansions);
}


/*
		//color table acording to cutoff table
		QByteArrayList repeats = format_repcn.split('/');
		int repeats_allele1, repeats_allele2;
		if ((repeats.size() < 1) || (repeats.size() > 2)) THROW(FileParseException, "Invalid allele count in repeat entries!");

		// skip coloring if no repeat information is available:
		if ((repeats.at(0).trimmed() != ".") && (repeats.at(0).trimmed() != ""))
		{
			repeats_allele1 = Helper::toInt(repeats.at(0), "Repeat allele 1", QString::number(row_idx));

			if (repeats.size() == 1) repeats_allele2 = 0; // special case for male samples on chrX
			else repeats_allele2 = Helper::toInt(repeats.at(1), "Repeat allele 2", QString::number(row_idx));

			//check if pathogenic
			if (cutoff_info.min_pathogenic!=-1 && (repeats_allele1>=cutoff_info.min_pathogenic || repeats_allele2>=cutoff_info.min_pathogenic))
			{
				repeat_cell->setBackgroundColor(bg_red);
			}
			//above normal
			else if (cutoff_info.max_normal!=-1 && (repeats_allele1>cutoff_info.max_normal || repeats_allele2>cutoff_info.max_normal))
			{
				repeat_cell->setBackgroundColor(bg_orange);
			}
			//normal
			else if (cutoff_info.max_normal != -1)
			{
				repeat_cell->setBackgroundColor(bg_green);
			}
		}


		repeat_cell->setToolTip(repeat_tool_tip_text.join('\n'));
*/
