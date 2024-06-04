#include "RepeatExpansionWidget.h"
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMenu>
#include <QChartView>
QT_CHARTS_USE_NAMESPACE
#include <QSvgWidget>
#include <QSvgRenderer>
#include "Helper.h"
#include "GUIHelper.h"
#include "TsvFile.h"
#include "VcfFile.h"
#include "GlobalServiceProvider.h"
#include "IgvSessionManager.h"
#include "GeneInfoDBs.h"
#include "ClientHelper.h"
#include "Log.h"

RepeatExpansionWidget::RepeatExpansionWidget(QWidget* parent, QString vcf, QString sys_type)
	: QWidget(parent)
	, ui_()
	, sys_type_cutoff_col_("")
{
	ui_.setupUi(this);
	ui_.filter_hpo->setEnabled(LoginManager::active());
	ui_.filter_hpo->setEnabled(!GlobalServiceProvider::getPhenotypesFromSmallVariantFilter().isEmpty());

	connect(ui_.table, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(cellDoubleClicked(int, int)));
	connect(ui_.table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui_.filter_expanded, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_hpo, SIGNAL(stateChanged(int)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_id, SIGNAL(textEdited(QString)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_show, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRowVisibility()));

	//allow statistical filtering only if there is a cutoff for the system type
	if (sys_type=="WGS")
	{
		sys_type_cutoff_col_ = "staticial_cutoff_wgs";

		//hide lrGS column:
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "reads supporting"), true);

	}
	else if (sys_type=="lrGS")
	{
		is_longread_ = true;
		//TODO: after import
		sys_type_cutoff_col_ = "staticial_cutoff_lrgs";

		//for now: remove column
		int idx = ui_.filter_expanded->findText("statistical outlier");
		ui_.filter_expanded->removeItem(idx);

		//hide lrGS column:
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "reads spanning"), true);
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "reads in repeat"), true);
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "reads flanking"), true);
	}
	else
	{
		int idx = ui_.filter_expanded->findText("statistical outlier");
		ui_.filter_expanded->removeItem(idx);

		//hide lrGS column:
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "reads supporting"), true);
	}

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
	if (!image_loc.exists) //support repeats with underscore in name
	{
		locus_base_name = locus_base_name.split('_').at(0);
		image_loc = GlobalServiceProvider::fileLocationProvider().getRepeatExpansionImage(locus_base_name);
	}

	//get histogram
	FileLocation hist_loc = GlobalServiceProvider::fileLocationProvider().getRepeatExpansionHistogram(locus_base_name);

    //create menu
	QMenu menu(ui_.table);
	QAction* a_comments = menu.addAction(QIcon(":/Icons/Comment.png"), "Show comments");
	QAction* a_distribution = menu.addAction(QIcon(":/Icons/AF_histogram.png"), "Show distribution");
	QAction* a_show_svg = menu.addAction("Show image of repeat");
	a_show_svg->setEnabled(image_loc.exists);
	QAction* a_show_hist = menu.addAction("Show histogram of repeats");
	a_show_hist->setEnabled(hist_loc.exists);
	menu.addSeparator();
	QAction* a_omim = menu.addAction(QIcon(":/Icons/OMIM.png"), "Open OMIM page(s)");
	menu.addSeparator();
	QAction* a_copy = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy all");
	QAction* a_copy_sel = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy selection");

    //execute menu
	QAction* action = menu.exec(ui_.table->viewport()->mapToGlobal(pos));
	if (action==a_show_svg)
    {
		QString filename;
		QByteArray svg;
		if (!ClientHelper::isClientServerMode())
		{
			filename = QFileInfo(image_loc.filename).absoluteFilePath();
			VersatileFile file(image_loc.filename);
			file.open(QIODevice::ReadOnly);
			svg = file.readAll();
		}
		else
		{
			svg = VersatileFile(image_loc.filename).readAll();
		}

		QSvgWidget* widget = new QSvgWidget();
		widget->load(svg);
		QRect rect = widget->renderer()->viewBox();
		widget->setMinimumSize(rect.width(), rect.height());

		QScrollArea* scroll_area = new QScrollArea(this);
		scroll_area->setFrameStyle(QFrame::NoFrame);
		scroll_area->setWidget(widget);
		scroll_area->setMinimumSize(1200, 800);

		QSharedPointer<QDialog> dlg = GUIHelper::createDialog(scroll_area, "Image of " + getCell(row, "repeat ID").trimmed());
		dlg->exec();
	}
	else if (action==a_show_hist)
	{
		QString filename;
		QByteArray svg;
		if (!ClientHelper::isClientServerMode())
		{
			filename = QFileInfo(hist_loc.filename).absoluteFilePath();
			VersatileFile file(hist_loc.filename);
			file.open(QIODevice::ReadOnly);
			svg = file.readAll();
		}
		else
		{
			svg = VersatileFile(hist_loc.filename).readAll();
		}

		QSvgWidget* widget = new QSvgWidget();
		widget->load(svg);
		QRect rect = widget->renderer()->viewBox();
		widget->setMinimumSize(rect.width(), rect.height());

		QScrollArea* scroll_area = new QScrollArea(this);
		scroll_area->setFrameStyle(QFrame::NoFrame);
		scroll_area->setWidget(widget);
		scroll_area->setMinimumSize(1200, 800);

		QSharedPointer<QDialog> dlg = GUIHelper::createDialog(scroll_area, "Histogram of " + getCell(row, "repeat ID").trimmed());
		dlg->exec();
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
		//get comments
		NGSD db;
		QString id = getRepeatId(db, row, false);
		QString comments;
		if (!id.isEmpty()) comments = db.repeatExpansionComments(id.toInt());
		//show dialog
		QTextEdit* edit = new QTextEdit(this);
		edit->setMinimumWidth(800);
		edit->setMinimumHeight(500);
		edit->setReadOnly(true);
		edit->setHtml(comments);
		QSharedPointer<QDialog> dlg = GUIHelper::createDialog(edit, "Comments");
		dlg->exec();
	}
	else if (action==a_distribution)
	{
		QString title = getCell(row, "repeat ID") + " repeat histogram";

		try
		{
			//get RE lengths
			NGSD db;
			QString id = getRepeatId(db, row, false);
			QVector<double> lengths = db.getValuesDouble("SELECT allele1 FROM repeat_expansion_genotype WHERE repeat_expansion_id=" + id);
			lengths << db.getValuesDouble("SELECT allele2 FROM repeat_expansion_genotype WHERE repeat_expansion_id=" + id);

			//determine min, max and bin size
			std::sort(lengths.begin(), lengths.end());
			double min = 0;
			int max_bin = 0.995 * lengths.count();
			double max = lengths[max_bin];
			double median = BasicStatistics::median(lengths, false);
			if (2*median > max) max = 2*median;
			double bin_size = (max-min)/40;

			//create histogram
			Histogram hist(min, max, bin_size);
			hist.inc(lengths, true);

			//show chart
			QChartView* view = GUIHelper::histogramChart(hist, "repeat length");
			auto dlg = GUIHelper::createDialog(view, title);
			dlg->exec();
		}
		catch(Exception& e)
		{
			QMessageBox::warning(this, title, "Error:\n" + e.message());
			return;
		}
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

QString RepeatExpansionWidget::getRepeatId(NGSD& db, int row, bool throw_if_fails)
{
	QString region = getCell(row, "region");
	QString repeat_unit = getCell(row, "repeat unit");

	return db.repeatExpansionId(region, repeat_unit, throw_if_fails);
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

		if(is_longread_)
		{
			//repeat ID
			QByteArray repeat_id = re.info("LOCUS").trimmed();
			setCell(row_idx, "repeat ID", repeat_id);

			//region
			QString region = re.chr().strNormalized(true) + ":" + QString::number(re.start()) + "-" + re.info("END").trimmed();
			setCell(row_idx, "region", region);

			//repreat unit
			QByteArray repeat_unit = re.info("REF_MOTIF").trimmed();
			setCell(row_idx, "repeat unit", repeat_unit);

			//filters
			QString filters = re.filters().join(",");
			if (filters=="PASS") filters = "";
			setCell(row_idx, "filters", filters);

			//genotype
			QString genotype = re.formatValueFromSample("AC").trimmed();
			setCell(row_idx, "genotype", genotype);

			//genotype CI
			QByteArray genotype_ci = re.formatValueFromSample("ACR").trimmed();
			setCell(row_idx, "genotype CI", genotype_ci);

			//local coverage
			double coverage = Helper::toDouble(re.formatValueFromSample("DP").trimmed());
			setCell(row_idx, "locus coverage", QString::number(coverage, 'f', 2));

//			//no value in straglr
//			//reads flanking
//			setCell(row_idx, "reads flanking", "-");
//			//reads in repeat
//			setCell(row_idx, "reads in repeat", "-");

			//reads flanking
			QByteArray reads_supporting = re.formatValueFromSample("AD").trimmed().replace(".", "-");
			setCell(row_idx, "reads supporting", reads_supporting);
		}
		else
		{
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

	if (ui_.table->rowCount()<40)
	{
		GUIHelper::showMessage("Repeat expansions", "Repeat expansion calls are outdated. Please re-do the repeat expansion calling!");
	}
}

void RepeatExpansionWidget::loadMetaDataFromNGSD()
{
	if (!LoginManager::active()) return;

	NGSD db;

	//get infos from NGSD
	for (int row=0; row<ui_.table->rowCount(); ++row)
	{
		//check if repeat is in NGSD
		QString id = getRepeatId(db, row);
		if (id.isEmpty())
		{
			setCellDecoration(row, "repeat ID", "Repeat not found in NGSD", orange_);
			continue;
		}

		//max_normal
		QVariant tmp = db.getValue("SELECT max_normal FROM repeat_expansion WHERE id=" + id);
		QString max_normal = tmp.isNull() ? "" : tmp.toString().trimmed();
		setCell(row, "max. normal", max_normal);

		//min_pathogenic
		tmp = db.getValue("SELECT min_pathogenic FROM repeat_expansion WHERE id=" + id);
		QString min_pathogenic = tmp.isNull() ? "" : tmp.toString().trimmed();
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
		QString comments = db.repeatExpansionComments(id.toInt());
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


	//show
	QString show = ui_.filter_show->currentText();
	if (show=="diagnostic")
	{
		int col = GUIHelper::columnIndex(ui_.table, "type");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			QTableWidgetItem* item = ui_.table->item(row, col);
			if (item==nullptr || !item->text().startsWith("diagnostic"))
			{
				hidden[row] = true;
			}
		}
	}
	if (show=="research")
	{
		int col = GUIHelper::columnIndex(ui_.table, "type");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			QTableWidgetItem* item = ui_.table->item(row, col);
			if (item==nullptr || !item->text().startsWith("research"))
			{
				hidden[row] = true;
			}
		}
	}
	if (show=="low evidence")
	{
		int col = GUIHelper::columnIndex(ui_.table, "type");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			QTableWidgetItem* item = ui_.table->item(row, col);
			if (item==nullptr || !item->text().startsWith("low evidence"))
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

		NGSD db;
		int col_geno = GUIHelper::columnIndex(ui_.table, "genotype");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			QString id = getRepeatId(db, row, false);
			QVariant cutoff = db.getValue("SELECT " + sys_type_cutoff_col_ + " FROM repeat_expansion WHERE id=:0", true, id);
			if (!cutoff.isValid()) continue;

			bool above_cutoff = false;
			foreach(QString geno, ui_.table->item(row, col_geno)->text().split("/"))
			{
				geno = geno.trimmed();
				if (geno.isEmpty()) continue;

				double value = geno.toInt();
				if(value>cutoff.toDouble()) above_cutoff = true;
			}
			if (!above_cutoff) hidden[row] = true;
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
		int col_repeat_id = GUIHelper::columnIndex(ui_.table, "repeat ID");
		int col_diseases = GUIHelper::columnIndex(ui_.table, "diseases");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			if (!ui_.table->item(row, col_repeat_id)->text().contains(id_search_str, Qt::CaseInsensitive) && !ui_.table->item(row, col_diseases)->text().contains(id_search_str, Qt::CaseInsensitive))
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
}
