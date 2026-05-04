#include "CnvWidget.h"
#include "Helper.h"
#include "Exceptions.h"
#include "GUIHelper.h"
#include "GSvarHelper.h"
#include "VariantDetailsDockWidget.h"
#include "NGSD.h"
#include "Settings.h"
#include "Log.h"
#include "ProcessedSampleWidget.h"
#include "Histogram.h"
#include "ReportVariantDialog.h"
#include "SomaticReportVariantDialog.h"
#include "CnvSearchWidget.h"
#include "LoginManager.h"
#include "GeneInfoDBs.h"
#include "ValidationDialog.h"
#include "GlobalServiceProvider.h"
#include "IgvSessionManager.h"
#include "ClinvarUploadDialog.h"
#include <QMessageBox>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include "ColumnConfig.h"
#include "SettingsDialog.h"
#include <QtCharts/QChartView>

CnvWidget::CnvWidget(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::CnvWidget)
	, data_controller_(AnalysisDataController::instance())
	, state_(data_controller_.getCnvFilterState())
	, callset_id_("")
	, cnvs_(data_controller_.getCnvList())
	, special_cols_()
	, report_config_(data_controller_.getGermlineReportConfig())
	, somatic_report_config_(data_controller_.getSomaticReportConfig())
    , ngsd_user_logged_in_(LoginManager::active())
	, rc_enabled_(ngsd_user_logged_in_ && ((data_controller_.germlineReportSupported() && !report_config_->isFinalized()) || data_controller_.somaticReportSupported()))
	, is_somatic_(data_controller_.getAnalysisType() == AnalysisType::SOMATIC_PAIR || data_controller_.getAnalysisType() == AnalysisType::SOMATIC_SINGLESAMPLE)
{
	ui->setupUi(this);
	connect(ui->cnvs, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(cnvDoubleClicked(QTableWidgetItem*)));
	connect(ui->cnvs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui->flag_invisible_cnvs_artefacts, SIGNAL(clicked(bool)), this, SLOT(flagInvisibleSomaticCnvsAsArtefacts()) );
	connect(ui->flag_visible_cnvs_artefacts, SIGNAL(clicked(bool)), this, SLOT(flagVisibleSomaticCnvsAsArtefacts()) );
	connect(ui->copy_clipboard, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
	connect(&state_, SIGNAL(targetRegionChanged()), this, SLOT(clearTooltips()));
	connect(ui->filter_widget, SIGNAL(calculateGeneTargetRegionOverlap()), this, SLOT(annotateTargetRegionGeneOverlap()));
	connect(ui->cnvs->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(cnvHeaderDoubleClicked(int)));
	ui->cnvs->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->cnvs->verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(cnvHeaderContextMenu(QPoint)));

	ui->resize_btn->setMenu(new QMenu());
	ui->resize_btn->menu()->addAction("Open column settings", this, SLOT(openColumnSettings()));
	ui->resize_btn->menu()->addAction("Apply column width settings", this, SLOT(adaptColumnWidthsAndHeights()));
	ui->resize_btn->menu()->addAction("Show all columns", this, SLOT(showAllColumns()));

	GUIHelper::styleSplitter(ui->splitter);
	ui->splitter->setStretchFactor(0, 10);
	ui->splitter->setStretchFactor(1, 1);

	//determine callset ID
    if (data_controller_.germlineReportSupported())
    {
        NGSD db;
		ps_id_ = db.processedSampleId(data_controller_.germlineReportSample(), false);
		if (ngsd_user_logged_in_ && ps_id_ != "")
        {
			callset_id_ = db.getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=" + ps_id_).toString();
        }
    }
	ui->quality->setEnabled(callset_id_!="");

	//set up NGSD menu (before loading CNV - QC actions are inserted then)
	ui->ngsd_btn->setMenu(new QMenu());
	ui->ngsd_btn->menu()->addAction(QIcon(":/Icons/Edit.png"), "Edit quality", this, SLOT(editQuality()))->setEnabled(callset_id_!="");
	ui->ngsd_btn->menu()->addSeparator();

	ui->cnvs->setSelectionMode(QAbstractItemView::ExtendedSelection);

	//precalculate het hit genes in data controller for filtering of the CNVs.
	data_controller_.getHetHitGenes();

	initGUI();
}

void CnvWidget::initGUI()
{
	//set up GUI
	try
	{
		updateGUI();
	}
	catch(Exception e)
	{
		addInfoLine("<font color='red'>Error parsing file:\n" + e.message() + "</font>");
		disableGUI();
	}

	if(is_somatic_)
	{
		ui->flag_invisible_cnvs_artefacts->setEnabled(true);
		ui->flag_visible_cnvs_artefacts->setEnabled(true);
	}
}

CnvWidget::~CnvWidget()
{
	delete ui;
}

void CnvWidget::showEvent(QShowEvent* e)
{
	QWidget::showEvent(e);
	// Call slot via queued connection so it's called from the UI thread after this method has returned and the window has been shown
	QMetaObject::invokeMethod(this, "proposeQualityIfUnset", Qt::ConnectionType::QueuedConnection);
}

void CnvWidget::cnvDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	int col = item->column();
	int row = item->row();
	QString col_name = item->tableWidget()->horizontalHeaderItem(col)->text();
	if (special_cols_.contains(col_name))
	{
		int col_index = cnvs_.annotationIndexByName(col_name.toUtf8(), false);
		if (col_index==-1) return;
		QString text = cnvs_[row].annotations()[col_index].trimmed();
		if (text.isEmpty()) return;
		QString title = col_name + " of CNV " + cnvs_[row].toString();

		if (col_name=="cn_pathogenic")
		{
			VariantDetailsDockWidget::showOverviewTable(title, text, ',');
		}
		if (col_name=="dosage_sensitive_disease_genes")
		{
			VariantDetailsDockWidget::showOverviewTable(title, text, ',', "https://search.clinicalgenome.org/kb/gene-dosage?search=");
		}
		if (col_name=="clinvar_cnvs")
		{
			VariantDetailsDockWidget::showOverviewTable(title, text, ',', "https://www.ncbi.nlm.nih.gov/clinvar/variation/");
		}
		if (col_name=="hgmd_cnvs")
		{
			VariantDetailsDockWidget::showOverviewTable(title, text, ',', "https://my.qiagendigitalinsights.com/bbp/view/hgmd/pro/mut.php?acc=");
		}
		if (col_name=="omim")
		{
			text = text.replace('_', ' ');
			VariantDetailsDockWidget::showOverviewTable(title, text, ',', "https://www.omim.org/entry/");
		}
	}
	else
	{
        IgvSessionManager::get(0).gotoInIGV(cnvs_[item->row()].toString(), true);
	}
}

void CnvWidget::addInfoLine(QString text)
{
	QLabel* label = new QLabel(text);
	label->setTextInteractionFlags(Qt::TextBrowserInteraction);
	ui->info_messages->layout()->addWidget(label);

	if(text.contains("TrioMaternalContamination"))
	{
		QStringList information_list = text.split("|");
		if(information_list.size() == 2)
		{
			QStringList mother_list = information_list.at(0).split(":");
			QStringList father_list = information_list.at(1).split(":");
			if(mother_list.size() == 2 && father_list.size() == 2)
			{
				bool is_double_mother(false);
				bool is_double_father(false);

				double mother = mother_list.at(1).toDouble(&is_double_mother);
				double father = father_list.at(1).toDouble(&is_double_father);

				if(is_double_mother && is_double_father)
				{
					double diff = std::abs(mother - father);
					if(diff > 0.1)
					{
						label->setStyleSheet("QLabel { color : red;}");
					}
				}
			}
		}
	}
	else if (text.contains(":"))
	{
		QString metric = text.split(":")[0].trimmed();
		while(metric.startsWith('#'))
		{
			metric = metric.mid(1).trimmed();
		}

		//special handling for trio output (metrics are prefixed with processed sample name)
		if (cnvs_.type()==CnvListType::CLINCNV_GERMLINE_MULTI)
		{
			metric = metric.split(" ").mid(1).join(" ");
		}

		//add distribution menu entry (only once for each metric)
		if (!metrics_done_.contains(metric))
		{
            ui->ngsd_btn->menu()->addAction("Show distribution: " + metric, this, SLOT(showQcMetricHistogram()))->setEnabled(ngsd_user_logged_in_ && ps_id_!="");
			metrics_done_ << metric;
		}
	}

}

void CnvWidget::disableGUI()
{
	ui->cnvs->setEnabled(false);
	ui->filter_widget->setEnabled(false);
	ui->ngsd_btn->setEnabled(false);
}

void CnvWidget::updateGUI()
{
	//set special columns
	special_cols_ = QStringList() << "cn_pathogenic" << "dosage_sensitive_disease_genes" << "clinvar_cnvs" << "hgmd_cnvs" << "omim";

	//show comments
	foreach(const QByteArray& comment, cnvs_.comments())
	{
		if (cnvs_.type()==CnvListType::CLINCNV_GERMLINE_SINGLE || cnvs_.type()==CnvListType::CLINCNV_GERMLINE_MULTI)
		{
			if (comment.contains("Analysis finished on")) continue;
			if (comment.contains("was it outlier after clustering")) continue;
			if (comment.contains("fraction of outliers")) continue;
		}
		addInfoLine(comment);
	}

	//determine column order of annotations
	ColumnConfig config = ColumnConfig::fromString(Settings::string("column_config_cnv", true));
	QStringList col_order;
	QList<int> anno_index_order;
	config.getOrder(cnvs_, col_order, anno_index_order);
	foreach(int i, anno_index_order)
	{
		QByteArray header = cnvs_.annotationHeaders()[i];

		//add column
		ui->cnvs->setColumnCount(ui->cnvs->columnCount() + 1);

		//create item
		QTableWidgetItem* item = new QTableWidgetItem(QString(header));
		QStringList tooltip_lines;

		QByteArray header_desc = cnvs_.headerDescription(header);
		if (!header_desc.isEmpty())
		{
			tooltip_lines << header_desc;
		}
		if (special_cols_.contains(header))
		{
			item->setIcon(QIcon("://Icons/Table.png"));
			tooltip_lines << "Double click table cell to open table view of annotations";
		}
		item->setToolTip(tooltip_lines.join('\n'));

		//set header
		ui->cnvs->setHorizontalHeaderItem(ui->cnvs->columnCount() -1, item);
	}

	//get report variant indices
	QSet<int> report_variant_indices;
    if(!is_somatic_) report_variant_indices = Helper::listToSet(report_config_->variantIndices(VariantType::CNVS, false));
    else report_variant_indices = Helper::listToSet(somatic_report_config_->variantIndices(VariantType::CNVS, false));


	//show variants
	ui->cnvs->setRowCount(cnvs_.count());
	for (int r=0; r<cnvs_.count(); ++r)
	{
		//vertical headers
		QTableWidgetItem* header_item = GUIHelper::createTableItem(QByteArray::number(r+1));
		if (report_variant_indices.contains(r))
		{
			bool show_report_icon;
			bool causal = false;
			if(!is_somatic_)
			{
				const ReportVariantConfiguration& rc = report_config_->get(VariantType::CNVS, r);
				show_report_icon = rc.showInReport();
				causal = rc.causal;
			}
			else
			{
				show_report_icon = somatic_report_config_->get(VariantType::CNVS, r).showInReport();
			}

			header_item->setIcon(VariantTable::reportIcon(show_report_icon, causal));
		}
		ui->cnvs->setVerticalHeaderItem(r, header_item);

		//table cells
		ui->cnvs->setItem(r, 0, GUIHelper::createTableItem(cnvs_[r].toString()));
		ui->cnvs->setItem(r, 1, GUIHelper::createTableItem(QString::number(cnvs_[r].size()/1000.0, 'f', 3), Qt::AlignRight|Qt::AlignTop));
		QString regions = QString::number(cnvs_[r].regions());
		if (regions=="0") regions="n/a";
		ui->cnvs->setItem(r, 2, GUIHelper::createTableItem(regions, Qt::AlignRight|Qt::AlignTop));

		QTableWidgetItem* item = GUIHelper::createTableItem(QString(cnvs_[r].genes().join(',')));
		GSvarHelper::colorGeneItem(item, cnvs_[r].genes());
		ui->cnvs->setItem(r, 3, item);

		int c = 4;
		foreach(int index, anno_index_order)
		{
			QTableWidgetItem* item = GUIHelper::createTableItem(cnvs_[r].annotations()[index]);
			//special handling for OMIM
			if (cnvs_.annotationHeaders()[index]=="omim")
			{
				item->setText(item->text().replace("_", " "));
			}
			item->setToolTip(item->text().replace("],","]\n"));
			ui->cnvs->setItem(r, c++, item);
		}
	}

	//resize columns/rows
	adaptColumnWidthsAndHeights();

	//hide columns
	config.applyHidden(ui->cnvs);

	//update quality from NGSD
	updateQuality();
}

void CnvWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui->cnvs);
}

void CnvWidget::showContextMenu(QPoint p)
{
	QMenu menu;

	//get selected rows
	QList<int> selected_rows = GUIHelper::selectedTableRows(ui->cnvs);

	//special case: 2 variants selected -> show compHet upload to ClinVar
	if (selected_rows.count() == 2)
	{
		//ClinVar publication
		QAction* a_clinvar_pub = menu.addAction(QIcon("://Icons/ClinGen.png"), "Publish compound-heterozygote CNV in ClinVar");
		a_clinvar_pub->setEnabled(ngsd_user_logged_in_ && ! Settings::string("clinvar_api_key", true).trimmed().isEmpty() &&  data_controller_.germlineReportSupported());

		//execute menu
		QAction* action = menu.exec(ui->cnvs->viewport()->mapToGlobal(p));
		if (action == a_clinvar_pub) uploadToClinvar(selected_rows.at(0), selected_rows.at(1));
		return;
	}
	else if (selected_rows.count() != 1)
	{
		return;
	}

	//else: standard case: 1 variant selected
	int row = selected_rows.at(0);

	//create menu
	QAction* a_rep_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	a_rep_edit->setEnabled(rc_enabled_);
	QAction* a_rep_del = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	if(!is_somatic_) a_rep_del->setEnabled(rc_enabled_ && report_config_->exists(VariantType::CNVS, row));
    else a_rep_del->setEnabled(ngsd_user_logged_in_ && somatic_report_config_->exists(VariantType::CNVS, row));
	menu.addSeparator();
	QAction* a_cnv_val = menu.addAction("Perform copy-number variant validation");
    a_cnv_val->setEnabled(ngsd_user_logged_in_);
	menu.addSeparator();
	QAction* a_ngsd_search = menu.addAction(QIcon(":/Icons/NGSD.png"), "Matching CNVs in NGSD");
    a_ngsd_search->setEnabled(ngsd_user_logged_in_);
	menu.addSeparator();
	QAction* a_deciphter = menu.addAction(QIcon("://Icons/Decipher.png"), "Open in Decipher browser");
	QAction* a_dgv = menu.addAction(QIcon("://Icons/DGV.png"), "Open in DGV");
	QAction* a_ucsc = menu.addAction(QIcon("://Icons/UCSC.png"), "Open in UCSC browser");
	QAction* a_ucsc_override = menu.addAction(QIcon("://Icons/UCSC.png"), "Open in UCSC browser (override tracks)");
	menu.addSeparator();

	//ClinVar search
	QMenu*sub_menu = menu.addMenu(QIcon("://Icons/ClinGen.png"), "ClinVar");
	QAction* a_clinvar_find = sub_menu->addAction("Find in ClinVar");
	QAction* a_clinvar_pub = sub_menu->addAction("Publish in ClinVar");
	a_clinvar_pub->setEnabled(ngsd_user_logged_in_ && !Settings::string("clinvar_api_key", true).trimmed().isEmpty() && data_controller_.germlineReportSupported());

	//PubMed
	sub_menu = menu.addMenu(QIcon("://Icons/PubMed.png"), "PubMed");
	for (const QByteArray& g : cnvs_[row].genes())
	{
		sub_menu->addAction(g + " AND (\"mutation\" OR \"variant\")");
		for (const Phenotype& p : state_.getPhenotypes())
		{
			sub_menu->addAction(g + " AND \"" + p.name().trimmed() + "\"");
		}
	}

	//gene sub-menus
	if (!cnvs_[row].genes().isEmpty())
	{
		menu.addSeparator();

		int gene_nr = 1;
        for (const QByteArray& gene : cnvs_[row].genes())
		{
			++gene_nr;
			if (gene_nr>=10) break; //don't show too many sub-menues for large variants!

			QMenu* sub_menu = menu.addMenu(gene);
            sub_menu->addAction(QIcon("://Icons/NGSD_gene.png"), "Gene tab")->setEnabled(ngsd_user_logged_in_);
			sub_menu->addAction(QIcon("://Icons/Google.png"), "Google");
            for (const GeneDB& db : GeneInfoDBs::all())
			{
				sub_menu->addAction(db.icon, db.name);
			}
		}
	}

	//exec menu
	QAction* action = menu.exec(ui->cnvs->viewport()->mapToGlobal(p));
	if (action==nullptr) return;

	//react on selection
	QMenu* parent_menu = qobject_cast<QMenu*>(action->parent());
	if (action==a_dgv)
	{
		QDesktopServices::openUrl(QUrl("http://dgv.tcag.ca/gb2/gbrowse/dgv2_hg38/?name=" + cnvs_[row].toString()));
	}
	else if (action==a_ucsc)
	{
		QDesktopServices::openUrl(QUrl("https://genome.ucsc.edu/cgi-bin/hgTracks?db=hg38&position=" + cnvs_[row].toString()));
	}
	else if (action==a_ucsc_override)
	{
		QDesktopServices::openUrl(QUrl("https://genome.ucsc.edu/cgi-bin/hgTracks?db=hg38&ignoreCookie=1&hideTracks=1&cytoBand=pack&refSeqComposite=dense&ensGene=dense&omimGene2=pack&geneReviews=pack&dgvPlus=squish&genomicSuperDups=squish&position=" + cnvs_[row].toString()));
	}
	else if (action==a_deciphter)
	{
		QDesktopServices::openUrl(QUrl("https://www.deciphergenomics.org/browser#q/" + cnvs_[row].toString().remove("chr")));
	}
	else if (action==a_rep_edit)
	{
		editReportConfiguration(row);
	}
	else if (action==a_rep_del)
	{
		if(!is_somatic_)
		{
			report_config_->remove(VariantType::CNVS, row);
		}
		else
		{
			//Delete som variant configuration for more than one cnv
			QModelIndexList selectedRows = ui->cnvs->selectionModel()->selectedRows();
			foreach(const auto& selected_row, selectedRows)
			{
				if( ui->cnvs->isRowHidden(selected_row.row()) ) continue;
				somatic_report_config_->remove(VariantType::CNVS, selected_row.row());
				updateReportConfigHeaderIcon(selected_row.row());
			}
			emit storeSomaticReportConfiguration();
		}
		updateReportConfigHeaderIcon(row);
	}
	else if (action==a_cnv_val)
	{
		editCnvValidation(row);
	}
	else if (action==a_ngsd_search)
	{
		CnvSearchWidget* widget = new CnvSearchWidget();
		widget->setCoordinates(cnvs_[row].chr(), cnvs_[row].start(), cnvs_[row].end());
		auto dlg = GUIHelper::createDialog(widget, "CNV search");

		dlg->exec();
	}
	else if (action==a_clinvar_find)
	{
		//use GSvar helper with dummy variant
		QString url = GSvarHelper::clinVarSearchLink(Variant(cnvs_[row].chr(), cnvs_[row].start(), cnvs_[row].end(), "", ""));
		QDesktopServices::openUrl(QUrl(url));
	}
	else if (action==a_clinvar_pub)
	{
		uploadToClinvar(row);
	}
	else if (parent_menu && parent_menu->title()=="PubMed")
	{
		QDesktopServices::openUrl(QUrl("https://pubmed.ncbi.nlm.nih.gov/?term=" + action->text()));
	}
	else if (parent_menu) //gene sub-menus
	{
		QString gene = parent_menu->title();
		QString db_name = action->text();

		if (db_name=="Gene tab")
		{
			GlobalServiceProvider::openGeneTab(gene);
		}
		else if (db_name=="Google")
		{
			QString query = gene + " AND (mutation";
			for (const Phenotype& pheno : state_.getPhenotypes())
			{
				query += " OR \"" + pheno.name() + "\"";
			}
			query += ")";

			QDesktopServices::openUrl(QUrl("https://www.google.com/search?q=" + query.replace("+", "%2B").replace(' ', '+')));
		}
		else //other databases
		{
			GeneInfoDBs::openUrl(db_name, gene);
		}
	}
}

void CnvWidget::proposeQualityIfUnset()
{
	if (callset_id_=="") return;
	if (cnvs_.type()!=CnvListType::CLINCNV_GERMLINE_SINGLE) return;

	//check if quality is set
	NGSD db;
	QString quality =  db.getValue("SELECT quality FROM cnv_callset WHERE id=" + callset_id_).toString();
	if (quality!="n/a") return;


	//check number of iterations
	QStringList errors;
	QString iterations = cnvs_.qcMetric("number of iterations", false);
	if (iterations.isEmpty()) return;
	if(iterations!="1")
	{
		errors << "Number of iteration > 1";
	}

	//check number of high-quality CNVs
	QString sys_id = db.getValue("SELECT processing_system_id FROM processed_sample WHERE id=" + ps_id_).toString();
	QVector<double> hq_cnv_dist = db.cnvCallsetMetrics(sys_id, "high-quality cnvs");
	if (hq_cnv_dist.count()<20) return;
	std::sort(hq_cnv_dist.begin(), hq_cnv_dist.end());
	double mean = BasicStatistics::median(hq_cnv_dist, false);
	double stdev = 1.482 * BasicStatistics::mad(hq_cnv_dist, mean);

	QString hq_cnvs = cnvs_.qcMetric("high-quality cnvs", false);
	if (hq_cnvs.isEmpty()) return;
	if (hq_cnvs.toDouble()> mean + 2.5*stdev)
	{
		errors << "Number of high-quality CNVs is too high (median: " + QString::number(mean, 'f', 2) + " / stdev: " + QString::number(stdev, 'f', 2) + ")";
	}

	if(errors.count()==0)
	{
		db.getQuery().exec("UPDATE cnv_callset SET quality='good' WHERE id=" + callset_id_);
		updateQuality();
	}
	else
	{
		QMessageBox::warning(this, "CNV callset quality", "CNV callset quality seems to be not optimal:\n" + errors.join("\n")
							 + "\n\nPlease have a look at the quality parameters and set the CNV quality manually!");
	}
}

void CnvWidget::updateQuality()
{
	QString quality = "n/a";
	if  (callset_id_!="")
	{
		quality = NGSD().getValue("SELECT quality FROM cnv_callset WHERE id=" + callset_id_).toString();
	}
	ProcessedSampleWidget::styleQualityLabel(ui->quality, quality);
}

void CnvWidget::editQuality()
{
	NGSD db;
	QStringList qualities = db.getEnum("cnv_callset", "quality");

	bool ok;
	QString quality = QInputDialog::getItem(this, "Select CNV callset quality", "quality:", qualities, qualities.indexOf(ui->quality->toolTip()), false, &ok);
	if (!ok) return;

	db.getQuery().exec("UPDATE cnv_callset SET quality='" + quality + "' WHERE id=" + callset_id_);

	updateQuality();
}

void CnvWidget::showQcMetricHistogram()
{
	//determine metrics name
	QAction* action = qobject_cast<QAction*>(sender());
	if (action==nullptr || !action->text().contains(":")) return;
	QString metric_name = action->text().split(":")[1].trimmed();

	//get metrics
	NGSD db;
	QString sys_id = db.getValue("SELECT processing_system_id FROM processed_sample WHERE id=" + ps_id_).toString();
	QVector<double> metrics = db.cnvCallsetMetrics(sys_id, metric_name);
	if (metrics.isEmpty())
	{
		QMessageBox::information(this, "CNV callset QC metrics", "No (numeric) QC entries found for metric '" + metric_name + "'!");
		return;
	}

	//determine median
	std::sort(metrics.begin(), metrics.end());
	double median = BasicStatistics::median(metrics, false);
	double stdev = 1.482 * BasicStatistics::mad(metrics, median);

	//create histogram
	double upper_bound = median + 4.0 * stdev;
	if (metric_name=="number of iterations" && upper_bound<4.0) upper_bound = 4.0;
	if (metric_name=="quality used at final iteration" && upper_bound<80.0) upper_bound = 80.0;
	if (metric_name=="mean correlation to reference samples" && upper_bound>1.0) upper_bound = 1.0;

	Histogram hist(0.0, upper_bound, upper_bound/40);
	hist.inc(metrics, true);

	//show histogram
	QChartView* view = GUIHelper::histogramChart(hist, metric_name);
	auto dlg = GUIHelper::createDialog(view, "QC-metric distribution: " + metric_name);
	dlg->exec();
}

void CnvWidget::updateReportConfigHeaderIcon(int row)
{
	//report config-based filter is on => update whole variant list
	if (state_.getReportConfigFilter()!=ReportConfigFilter::NONE)
	{
		updateGUI();
	}
	else //no filter => refresh icon only
	{
		QIcon report_icon;
		if (!is_somatic_ && report_config_->exists(VariantType::CNVS, row))
		{
			const ReportVariantConfiguration& rc = report_config_->get(VariantType::CNVS, row);
			report_icon = VariantTable::reportIcon(rc.showInReport(), rc.causal);
		}
		else if(is_somatic_ && somatic_report_config_->exists(VariantType::CNVS, row))
		{
			report_icon = VariantTable::reportIcon(somatic_report_config_->get(VariantType::CNVS, row).showInReport(), false);
		}
		ui->cnvs->verticalHeaderItem(row)->setIcon(report_icon);
	}
}

void CnvWidget::cnvHeaderDoubleClicked(int row)
{
	if (!rc_enabled_) return;

	editReportConfiguration(row);
}

void CnvWidget::cnvHeaderContextMenu(QPoint pos)
{
	if (!rc_enabled_) return;

	//get variant index
	int row = ui->cnvs->verticalHeader()->visualIndexAt(pos.ry());

	//set up menu
	QMenu menu(ui->cnvs->verticalHeader());
	QAction* a_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	QAction* a_delete = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	if(!is_somatic_) a_delete->setEnabled(report_config_->exists(VariantType::CNVS, row) && !report_config_->isFinalized());
	else a_delete->setEnabled(somatic_report_config_->exists(VariantType::CNVS, row));

	//exec menu
	pos = ui->cnvs->verticalHeader()->viewport()->mapToGlobal(pos);
	QAction* action = menu.exec(pos);
	if (action==nullptr) return;

	//actions
	if (action==a_edit)
	{
		editReportConfiguration(row);
	}
	else if (action==a_delete)
	{
		if(!is_somatic_)
		{
			report_config_->remove(VariantType::CNVS, row);
			updateReportConfigHeaderIcon(row);
		}
		else
		{
			//Delete som variant configuration for more than one cnv
			QModelIndexList selectedRows = ui->cnvs->selectionModel()->selectedRows();
			foreach(const auto& selected_row, selectedRows)
			{
				somatic_report_config_->remove(VariantType::CNVS, selected_row.row());
				updateReportConfigHeaderIcon(selected_row.row());
			}

			emit storeSomaticReportConfiguration();
		}
	}
}

void CnvWidget::updateStatus(int shown)
{
	QString text = QString::number(shown) + "/" + QString::number(cnvs_.count()) + " passing filter(s)";
	ui->status->setText(text);
}

void CnvWidget::editReportConfiguration(int row)
{
	if(cnvs_.type() == CnvListType::CLINCNV_TUMOR_NORMAL_PAIR)
	{
		//Handle som variant configuration for more than one variant
		QModelIndexList selectedRows = ui->cnvs->selectionModel()->selectedRows();
		if(selectedRows.count() > 1)
		{
			QList<int> rows;
			foreach(const auto& selected_row, selectedRows)
			{
				if( ui->cnvs->isRowHidden(selected_row.row()) ) continue;
				rows << selected_row.row();
			}
			editSomaticReportConfiguration(rows);
		}
		else //single somatic variant
		{
			editSomaticReportConfiguration(row);
		}
	}
	else if(cnvs_.type() == CnvListType::CLINCNV_TUMOR_ONLY)
	{
		QMessageBox::warning(this, "Not implemented", "Report Config is not yet implemented for tumor only!");
	}
	else
	{
		editGermlineReportConfiguration(row);
	}
}

void CnvWidget::importPhenotypesFromNGSD()
{
	if (ps_id_=="")
	{
		QMessageBox::warning(this, "Error loading phenotypes", "Cannot load phenotypes because no processed sample ID is set!");
		return;
	}

	NGSD db;
	QString sample_id = db.getValue("SELECT sample_id FROM processed_sample WHERE id=:0", false, ps_id_).toString();
	PhenotypeList phenotypes = db.getSampleData(sample_id).phenotypes;

	state_.setPhenotypes(phenotypes);
}

void CnvWidget::annotateTargetRegionGeneOverlap()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//generate gene regions and index
	BedFile roi_genes = NGSD().genesToRegions(state_.getTargetRegionInfo().genes, Transcript::ENSEMBL, "gene", true);
	roi_genes.extend(5000);
	ChromosomalIndex<BedFile> roi_genes_index(roi_genes);

	// update gene tooltips
	int gene_idx = GUIHelper::columnIndex(ui->cnvs, "genes", false);
	if (gene_idx!=-1)
	{
		for (int row_idx = 0; row_idx < ui->cnvs->rowCount(); ++row_idx)
		{
			// get all matching lines in gene bed file
			QVector<int> matching_indices = roi_genes_index.matchingIndices(cnvs_[row_idx].chr(), cnvs_[row_idx].start(), cnvs_[row_idx].end());

			// extract gene names
			GeneSet genes;
			foreach (int idx, matching_indices)
			{
				genes.insert(roi_genes[idx].annotations().at(0));
			}

			// update tooltip
			ui->cnvs->item(row_idx, gene_idx)->setToolTip("<div style=\"wordwrap\">Target region gene overlap: <br> " + genes.toString(", ") + "</div>");
		}
	}
	QApplication::restoreOverrideCursor();
}

void CnvWidget::clearTooltips()
{
	int gene_idx = GUIHelper::columnIndex(ui->cnvs, "genes", false);
	if (gene_idx==-1) return;

	for (int r=0; r<ui->cnvs->rowCount(); ++r)
	{
		// remove tool tip
		ui->cnvs->item(r, gene_idx)->setToolTip("");
	}
}

void CnvWidget::editGermlineReportConfiguration(int row)
{
    if(data_controller_.germlineReportSupported())
	{
		THROW(ProgrammingException, "ReportConfiguration in CnvWidget is nullpointer.");
	}

	NGSD db;

	//init/get config
	ReportVariantConfiguration var_config;
	if (report_config_->exists(VariantType::CNVS, row))
	{
		var_config = report_config_->get(VariantType::CNVS, row);
	}
	else
	{
		var_config.variant_type = VariantType::CNVS;
		var_config.variant_index = row;
	}

	//get inheritance mode by gene
	QList<KeyValuePair> inheritance_by_gene;
	int i_genes = cnvs_.annotationIndexByName("genes", false);
	if (i_genes!=-1)
	{
		GeneSet genes = GeneSet::createFromText(cnvs_[row].annotations()[i_genes], ',');
		for (int i=0; i<genes.count(); i++)
		{
			const QByteArray& gene = genes[i];
			GeneInfo gene_info = db.geneInfo(gene);
			inheritance_by_gene << KeyValuePair{gene, gene_info.inheritance};
		}
	}

	//exec dialog
	ReportVariantDialog dlg(cnvs_[row].toStringWithMetaData(), inheritance_by_gene, var_config, this);
	dlg.setEnabled(!report_config_->isFinalized());
	if (dlg.exec()!=QDialog::Accepted) return;

	//update config, GUI and NGSD
	report_config_->set(var_config);
	updateReportConfigHeaderIcon(row);
}

void CnvWidget::editSomaticReportConfiguration(int row)
{
    if(data_controller_.somaticReportSupported())
	{
		THROW(ProgrammingException, "SomaticReportConfiguration in CnvWidget is null pointer.");
	}

	SomaticReportVariantConfiguration var_config;
	bool settings_exist = somatic_report_config_->exists(VariantType::CNVS, row);
	if(settings_exist)
	{
		var_config = somatic_report_config_->get(VariantType::CNVS, row);
	}
	else
	{
		var_config.variant_type = VariantType::CNVS;
		var_config.variant_index = row;
	}

	SomaticReportVariantDialog* dlg = new SomaticReportVariantDialog(cnvs_[row].toStringWithMetaData(), var_config, this);
	if(dlg->exec()!=QDialog::Accepted) return;

	somatic_report_config_->addSomaticVariantConfiguration(var_config);
	updateReportConfigHeaderIcon(row);
	emit storeSomaticReportConfiguration();
}

void CnvWidget::editCnvValidation(int row)
{
	try
	{
		ValidationDialog dlg(this, data_controller_.getCnvValidationEntry(row));

		if (dlg.exec())
		{
			data_controller_.storeVariantValidation(dlg.getValidation());
		}
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
		return;
	}
}

void CnvWidget::editSomaticReportConfiguration(const QList<int> &rows)
{
    if(data_controller_.somaticReportSupported())
	{
		THROW(ProgrammingException, "SomaticReportConfiguration in CnvWidget is null pointer.");
	}

	SomaticReportVariantConfiguration generic_var_config;
	generic_var_config.variant_index = -1;
	generic_var_config.variant_type = VariantType::CNVS;

	SomaticReportVariantDialog* dlg = new SomaticReportVariantDialog(QString::number(rows.count()) +" selected cnvs", generic_var_config, this);
	if(dlg->exec() != QDialog::Accepted) return;

	//Accepted was pressed -> see slot writeBackSettings()
	for(int row : rows)
	{
		SomaticReportVariantConfiguration temp_var_config = generic_var_config;
		temp_var_config.variant_index = row;

		somatic_report_config_->addSomaticVariantConfiguration(temp_var_config);

		updateReportConfigHeaderIcon(row);
	}
}

void CnvWidget::uploadToClinvar(int index1, int index2)
{
    if (!ngsd_user_logged_in_) return;
	try
	{
		ClinvarUploadDialog dlg(this);
		dlg.setData(data_controller_.getClinvarUploadDataCnv(index1, index2));
		dlg.exec();
	}
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, "ClinVar submission error");
	}

}

void CnvWidget::flagInvisibleSomaticCnvsAsArtefacts()
{
	if(somatic_report_config_ == nullptr)
	{
		THROW(ProgrammingException, "SomaticReportConfiguration in CnvWidget is null pointer.");
	}

	SomaticReportVariantConfiguration generic_var_config;
	generic_var_config.variant_type = VariantType::CNVS;
	generic_var_config.exclude_artefact = true;
	generic_var_config.comment = "Flagged as CNV artefact by batch filtering";

	for(int r=0; r<ui->cnvs->rowCount(); ++r)
	{
		if(ui->cnvs->isRowHidden(r))
		{
			generic_var_config.variant_index = r;
			somatic_report_config_->addSomaticVariantConfiguration(generic_var_config);
			updateReportConfigHeaderIcon(r);
		}
	}

	emit storeSomaticReportConfiguration();
}

void CnvWidget::flagVisibleSomaticCnvsAsArtefacts()
{
	if(somatic_report_config_ == nullptr)
	{
		THROW(ProgrammingException, "SomaticReportConfiguration in CnvWidget is null pointer");
	}

	SomaticReportVariantConfiguration generic_var_config;
	generic_var_config.variant_type = VariantType::CNVS;
	generic_var_config.exclude_artefact = true;
	generic_var_config.comment = "Flagged as CNV artefact by batch filtering";

	for(int r=0; r<ui->cnvs->rowCount(); ++r)
	{
		if(!ui->cnvs->isRowHidden(r))
		{
			generic_var_config.variant_index = r;
			somatic_report_config_->addSomaticVariantConfiguration(generic_var_config);
			updateReportConfigHeaderIcon(r);
		}
	}
	emit storeSomaticReportConfiguration();
}

void CnvWidget::openColumnSettings()
{
	SettingsDialog dlg(this);
	dlg.gotoPage("columns", variantTypeToString(VariantType::CNVS));
	if (dlg.exec()==QDialog::Accepted)
	{
		dlg.storeSettings();
	}
}

void CnvWidget::adaptColumnWidthsAndHeights()
{
	ColumnConfig config = ColumnConfig::fromString(Settings::string("column_config_cnv", true));
	config.applyColumnWidths(ui->cnvs);
}

void CnvWidget::showAllColumns()
{
	for (int i=0; i<ui->cnvs->colorCount(); ++i)
	{
		ui->cnvs->setColumnHidden(i, false);
	}
}
