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
	ui_.filter_hpo->setEnabled(LoginManager::active());
	ui_.filter_hpo->setEnabled(!GlobalServiceProvider::getPhenotypesFromSmallVariantFilter().isEmpty());

	connect(ui_.table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui_.filter_expanded, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_hpo, SIGNAL(stateChanged(int)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_ngsd, SIGNAL(stateChanged(int)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_id, SIGNAL(textEdited(QString)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_diagnostic, SIGNAL(stateChanged(int)), this, SLOT(updateRowVisibility()));

	loadDataFromVCF(vcf);
	loadMetaDataFromNGSD();
	GUIHelper::resizeTableCells(ui_.table, 200);

	colorRepeatCountBasedOnCutoffs();
	updateRowVisibility();
}

void RepeatExpansionWidget::showContextMenu(QPoint pos)
{
	// determine selected row
	QItemSelection selection = ui_.table->selectionModel()->selection();
	if(selection.count() != 1) return;
	int row = selection.at(0).indexes().at(0).row();

	//get image
	QString locus_base_name = getCell(row, "repeat ID").trimmed();
	FileLocation image_loc = GlobalServiceProvider::fileLocationProvider().getRepeatExpansionImage(locus_base_name);
	if (!image_loc.exists) //TODO support repeats with underscore in name
	{
		locus_base_name = locus_base_name.split('_').at(0);
		image_loc = GlobalServiceProvider::fileLocationProvider().getRepeatExpansionImage(locus_base_name);
	}

    //create menu
	QMenu menu(ui_.table);
	QAction* a_show_svg = menu.addAction("Show image of repeat");
	a_show_svg->setEnabled(image_loc.exists);
	QAction* a_omim = menu.addAction(QIcon(":/Icons/OMIM.png"), "Open OMIM page(s)");
	menu.addSeparator();
	QAction* a_comments = menu.addAction("Show/edit comments");
	menu.addSeparator();
	QAction* a_copy = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy all");
	QAction* a_copy_sel = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy selection");

    //execute menu
	QAction* action = menu.exec(ui_.table->viewport()->mapToGlobal(pos));
	if (action==a_show_svg)
    {
        //open SVG in browser
		QString filename = image_loc.filename;
		if (!ClientHelper::isClientServerMode()) filename = QFileInfo(image_loc.filename).absoluteFilePath();

		QDesktopServices::openUrl(QUrl(filename));
	}
	else if (action==a_copy)
	{
		GUIHelper::copyToClipboard(ui_.table);
	}
	else if (action==a_copy_sel)
	{
		GUIHelper::copyToClipboard(ui_.table, true);
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
		GUIHelper::copyToClipboard(ui_.table, true);
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
	int col = GUIHelper::columnIndex(ui_.table, column);

	//set item
	QTableWidgetItem* item = GUIHelper::createTableItem(value);
	ui_.table->setItem(row, col, item);

	return item;
}

QString RepeatExpansionWidget::getCell(int row, QString column)
{
	//determine column index
	int col = GUIHelper::columnIndex(ui_.table, column);

	//set item
	QTableWidgetItem* item = ui_.table->item(row, col);
	if (item==nullptr) return "";

	return item->text().trimmed();
}

void RepeatExpansionWidget::setCellDecoration(int row, QString column, QString tooltip, QColor bg_color)
{
	//determine column index
	int col = GUIHelper::columnIndex(ui_.table, column);
	QTableWidgetItem* item = ui_.table->item(row, col);

	//create item if missing
	if (item==nullptr)
	{
		item = GUIHelper::createTableItem("");
		ui_.table->setItem(row, col, item);
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
	ui_.table->setRowCount(repeat_expansions.count());
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

	//get infos from NGSD
	for (int row=0; row<ui_.table->rowCount(); ++row)
	{
		QString region = getCell(row, "region");
		QString repeat_unit = getCell(row, "repeat unit");

		//check if repeat is in NGSD
		QString id = db.getValue("SELECT id FROM repeat_expansion WHERE region='"+region+"' and repeat_unit='" + repeat_unit + "'", true).toString().trimmed();
		if (id.isEmpty())
		{
			setCellDecoration(row, "repeat ID", "Repeat not found in NGSD", orange_);
			continue;
		}

		//max_normal
		QString max_normal = db.getValue("SELECT max_normal FROM repeat_expansion WHERE id=" + id).toString().trimmed();
		setCell(row, "max. normal", max_normal);

		//min_pathogenic
		QString min_pathogenic = db.getValue("SELECT min_pathogenic FROM repeat_expansion WHERE id=" + id).toString().trimmed();
		setCell(row, "min. pathogenic", min_pathogenic);

		//inheritance
		QString inheritance = db.getValue("SELECT inheritance FROM repeat_expansion WHERE id=" + id).toString().trimmed();
		setCell(row, "inheritance", inheritance);

		//location
		QString location = db.getValue("SELECT location FROM repeat_expansion WHERE id=" + id).toString().trimmed();
		setCell(row, "location", location);

		//diseases
		QString disease_names = db.getValue("SELECT disease_names FROM repeat_expansion WHERE id=" + id).toString().trimmed();
		setCell(row, "diseases", disease_names);

		//OMIM IDs
		QString disease_ids_omim = db.getValue("SELECT disease_ids_omim FROM repeat_expansion WHERE id=" + id).toString().trimmed();
		setCell(row, "OMIM disease IDs", disease_ids_omim);

		//comments
		QString comments = db.getValue("SELECT comments FROM repeat_expansion WHERE id=" + id).toString().trimmed();
		QTableWidgetItem* item = setCell(row, "comments", "");
		if (!comments.isEmpty())
		{
			setCellDecoration(row, "comments", comments);
			item->setIcon(QIcon(":/Icons/Info.png"));
		}

		//HPO terms
		QString hpo_terms = db.getValue("SELECT hpo_terms FROM repeat_expansion WHERE id=" + id).toString().trimmed();
		setCell(row, "HPO terms", hpo_terms);

		//type
		QString type = db.getValue("SELECT type FROM repeat_expansion WHERE id=" + id).toString().trimmed();
		setCell(row, "type", type);
	}
}

void RepeatExpansionWidget::colorRepeatCountBasedOnCutoffs()
{
	for (int row=0; row<ui_.table->rowCount(); ++row)
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

void RepeatExpansionWidget::updateRowVisibility()
{
	QBitArray hidden(ui_.table->rowCount(), false);

	//in NGSD?
	if (ui_.filter_ngsd->isChecked())
	{
		int col = GUIHelper::columnIndex(ui_.table, "repeat ID");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			if (ui_.table->item(row, col)->backgroundColor()==orange_)
			{
				hidden[row] = true;
			}
		}
	}

	//expansion status
	if (ui_.filter_expanded->currentText()=="larger than normal")
	{
		int col = GUIHelper::columnIndex(ui_.table, "genotype");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			if (ui_.table->item(row, col)->backgroundColor()!=orange_) hidden[row] = true;
		}
	}
	if (ui_.filter_expanded->currentText()=="pathogenic")
	{
		int col = GUIHelper::columnIndex(ui_.table, "genotype");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			if (ui_.table->item(row, col)->backgroundColor()!=red_) hidden[row] = true;
		}
	}
	if (ui_.filter_expanded->currentText()=="statistical outlier")
	{
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			//TODO
		}
	}

	//HPO filter
	if (ui_.filter_hpo->isChecked())
	{
		//determine hpo subtree of patient
		PhenotypeList pheno_subtrees;
		NGSD db;
		foreach(const Phenotype& pheno, GlobalServiceProvider::getPhenotypesFromSmallVariantFilter())
		{

			pheno_subtrees << db.phenotypeChildTerms(db.phenotypeIdByAccession(pheno.accession()), true);
		}

		//filter REs based on overlap with HPOs
		int col = GUIHelper::columnIndex(ui_.table, "HPO terms");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{

			bool hpo_match = false;
			QTableWidgetItem* item = ui_.table->item(row, col);
			if (item!=nullptr)
			{
				foreach(QByteArray hpo_acc, item->text().toUtf8().split(','))
				{
					if (pheno_subtrees.containsAccession(hpo_acc))
					{
						hpo_match = true;
						break;
					}
				}
			}
			if (!hpo_match) hidden[row] = true;
		}
	}

	//repeat ID text search
	QString id_search_str = ui_.filter_id->text().trimmed();
	if (!id_search_str.isEmpty())
	{
		int col = GUIHelper::columnIndex(ui_.table, "repeat ID");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			if (!ui_.table->item(row, col)->text().contains(id_search_str, Qt::CaseInsensitive))
			{
				hidden[row] = true;
			}
		}
	}

	//diagnostic only
	if (ui_.filter_diagnostic->isChecked())
	{
		int col = GUIHelper::columnIndex(ui_.table, "type");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			QTableWidgetItem* item = ui_.table->item(row, col);
			if (item==nullptr) continue;
			if (!item->text().startsWith("diagnostic"))
			{
				hidden[row] = true;
			}
		}
	}

	//show/hide rows
	for (int row=0; row<ui_.table->rowCount(); ++row)
	{
		ui_.table->setRowHidden(row, hidden[row]);
	}

	QTextStream(stdout) << "REs shown: " << hidden.count(false) << endl; //TODO remove
}
