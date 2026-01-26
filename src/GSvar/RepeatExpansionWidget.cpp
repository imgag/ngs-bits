#include "RepeatExpansionWidget.h"
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMenu>
#include <QChartView>
#include "LoginManager.h"
#include "Helper.h"
#include "GUIHelper.h"
#include "GlobalServiceProvider.h"
#include "IgvSessionManager.h"
#include "GeneInfoDBs.h"
#include "ClientHelper.h"
#include "Log.h"
#include "ReportVariantDialog.h"
#include <QtSvg/QSvgRenderer>
#include <QPainter>

RepeatExpansionWidget::RepeatExpansionWidget(QWidget* parent, const RepeatLocusList& res, QSharedPointer<ReportConfiguration> report_config, QString sys_name)
	: QWidget(parent)
	, ui_()
	, res_(res)
	, sys_name_(sys_name)
	, sys_type_cutoff_col_("")
	, report_config_(report_config)
    , ngsd_user_logged_in_(LoginManager::active())
    , rc_enabled_(ngsd_user_logged_in_ && report_config_!=nullptr && !report_config_->isFinalized())
{
    ui_.setupUi(this);
    ui_.filter_hpo->setEnabled(ngsd_user_logged_in_);
	ui_.filter_hpo->setEnabled(!GlobalServiceProvider::filterWidget()->phenotypes().isEmpty());

	connect(ui_.table, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(cellDoubleClicked(int, int)));
	connect(ui_.table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui_.filter_expanded, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_hpo, SIGNAL(stateChanged(int)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_rc, SIGNAL(stateChanged(int)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_id, SIGNAL(textEdited(QString)), this, SLOT(updateRowVisibility()));
	connect(ui_.filter_show, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRowVisibility()));	
	ui_.table->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_.table->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(svHeaderDoubleClicked(int)));
	connect(ui_.table->verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(svHeaderContextMenu(QPoint)));

	//allow statistical filtering only if there is a cutoff for the system type
	NGSD db;
	sys_type_ = db.getValue("SELECT type FROM processing_system WHERE name_manufacturer LIKE '" + sys_name_ + "'").toString();
	if (sys_type_=="WGS")
	{
		sys_type_cutoff_col_ = "statisticial_cutoff_wgs";

		//hide lrGS column
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "reads supporting"), true);

	}
	else if (sys_type_=="lrGS")
	{
		sys_type_cutoff_col_ = "statisticial_cutoff_lrgs";

		//hide srGS columns
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "reads spanning"), true);
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "reads in repeat"), true);
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "reads flanking"), true);
	}
	else
	{
		int idx = ui_.filter_expanded->findText("statistical outlier");
		ui_.filter_expanded->removeItem(idx);

		//hide lrGS column
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "reads supporting"), true);
	}
	if (sys_type_cutoff_col_.isEmpty())
	{
		ui_.table->setColumnHidden(GUIHelper::columnIndex(ui_.table, "statistical cutoff"), true);
	}

    if (!res_.isEmpty())
    {
        displayRepeats();
        loadMetaDataFromNGSD();
        GUIHelper::resizeTableCellWidths(ui_.table, 300);
        GUIHelper::resizeTableCellHeightsToMinimum(ui_.table);
        colorRepeatCountBasedOnCutoffs();
        colorRepeatCountConfidenceInterval();
        updateRowVisibility();
        setReportConfigHeaderIcons();
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
	a_edit->setEnabled(rc_enabled_);
	QAction* a_delete = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	a_delete->setEnabled(rc_enabled_ && report_config_->exists(VariantType::RES, row));
	menu.addSeparator();
	QAction* a_comments = menu.addAction(QIcon(":/Icons/Comment.png"), "Show comments");
	QAction* a_distribution = menu.addAction(QIcon(":/Icons/AF_histogram.png"), "Show distribution for " + sys_name_);
    a_distribution->setEnabled(ngsd_user_logged_in_);
	QAction* a_show_svg = menu.addAction("Show repeat allele(s) image");
	a_show_svg->setEnabled(image_loc.exists);
	QAction* a_show_hist = menu.addAction("Show read lengths histogram");
	a_show_hist->setEnabled(hist_loc.exists);
	menu.addSeparator();
	QAction* a_omim_gene = menu.addAction(QIcon(":/Icons/OMIM.png"), "Open OMIM gene page");
	QAction* a_omim = menu.addAction(QIcon(":/Icons/OMIM.png"), "Open OMIM disease page(s)");
	QAction* a_stripy = menu.addAction("Open STRipy locus information");
	QAction* a_strchive = menu.addAction(QIcon(":/Icons/strchive.png"), "Open STRchive locus information");
	menu.addSeparator();
	QAction* a_copy = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy all");
	QAction* a_copy_sel = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy selection");
	menu.addSeparator();
	QAction* a_google = menu.addAction(QIcon("://Icons/Google.png"), "Google");
	QAction* a_pubmed = menu.addAction(QIcon("://Icons/PubMed.png"), "PubMed");
	foreach(const GeneDB& db, GeneInfoDBs::all())
	{
		menu.addAction(db.icon, db.name);
	}

    //execute menu
	QAction* action = menu.exec(ui_.table->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action==a_show_svg)
	{
		QString filename = image_loc.filename;
		if (!ClientHelper::isClientServerMode()) filename = QFileInfo(image_loc.filename).absoluteFilePath();

		QDesktopServices::openUrl(QUrl(filename));
	}
	else if (action==a_show_hist)
	{
		QByteArray svg;
		VersatileFile file(hist_loc.filename);
		if (!file.open(QIODevice::ReadOnly, false))
		{
			QMessageBox::warning(this, "RE historgam", "Could not open histogram SVG file!");
			return;
		}
		svg = file.readAll();

        QSvgRenderer renderer(svg);
        if (!renderer.isValid()) {
            QMessageBox::warning(this, "SVG error", "Failed to load SVG file");
            return;
        }
        QSize svg_size = renderer.viewBox().size();

        // New pixmap for SVG rendering
        QPixmap pixmap(svg_size);
        pixmap.fill(Qt::transparent);

        // Draw SVG content inside the pixmap
        QPainter painter(&pixmap);
        renderer.render(&painter);

        // Display the pixmap inside QLabel
        QLabel *label = new QLabel;
        label->setPixmap(pixmap);
        label->setMinimumSize(svg_size);

		QScrollArea* scroll_area = new QScrollArea(this);
		scroll_area->setFrameStyle(QFrame::NoFrame);
        scroll_area->setWidget(label);
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
	else if (action==a_omim_gene)
	{
		QString gene_symbol = res_[row].geneSymbol();
		NGSD db;
		QString mim = db.getValue("SELECT mim FROM omim_gene WHERE gene LIKE '" + gene_symbol + "'", true).toString();
		if (!mim.isEmpty())
		{
			QDesktopServices::openUrl(QUrl("https://www.omim.org/entry/" + mim));
		}
	}
	else if (action==a_omim)
    {
        QRegularExpression mim_exp("([0-9]{6})");
        QString text = getCell(row, "OMIM disease IDs");

        QRegularExpressionMatchIterator it = mim_exp.globalMatch(text);
        while (it.hasNext())
        {
            QRegularExpressionMatch match = it.next();
            QDesktopServices::openUrl(QUrl("https://www.omim.org/entry/" + match.captured(1)));
        }
	}
	else if (action==a_stripy)
	{
		QString name = getCell(row, "repeat ID");
		QDesktopServices::openUrl(QUrl("https://stripy.org/database/" + name));
	}
	else if (action==a_strchive)
	{
		QString name = getCell(row, "repeat ID");

		NGSD db;
		QString id = getRepeatId(db, row, false);
		QString url = db.getValue("SELECT strchive_link FROM repeat_expansion WHERE id='"+id + "'").toString().trimmed();

		if (url=="")
		{
			QMessageBox::warning(this, "STRchive", "No STRchive URL available in NGSD for repeat with name '"+name+"'");
		}
		else
		{
			QDesktopServices::openUrl(QUrl(url));
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
			QString sys_id = db.getValue("SELECT id FROM processing_system WHERE name_manufacturer LIKE '" + sys_name_ + "'").toString();
			QVector<double> lengths = db.getValuesDouble("SELECT reg.allele1 FROM repeat_expansion_genotype reg, processed_sample ps WHERE ps.id=reg.processed_sample_id AND reg.repeat_expansion_id=" + id + " AND ps.processing_system_id=" + sys_id);
			lengths << db.getValuesDouble("SELECT reg.allele2 FROM repeat_expansion_genotype reg, processed_sample ps WHERE ps.id=reg.processed_sample_id AND reg.repeat_expansion_id=" + id + " AND ps.processing_system_id=" + sys_id);

			//determine min, max and bin size
			std::sort(lengths.begin(), lengths.end());
			double min = 0;
			int max_bin = 0.995 * lengths.count();
			double max = lengths[max_bin];
			double median = BasicStatistics::median(lengths, false);
			if (2*median > max) max = 2*median;

			//increase upper limit to show current RE
			double cur_max = -1;
			foreach (const QString& gt_str, getCell(row, "genotype").split("/"))
			{
				double gt = 1.1 * Helper::toDouble(gt_str, "genotype", title); //current genotype + 10% offset
				cur_max = std::max(cur_max, gt);
			}
			max = std::max(max, cur_max);

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
	else if (action==a_google)
	{
		QDesktopServices::openUrl(QUrl("https://www.google.com/search?q="+res_[row].geneSymbol()+"+repeat"));
	}
	else if (action==a_pubmed)
	{
		QDesktopServices::openUrl(QUrl("https://pubmed.ncbi.nlm.nih.gov/?term=" + res_[row].geneSymbol()+"+repeat"));
	}
	else //fallback to gene databases
	{
		QString gene_symbol = res_[row].geneSymbol();
		GeneInfoDBs::openUrl(action->text(), gene_symbol);
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

QString RepeatExpansionWidget::getRepeatExpansionFieldById(DBTable& table_data, QString field, QString& id)
{
    for (int i=0; i<table_data.rowCount(); i++)
    {
        if (table_data.row(i).id()==id)
        {
            int column_index = table_data.columnIndex(field);
            return table_data.row(i).value(column_index).trimmed();
        }
    }
    return "";
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
        item->setBackground(QBrush(QColor(bg_color)));
	}
}

void RepeatExpansionWidget::updateReportConfigHeaderIcon(int row)
{
	QIcon report_icon;
	if (report_config_->exists(VariantType::RES, row))
	{
		const ReportVariantConfiguration& rc = report_config_->get(VariantType::RES, row);
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
		QTableWidgetItem* item = setCell(row_idx, "region", re.region().toString(true));
		item->setToolTip("region size: " + QString::number(re.region().length()));

		//repreat unit
		item = setCell(row_idx, "repeat unit", re.unit());
		item->setToolTip("units in region: " + QString::number((double) re.region().length() / re.unit().trimmed().length(), 'f', 2));

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

		//reads spanning
		setCell(row_idx, "reads spanning", re.readsSpanning());
		int min_spanning = 999;
		foreach(QString spanning_count, re.readsSpanning().split('/'))
		{
			spanning_count = spanning_count.trimmed();
			if (Helper::isNumeric(spanning_count))
			{
				min_spanning = std::min(spanning_count.toInt(), min_spanning);
			}
		}
		if (min_spanning<3)
		{
			setCellDecoration(row_idx, "reads spanning", "Less than 3 spanning reads", yellow_);
		}
	}
}

void RepeatExpansionWidget::loadMetaDataFromNGSD()
{
    if (!ngsd_user_logged_in_) return;

    NGSD db;
    SqlQuery re_query = db.getQuery();
    re_query.exec("SELECT * FROM repeat_expansion");    
    DBTable re_table = db.createTable("repeat_expansion", "SELECT * FROM repeat_expansion");

    //get infos from NGSD
    for (int row=0; row<ui_.table->rowCount(); ++row)
    {
        //check if repeat is in NGSD
        QString region = getCell(row, "region");
        QString repeat_unit = getCell(row, "repeat unit");

        QString id;
        for (int re_index=0; re_index<re_table.rowCount(); re_index++)
        {
            int region_index = re_table.columnIndex("region");
            int repeat_unit_index = re_table.columnIndex("repeat_unit");

            if (re_table.row(re_index).value(region_index)==region && re_table.row(re_index).value(repeat_unit_index)==repeat_unit)
            {
                id = re_table.row(re_index).id();
            }
        }
        if (id.isEmpty())
        {
            setCellDecoration(row, "repeat ID", "Repeat not found in NGSD", orange_);
            continue;
        }

        //max_normal
        QString max_normal = getRepeatExpansionFieldById(re_table, "max_normal", id);
        setCell(row, "max. normal", max_normal);

        //min_pathogenic
        QString min_pathogenic = getRepeatExpansionFieldById(re_table, "min_pathogenic", id);
        setCell(row, "min. pathogenic", min_pathogenic);

        //inheritance
        QString inheritance = getRepeatExpansionFieldById(re_table, "inheritance", id);
        setCell(row, "inheritance", inheritance);

        //location
        QString location = getRepeatExpansionFieldById(re_table, "location", id);
        setCell(row, "location", location);

        //diseases
        QString disease_names = getRepeatExpansionFieldById(re_table, "disease_names", id);
        setCell(row, "diseases", disease_names);

        //OMIM IDs
        QStringList disease_ids_omim = getRepeatExpansionFieldById(re_table, "disease_ids_omim", id).split(",");
        QString omim_like_clause = "";
        QString omim_when_clause = "";
        for (const QString &id : disease_ids_omim)
        {
            QString disease_id = id.trimmed();
            if (disease_id.isEmpty()) continue;
            if (!omim_like_clause.isEmpty())
            {
                omim_like_clause += " OR ";
                omim_when_clause += " ";
            }
            omim_like_clause += "phenotype LIKE '%" + disease_id + "%'";
            omim_when_clause += "WHEN phenotype LIKE '%" + disease_id + "%' THEN '" + disease_id + "'";
        }
        if (!omim_when_clause.isEmpty()) omim_when_clause = ", CASE " + omim_when_clause + " ELSE NULL END AS matched_disease_id";

        QStringList names;
        if (!omim_when_clause.isEmpty() && !omim_like_clause.isEmpty())
        {
            SqlQuery omim_query = db.getQuery();
            omim_query.exec("SELECT phenotype" + omim_when_clause + " FROM omim_phenotype WHERE " + omim_like_clause);
            while (omim_query.next())
            {
                QString disease_id = omim_query.value(1).toString().trimmed();
                QString name = omim_query.value(0).toString().trimmed();
                name.replace(disease_id, "");
                name.replace("(3)", "");
                name = name.trimmed();
                while(name.endsWith(',') || name.endsWith(' ')) name.chop(1);
                if (!name.isEmpty()) names <<  disease_id + " - " + name;
            }
        }
        setCell(row, "OMIM disease IDs", names.join("; "));

        //comments
        QStringList comments_list = getRepeatExpansionFieldById(re_table, "comments", id).replace("<br>", "\n").trimmed().split("\n");
        for (int comment_index=0; comment_index<comments_list.count(); ++comment_index)
        {
            QString line = comments_list[comment_index].trimmed();
            if (line.startsWith('#') && line.endsWith('#'))
            {
                comments_list[comment_index] = "<b>" + line.mid(1, line.length()-2) + "</b>";
            }
        }

        QString comments = comments_list.join("<br>");
        QTableWidgetItem* item = setCell(row, "comments", "");
        if (!comments.isEmpty())
        {
            setCellDecoration(row, "comments", comments);
            item->setIcon(QIcon(":/Icons/Info.png"));
        }

        //HPO terms
        QStringList hpo_terms = getRepeatExpansionFieldById(re_table, "hpo_terms", id).split(",");
        QString hpo_like_clause = "";
        for (const QString &term : hpo_terms)
        {
            if (!hpo_like_clause.isEmpty()) hpo_like_clause += " OR ";
            hpo_like_clause += "hpo_id LIKE '" + term.trimmed() + "'";
        }
        if (!hpo_like_clause.isEmpty())
        {
            QSqlQuery hpo_query = db.getQuery();
            hpo_query.exec("SELECT hpo_id, name FROM hpo_term WHERE " + hpo_like_clause);

            int hpo_terms_counter = -1;
            while(hpo_query.next())
            {
                hpo_terms_counter++;
                QString name = hpo_query.value(1).toString();
                if (!name.isEmpty())
                {
                    hpo_terms[hpo_terms_counter] = hpo_query.value(0).toString().trimmed() + " - " + name;
                }
                else
                {
                    hpo_terms[hpo_terms_counter] = hpo_query.value(0).toString().trimmed();
                }
            }
        }
        setCell(row, "HPO terms", hpo_terms.join("; "));

        //type
        QString type = getRepeatExpansionFieldById(re_table, "type", id);
        setCell(row, "type", type);

        //inhouse_testing
        QString inhouse_testing = getRepeatExpansionFieldById(re_table, "inhouse_testing", id)=="1" ? "yes" : "no";
        setCell(row, "in-house testing", inhouse_testing);

        //statistical cutoff
        if (!sys_type_cutoff_col_.isEmpty())
        {
            QString cutoff = getRepeatExpansionFieldById(re_table, sys_type_cutoff_col_, id);
            setCell(row, "statistical cutoff", cutoff);
        }
    }
}

void RepeatExpansionWidget::colorRepeatCountBasedOnCutoffs()
{
	for (int row=0; row<ui_.table->rowCount(); ++row)
	{
		bool ok = false;
		int tmp = -1;

		//determine cutoffs
		int max_normal = -1;
		tmp = getCell(row, "max. normal").toInt(&ok);
		if(ok) max_normal = tmp;
		int min_pathogenic = -1;
		tmp = getCell(row, "min. pathogenic").toInt(&ok);
		if(ok) min_pathogenic = tmp;
		double statistical_cutoff = -1.0;
		double tmp2 = getCell(row, "statistical cutoff").toDouble(&ok);
		if(ok) statistical_cutoff = tmp2;
		if (max_normal==-1 && min_pathogenic==-1 && statistical_cutoff==-1) continue;

		//determine maximum repeat count of alleles
		QStringList genotypes = getCell(row, "genotype").split("/");
		int max = -1;
		foreach(QString geno, genotypes)
		{
			bool ok = false;
			int repeat_count = std::round(geno.trimmed().toFloat(&ok));
			if (ok)
			{
				max = std::max(max, repeat_count);
			}
		}
		if (max==-1) continue;

		//color
		if (min_pathogenic!=-1 && max>=min_pathogenic)
		{
			setCellDecoration(row, "genotype", "Above min. pathogenic cutoff!", red_);
		}
		else if (max_normal!=-1 && max>max_normal)
		{
			setCellDecoration(row, "genotype", "Above max. normal cutoff!", orange_);
		}
		else if (statistical_cutoff!=-1.0 && max>=statistical_cutoff)
		{
			setCellDecoration(row, "genotype", "Above statistical cutoff!", yellow_);
		}
	}
}

void RepeatExpansionWidget::colorRepeatCountConfidenceInterval()
{
	for (int row=0; row<ui_.table->rowCount(); ++row)
	{
		//determine cutoff
		bool ok = false;
		int min_pathogenic = getCell(row, "min. pathogenic").toInt(&ok);
		if (!ok) continue;

		//determine max CI
		int max_ci = -1;
		QStringList ci_values = getCell(row, "genotype CI").replace("-", " ").replace("/", " ").split(" ");
		foreach(QString ci_value, ci_values)
		{
			bool ok = false;
			int value = ci_value.toInt(&ok);
			if (ok && value>=0)
			{
				max_ci = value;
			}
		}

		//color max CI
		if (max_ci>min_pathogenic)
		{
			setCellDecoration(row, "genotype CI", "Above min. pathogenic cutoff!", yellow_);
		}
	}
}

void RepeatExpansionWidget::setReportConfigHeaderIcons()
{
	if(report_config_==NULL) return;

    QSet<int> report_variant_indices = Helper::listToSet(report_config_->variantIndices(VariantType::RES, false));
	for(int r=0; r<res_.count(); ++r)
	{
        QTableWidgetItem* header_item = GUIHelper::createTableItem(QByteArray::number(r+1));
		if (report_variant_indices.contains(r))
		{
			const ReportVariantConfiguration& rc = report_config_->get(VariantType::RES, r);
			header_item->setIcon(VariantTable::reportIcon(rc.showInReport(), rc.causal));
		}
		ui_.table->setVerticalHeaderItem(r, header_item);
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
	if (ui_.filter_expanded->currentText()=="possibly expanded")
	{
		int col = GUIHelper::columnIndex(ui_.table, "genotype");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
            QColor color = ui_.table->item(row, col)->background().color();
			if (color!=red_ && color!=orange_ && color!=yellow_) hidden[row] = true;
		}
	}
	if (ui_.filter_expanded->currentText()=="pathogenic")
	{
		int col = GUIHelper::columnIndex(ui_.table, "genotype");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
            if (ui_.table->item(row, col)->background().color()!=red_) hidden[row] = true;
		}
	}

	//HPO filter
	if (ui_.filter_hpo->isChecked())
	{
		//determine hpo subtree of patient
		PhenotypeList pheno_subtrees = GlobalServiceProvider::filterWidget()->phenotypes();
		NGSD db;
        for (const Phenotype& pheno : GlobalServiceProvider::filterWidget()->phenotypes())
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
				foreach(QByteArray hpo_info, item->text().toUtf8().split(';'))
				{
					QByteArray hpo_acc = hpo_info.split('-').at(0).trimmed();
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

	//RC filter
	if (ui_.filter_rc->isChecked() && report_config_!=NULL)
	{
        QSet<int> report_variant_indices = Helper::listToSet(report_config_->variantIndices(VariantType::RES, false));
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			if (!report_variant_indices.contains(row)) hidden[row] = true;
		}
	}

	//repeat ID text search
	QString id_search_str = ui_.filter_id->text().trimmed();
	if (!id_search_str.isEmpty())
	{
		int col_repeat_id = GUIHelper::columnIndex(ui_.table, "repeat ID");
		int col_diseases = GUIHelper::columnIndex(ui_.table, "diseases");
		int col_omim = GUIHelper::columnIndex(ui_.table, "OMIM disease IDs");
		for (int row=0; row<ui_.table->rowCount(); ++row)
		{
			if (!ui_.table->item(row, col_repeat_id)->text().contains(id_search_str, Qt::CaseInsensitive) && !ui_.table->item(row, col_diseases)->text().contains(id_search_str, Qt::CaseInsensitive) && !ui_.table->item(row, col_omim)->text().contains(id_search_str, Qt::CaseInsensitive))
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
    if (!ngsd_user_logged_in_) return;
	editReportConfiguration(row);
}

void RepeatExpansionWidget::svHeaderContextMenu(QPoint pos)
{
	if (!rc_enabled_) return;

	//get variant index
	int row = ui_.table->verticalHeader()->visualIndexAt(pos.ry());

	//set up menu
	QMenu menu(ui_.table->verticalHeader());
	QAction* a_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	a_edit->setEnabled(rc_enabled_);
	QAction* a_delete = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	a_delete->setEnabled(rc_enabled_ && report_config_->exists(VariantType::RES, row));

	//exec menu
	pos = ui_.table->verticalHeader()->viewport()->mapToGlobal(pos);
	QAction* action = menu.exec(pos);
	if (action==nullptr) return;

    if(!ngsd_user_logged_in_) return; //do nothing if no access to NGSD

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
