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
#include "ReportVariantDialog.h"

RepeatExpansionWidget::RepeatExpansionWidget(QWidget* parent, const RepeatLocusList& res, QSharedPointer<ReportConfiguration> report_config, QString sys_type)
	: QWidget(parent)
	, ui_()
	, res_(res)
	, sys_type_cutoff_col_("")
	, report_config_(report_config)
	, ngsd_enabled_(LoginManager::active())
	, rc_enabled_(ngsd_enabled_ && report_config_!=nullptr && !report_config_->isFinalized())
{
	ui_.setupUi(this);
	ui_.filter_hpo->setEnabled(ngsd_enabled_);
	ui_.filter_hpo->setEnabled(!GlobalServiceProvider::filterWidget()->phenotypes().isEmpty());

	connect(ui_.table, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(cellDoubleClicked(int, int)));
	connect(ui_.table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui_.filter_expanded, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_hpo, SIGNAL(stateChanged(int)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_id, SIGNAL(textEdited(QString)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_show, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRowVisibility()));	
	ui_.table->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_.table->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(svHeaderDoubleClicked(int)));
	connect(ui_.table->verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(svHeaderContextMenu(QPoint)));

	//allow statistical filtering only if there is a cutoff for the system type
	if (sys_type=="WGS")
	{
		sys_type_cutoff_col_ = "staticial_cutoff_wgs";

		//hide lrGS column:
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "reads supporting"), true);

	}
	else if (sys_type=="lrGS")
	{
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

	if (!res_.isEmpty())
	{
		displayRepeats();
		loadMetaDataFromNGSD();
		GUIHelper::resizeTableCells(ui_.table, 200);

		colorRepeatCountBasedOnCutoffs();
		updateRowVisibility();
	}
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
	QAction* a_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	QAction* a_delete = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	a_delete->setEnabled(!report_config_->isFinalized() && report_config_->exists(VariantType::RES, row));
	menu.addSeparator();
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
	else if (action==a_edit)
	{
		editReportConfiguration(row);
	}
	else if (action==a_delete)
	{
		report_config_->remove(VariantType::RES, row);
		updateReportConfigHeaderIcon(row);
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
	BedLine region = BedLine::fromString(getCell(row, "region"));
	QString repeat_unit = getCell(row, "repeat unit");

	int id = db.repeatExpansionId(region, repeat_unit, throw_if_fails);
	if (id==-1) return "";

	return QString::number(id);
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

void RepeatExpansionWidget::updateReportConfigHeaderIcon(int row)
{
	QIcon report_icon;
	if (report_config_->exists(VariantType::RES, row))
	{
		const ReportVariantConfiguration& rc = report_config_->get(VariantType::SVS, row);
		report_icon = VariantTable::reportIcon(rc.showInReport(), rc.causal);
	}
	ui_.table->verticalHeaderItem(row)->setIcon(report_icon);
}

void RepeatExpansionWidget::editReportConfiguration(int row)
{
	if(report_config_ == nullptr)
	{
		THROW(ProgrammingException, "ReportConfiguration in RepeatExpansionWidget is nullpointer.");
	}

	//init/get config
	ReportVariantConfiguration var_config;
	if (report_config_->exists(VariantType::RES, row))
	{
		var_config = report_config_->get(VariantType::RES, row);
	}
	else
	{
		var_config.variant_type = VariantType::RES;
		var_config.variant_index = row;
	}

	//exec dialog
	ReportVariantDialog dlg(res_[row].toString(false, false), QList<KeyValuePair>(), var_config, this);
	dlg.setEnabled(!report_config_->isFinalized());
	if (dlg.exec()!=QDialog::Accepted) return;

	//update config, GUI and NGSD
	report_config_->set(var_config);
	updateReportConfigHeaderIcon(row);
}

void RepeatExpansionWidget::displayRepeats()
{
	// fill table widget with variants/repeat expansions
	ui_.table->setRowCount(res_.count());
	for(int row_idx=0; row_idx<res_.count(); ++row_idx)
	{
		const RepeatLocus& re = res_[row_idx];

		//repeat ID
		setCell(row_idx, "repeat ID", re.name());

		//region
		setCell(row_idx, "region", re.region().toString(true));

		//repreat unit
		setCell(row_idx, "repeat unit", re.unit());

		//filters
		setCell(row_idx, "filters", re.filters().join(","));

		//genotype
		setCell(row_idx, "genotype", re.alleles());

		//genotype CI
		setCell(row_idx, "genotype CI", re.confidenceIntervals());

		//local coverage
		double coverage = Helper::toDouble(re.coverage(), "RE coverage");
		setCell(row_idx, "locus coverage", QString::number(coverage, 'f', 2));

		//reads flanking
		setCell(row_idx, "reads flanking", re.readsFlanking());

		//reads in repeat
		setCell(row_idx, "reads in repeat", re.readsInRepeat());

		//reads flanking
		setCell(row_idx, "reads spanning", re.readsFlanking());
	}

	if (ui_.table->rowCount()<40)
	{
		GUIHelper::showMessage("Repeat expansions", "Repeat expansion calls are outdated. Please re-do the repeat expansion calling!");
	}
}

void RepeatExpansionWidget::loadMetaDataFromNGSD()
{
	if (!ngsd_enabled_) return;

	NGSD db;

	//get infos from NGSD
	for (int row=0; row<ui_.table->rowCount(); ++row)
	{
		//check if repeat is in NGSD
		QString id = getRepeatId(db, row, false);
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
		foreach(const Phenotype& pheno, GlobalServiceProvider::filterWidget()->phenotypes())
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
void RepeatExpansionWidget::svHeaderDoubleClicked(int row)
{
	if (!ngsd_enabled_) return;
	editReportConfiguration(row);
}

void RepeatExpansionWidget::svHeaderContextMenu(QPoint pos)
{
	if (!ngsd_enabled_ || (report_config_ == nullptr)) return;

	//get variant index
	int row = ui_.table->verticalHeader()->visualIndexAt(pos.ry());

	//set up menu
	QMenu menu(ui_.table->verticalHeader());
	QAction* a_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	QAction* a_delete = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	a_delete->setEnabled(!report_config_->isFinalized() && report_config_->exists(VariantType::RES, row));

	//exec menu
	pos = ui_.table->verticalHeader()->viewport()->mapToGlobal(pos);
	QAction* action = menu.exec(pos);
	if (action==nullptr) return;

	if(!ngsd_enabled_) return; //do nothing if no access to NGSD

	//actions
	if (action==a_edit)
	{
		editReportConfiguration(row);
	}
	else if (action==a_delete)
	{
		report_config_->remove(VariantType::RES, row);
		updateReportConfigHeaderIcon(row);
	}
}
