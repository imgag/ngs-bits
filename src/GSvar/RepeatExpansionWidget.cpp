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

	connect(ui_.repeat_expansions, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui_.filter_expanded, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFilters()));
	connect(ui_.filter_hpo, SIGNAL(stateChanged(int)), this, SLOT(updateFilters()));

	loadDataFromVCF(vcf);
	loadMetaDataFromNGSD();
	GUIHelper::resizeTableCells(ui_.repeat_expansions);

	colorRepeatCountBasedOnCutoffs();
}

void RepeatExpansionWidget::showContextMenu(QPoint pos)
{
	// determine selected row
	QItemSelection selection = ui_.repeat_expansions->selectionModel()->selection();
	if(selection.count() != 1) return;
	int row = selection.at(0).indexes().at(0).row();

	//get image
	QString locus_base_name = getCell(row, "repeat ID").split('_').at(0); //TODO support ARX_1, ARX_2, ATXN8OS_CTA
	FileLocation image_loc = GlobalServiceProvider::fileLocationProvider().getRepeatExpansionImage(locus_base_name);

    //create menu
	QMenu menu(ui_.repeat_expansions);
	QAction* a_show_svg = menu.addAction("Show image of repeat");
	a_show_svg->setEnabled(image_loc.exists);
	QAction* a_omim = menu.addAction(QIcon(":/Icons/OMIM.png"), "Open OMIM page(s)");
	menu.addSeparator();
	QAction* a_comments = menu.addAction("Show/edit comments");
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
		QStringList omim_ids = getCell(row, "OMIM disease IDs").split(",");
		foreach(QString omim_id, omim_ids)
		{
			QDesktopServices::openUrl(QUrl("https://www.omim.org/entry/" + omim_id.trimmed()));
		}
	}
	else if (action==a_comments)
	{
		//TODO show/edit comment
	}
}

void RepeatExpansionWidget::cellDoubleClicked(int row, int /*col*/)
{
	QString region = getCell(row, "region");
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

QString RepeatExpansionWidget::getCell(int row, QString column)
{
	//determine column index
	int col = GUIHelper::columnIndex(ui_.repeat_expansions, column);

	//set item
	QTableWidgetItem* item = ui_.repeat_expansions->item(row, col);
	if (item==nullptr) return "";

	return item->text().trimmed();
}

void RepeatExpansionWidget::setCellDecoration(int row, QString column, QString tooltip, QColor bg_color)
{
	//determine column index
	int col = GUIHelper::columnIndex(ui_.repeat_expansions, column);
	QTableWidgetItem* item = ui_.repeat_expansions->item(row, col);

	//create item if missing
	if (item==nullptr)
	{
		item = GUIHelper::createTableItem("");
		ui_.repeat_expansions->setItem(row, col, item);
	}

	//set tooltip
	if (!tooltip.isEmpty())
	{
		item->setToolTip(tooltip);
	}

	//set background color
	if (bg_color.isValid())
	{
		item->setBackgroundColor(bg_color);
	}
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
		setCell(row_idx, "filters", filters);

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
}

void RepeatExpansionWidget::loadMetaDataFromNGSD()
{
	if (!LoginManager::active()) return;

	NGSD db;

	for (int row=0; row<ui_.repeat_expansions->rowCount(); ++row)
	{
		QString repeat_id = getCell(row, "repeat ID");

		//check if repeat is in NGSD
		QString id = db.getValue("SELECT id FROM repeat_expansion_meta_data WHERE repeat_id=:0", true, repeat_id).toString().trimmed();
		if (id.isEmpty())
		{
			setCellDecoration(row, "repeat ID", "Repeat ID not found in NGSD", orange_);
			continue;
		}

		//max_normal
		QString max_normal = db.getValue("SELECT max_normal FROM repeat_expansion_meta_data WHERE id=" + id).toString().trimmed();
		setCell(row, "max. normal", max_normal);

		//min_pathogenic
		QString min_pathogenic = db.getValue("SELECT min_pathogenic FROM repeat_expansion_meta_data WHERE id=" + id).toString().trimmed();
		setCell(row, "min. pathogenic", min_pathogenic);

		//inheritance
		QString inheritance = db.getValue("SELECT inheritance FROM repeat_expansion_meta_data WHERE id=" + id).toString().trimmed();
		setCell(row, "inheritance", inheritance);

		//location
		QString location = db.getValue("SELECT location FROM repeat_expansion_meta_data WHERE id=" + id).toString().trimmed();
		setCell(row, "location", location);

		//diseases
		QString disease_names = db.getValue("SELECT disease_names FROM repeat_expansion_meta_data WHERE id=" + id).toString().trimmed();
		setCell(row, "diseases", disease_names);

		//OMIM IDs
		QString disease_ids_omim = db.getValue("SELECT disease_ids_omim FROM repeat_expansion_meta_data WHERE id=" + id).toString().trimmed();
		setCell(row, "OMIM disease IDs", disease_ids_omim);

		//comments
		QString comments = db.getValue("SELECT comments FROM repeat_expansion_meta_data WHERE id=" + id).toString().trimmed();
		setCell(row, "comments", "present..."); //TODO icon
		setCellDecoration(row, "comments", comments);

		//HPO terms
		QString hpo_terms = db.getValue("SELECT hpo_terms FROM repeat_expansion_meta_data WHERE id=" + id).toString().trimmed();
		setCell(row, "HPO terms", hpo_terms);
	}
}

void RepeatExpansionWidget::colorRepeatCountBasedOnCutoffs()
{
	for (int row=0; row<ui_.repeat_expansions->rowCount(); ++row)
	{
		bool ok = false;

		//determine cutoffs
		int max_normal = getCell(row, "max. normal").toInt(&ok);
		if(!ok) continue;
		int min_pathogenic = getCell(row, "min. pathogenic").toInt(&ok);
		if(!ok) continue;

		//determine maximum
		QStringList genotypes = getCell(row, "genotype").split("/");
		int max = -1;
		foreach(QString geno, genotypes)
		{
			bool ok = false;
			int repeat_count = geno.trimmed().toInt(&ok);
			if (ok)
			{
				max = std::max(max, repeat_count);
			}
		}
		if (max==-1) continue;

		//color
		if (max>=min_pathogenic)
		{
			setCellDecoration(row, "genotype", "Above min. pathogenic cutoff!", red_);
		}
		else if (max>max_normal)
		{
			setCellDecoration(row, "genotype", "Above max. normal cutoff!", orange_);
		}
	}
}

void RepeatExpansionWidget::updateFilters()
{
	QBitArray hidden(ui_.repeat_expansions->rowCount(), false);

	//expansion status
	if (ui_.filter_expanded->currentText()=="larger than normal")
	{
		for (int row=0; row<ui_.repeat_expansions->rowCount(); ++row)
		{
			//TODO
		}
	}
	if (ui_.filter_expanded->currentText()=="pathogenic")
	{
		for (int row=0; row<ui_.repeat_expansions->rowCount(); ++row)
		{
			//TODO
		}
	}
	if (ui_.filter_expanded->currentText()=="statistical outlier")
	{
		for (int row=0; row<ui_.repeat_expansions->rowCount(); ++row)
		{
			//TODO
		}
	}

	//HPO filter
	if (ui_.filter_hpo->isChecked())
	{
		for (int row=0; row<ui_.repeat_expansions->rowCount(); ++row)
		{
			//TODO
		}
	}

	//show/hide rows
	for (int row=0; row<ui_.repeat_expansions->rowCount(); ++row)
	{
		ui_.repeat_expansions->setRowHidden(row, hidden[row]);
	}
	qDebug() << hidden;
}
