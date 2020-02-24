#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QClipboard>
#include <QBitArray>
#include <QByteArray>
#include <QMenu>
#include <QAction>
#include <QCompleter>
#include "SvWidget.h"
#include "ui_SvWidget.h"
#include "Helper.h"
#include "GUIHelper.h"
#include "PhenotypeSelectionWidget.h"
#include "Settings.h"
#include "Log.h"

SvWidget::SvWidget(const QStringList& bedpe_file_paths, FilterWidget* variant_filter_widget, const GeneSet& het_hit_genes, QHash<QByteArray, BedFile>& cache, QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::SvWidget)
	, variant_filter_widget_(variant_filter_widget)
	, var_het_genes_(het_hit_genes)
	, gene2region_cache_(cache)
{
	ui->setupUi(this);
	ui->svs->setContextMenuPolicy(Qt::CustomContextMenu);

	//link variant filter widget to FilterWidgetSV
	ui->filter_widget->setVariantFilterWidget(variant_filter_widget);

	//Setup signals and slots
	connect(ui->copy_to_clipboard,SIGNAL(clicked()),this,SLOT(copyToClipboard()));

	connect(ui->svs,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(SvDoubleClicked(QTableWidgetItem*)));

	connect(ui->svs,SIGNAL(itemSelectionChanged()),this,SLOT(SvSelectionChanged()));

	connect(ui->svs,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));


	//signals from the filter widget
	connect(ui->filter_widget, SIGNAL(filtersChanged()), this, SLOT(applyFilters()));

	//clear GUI
	clearGUI();

	if(bedpe_file_paths.isEmpty())
	{
		disableGUI("There is no SV file in the sample folder.");
	}
	else
	{
		loadSVs(bedpe_file_paths[0]);

		//Disable filters that cannot apply for tumor normal pairs (data is expanded already)
		if(sv_bedpe_file_.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
		{
			ui->info_a->setEnabled(false);
			ui->info_b->setEnabled(false);
			ui->sv_details->setEnabled(false);

		}
	}
}

void SvWidget::loadSVs(const QString &file_name)
{
	loading_svs_ = true;

	//clear GUI
	clearGUI();

	sv_bedpe_file_.clear();
	sv_bedpe_file_.load(file_name);

	if(sv_bedpe_file_.count() == 0)
	{
		disableGUI("There are no SVs in the data file");
		loading_svs_ = false;
		return;
	}

	//Set list of annotations to be showed, by default some annotations are filtered out
	QByteArrayList annotation_headers = sv_bedpe_file_.annotationHeaders();

	for(int i=0;i<annotation_headers.count();++i)
	{
		if(annotation_headers[i].contains("STRAND_")) continue;
		if(annotation_headers[i].contains("NAME_")) continue;
		if(annotation_headers[i] == "ID") continue;
		annotations_to_show_ << annotation_headers[i];
	}

	//Add annotation headers
	QList<int> annotation_indices;
	for(int i=0;i<sv_bedpe_file_.annotationHeaders().count();++i)
	{
		QByteArray header = sv_bedpe_file_.annotationHeaders()[i];

		if(!annotations_to_show_.contains(header)) continue;

		ui->svs->setColumnCount(ui->svs->columnCount() + 1 );
		ui->svs->setHorizontalHeaderItem(ui->svs->columnCount() - 1, new QTableWidgetItem(QString(header)));
		annotation_indices << i;
	}


	//Fill rows
	ui->svs->setRowCount(sv_bedpe_file_.count());

	//Fill table widget with data from bedpe file
	for(int row=0;row<sv_bedpe_file_.count();++row)
	{
		//Fill fixed columns
		ui->svs->setItem(row,0,new QTableWidgetItem(QString(sv_bedpe_file_[row].chr1().str())));
		ui->svs->setItem(row,1,new QTableWidgetItem(QString::number(sv_bedpe_file_[row].start1())));
		ui->svs->setItem(row,2,new QTableWidgetItem(QString::number(sv_bedpe_file_[row].end1())));

		ui->svs->setItem(row,3,new QTableWidgetItem(QString(sv_bedpe_file_[row].chr2().str())));
		ui->svs->setItem(row,4,new QTableWidgetItem(QString::number(sv_bedpe_file_[row].start2())));
		ui->svs->setItem(row,5,new QTableWidgetItem(QString::number(sv_bedpe_file_[row].end2())));

		//Fill annotation columns
		int col_in_widget = 6;
		foreach(int anno_index,annotation_indices)
		{
			ui->svs->setItem(row,col_in_widget,new QTableWidgetItem(QString(sv_bedpe_file_[row].annotations().at(anno_index))));
			++col_in_widget;
		}
	}

	//set entries for SV filter columns filter
	QStringList valid_filter_entries;
	foreach (const QByteArray& entry, sv_bedpe_file_.annotationDescriptionByID("FILTER").keys())
	{
		valid_filter_entries.append(entry);
	}
	valid_filter_entries.removeAll("PASS");
	valid_filter_entries.prepend("PASS");
	ui->filter_widget->setValidFilterEntries(valid_filter_entries);


	//only whole rows can be selected, one row at a time
	ui->svs->setSelectionBehavior(QAbstractItemView::SelectRows);
	resizeQTableWidget(ui->svs);

	loading_svs_ = false;

	//filter rows according default filter values
	applyFilters();
}


void SvWidget::clearGUI()
{
	ui->error_label->setHidden(true);
	ui->label_size->clear();
	ui->label_pe_af->clear();
	ui->label_sr_af->clear();
	ui->number_of_svs->clear();

	//clear info widgets
	ui->info_a->setRowCount(0);
	ui->info_b->setRowCount(0);
	ui->sv_details->setRowCount(0);

	//clear table widget to cols / rows specified in .ui file
	ui->svs->setRowCount(0);
	ui->svs->setColumnCount(6);

}

void SvWidget::resizeQTableWidget(QTableWidget *table_widget)
{
	table_widget->resizeRowToContents(0);

	int height = table_widget->rowHeight(0);

	for (int i=0; i<table_widget->rowCount(); ++i)
	{
		table_widget->setRowHeight(i, height);
	}
	GUIHelper::resizeTableCells(table_widget,200);
}


void SvWidget::applyFilters(bool debug_time)
{
	//skip if not necessary
	if (loading_svs_) return;
	int row_count = ui->svs->rowCount();
	if (row_count==0) return;

	FilterResult filter_result(row_count);

	// filters based on FilterWidgetSV

	try
	{
		QTime timer;
		timer.start();

		// filter by FilterCascade
		const FilterCascade& filter_cascade = ui->filter_widget->filters();

		//set comp-het gene list the first time the filter is applied
		for(int i=0; i<filter_cascade.count(); ++i)
		{
			const FilterSvCompHet* comphet_filter = dynamic_cast<const FilterSvCompHet*>(filter_cascade[i].data());
			if (comphet_filter!=nullptr && comphet_filter->hetHitGenes().count()!=var_het_genes_.count())
			{
				comphet_filter->setHetHitGenes(var_het_genes_);
			}
		}

		// apply filters of the filter cascade
		filter_result = filter_cascade.apply(sv_bedpe_file_, false, debug_time);
		ui->filter_widget->markFailedFilters();

		if (debug_time)
		{
			Log::perf("Applying annotation filters took ", timer);
			timer.start();
		}


		//filter by ROI
		QString roi = ui->filter_widget->targetRegion();
		if (roi!=roi_filename_) //update roi regions if it changed
		{
			roi_filename_ = "";
			roi_.clear();

			if (roi!="")
			{
				roi_.load(roi);
				roi_.merge();
				roi_filename_ = roi;
			}
		}
		if (roi!="") //perform actual filtering
		{
			for(int row=0; row<row_count; ++row)
			{
				if(!filter_result.flags()[row]) continue;

				if (!sv_bedpe_file_[row].intersectsWith(roi_, true)) filter_result.flags()[row] = false;
			}
		}

		//filter by region
		QString region_text = ui->filter_widget->region();
		BedLine region = BedLine::fromString(region_text);
		if (!region.isValid()) //check if valid chr
		{
			Chromosome chr(region_text);
			if (chr.isNonSpecial())
			{
				region.setChr(chr);
				region.setStart(1);
				region.setEnd(999999999);
			}
		}
		if (region.isValid()) //valid region (chr,start, end or only chr)
		{
			for(int row=0; row<row_count; ++row)
			{
				if (!filter_result.flags()[row]) continue;

				const BedpeLine& sv = sv_bedpe_file_[row];
				filter_result.flags()[row] = sv.intersectsWith(BedFile(region.chr(), region.start(), region.end()), true);
			}
		}


		//filter by phenotype (via genes, not genomic regions)
		QList<Phenotype> phenotypes = ui->filter_widget->phenotypes();
		if (!phenotypes.isEmpty())
		{
			//convert phenotypes to genes
			NGSD db;
			GeneSet pheno_genes;
			foreach(const Phenotype& pheno, phenotypes)
			{
				pheno_genes << db.phenotypeToGenes(pheno, true);
			}

			//convert genes to ROI (using a cache to speed up repeating queries)
			BedFile pheno_roi;
			foreach(const QByteArray& gene, pheno_genes)
			{
				if (!gene2region_cache_.contains(gene))
				{
					BedFile tmp = db.geneToRegions(gene, Transcript::ENSEMBL, "gene", true);
					tmp.clearAnnotations();
					tmp.extend(5000);
					tmp.merge();
					gene2region_cache_[gene] = tmp;
				}
				pheno_roi.add(gene2region_cache_[gene]);
			}
			pheno_roi.merge();

			for(int row=0; row<row_count; ++row)
			{
				if (!filter_result.flags()[row]) continue;

				filter_result.flags()[row] = sv_bedpe_file_[row].intersectsWith(pheno_roi, true);
			}
		}

		//filter by genes

		GeneSet gene_whitelist = ui->filter_widget->genes();
		if (!gene_whitelist.isEmpty())
		{
			QByteArray genes_joined = gene_whitelist.join('|');

			// get column index of 'GENES' column
			int i_genes = sv_bedpe_file_.annotationIndexByName("GENES", false);
			if (i_genes == -1)
			{
				QMessageBox::warning(this, "Filtering error", "BEDPE files does not contain a 'GENES' column! \nFiltering based on genes is not possible. Please reannotate the structural variant file.");

			}
			else
			{
				if (genes_joined.contains("*")) //with wildcards
				{
					QRegExp reg(genes_joined.replace("-", "\\-").replace("*", "[A-Z0-9-]*"));
					for(int row=0; row<row_count; ++row)
					{
						if (!filter_result.flags()[row]) continue;

						// generate GeneSet from column text
						GeneSet sv_genes = GeneSet::createFromText(sv_bedpe_file_[row].annotations()[i_genes], ',');

						bool match_found = false;
						foreach(const QByteArray& sv_gene, sv_genes)
						{
							if (reg.exactMatch(sv_gene))
							{
								match_found = true;
								break;
							}
						}
						filter_result.flags()[row] = match_found;
					}
				}
				else //without wildcards
				{
					for(int row=0; row<row_count; ++row)
					{
						if (!filter_result.flags()[row]) continue;

						// generate GeneSet from column text
						GeneSet sv_genes = GeneSet::createFromText(sv_bedpe_file_[row].annotations()[i_genes], ',');

						filter_result.flags()[row] = sv_genes.intersectsWith(gene_whitelist);
					}
				}
			}
		}



		//filter annotations by text
		QByteArray text = ui->filter_widget->text().trimmed().toLower();
		if (text!="")
		{
			for(int row=0; row<row_count; ++row)
			{
				if (!filter_result.flags()[row]) continue;

				bool match = false;
				foreach(const QByteArray& anno, sv_bedpe_file_[row].annotations())
				{
					if (anno.toLower().contains(text))
					{
						match = true;
						break;
					}
				}
				filter_result.flags()[row] = match;
			}
		}
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nTry re-annotating the NGSD columns.\n If re-annotation does not help, please re-analyze the sample (starting from annotation) in the sample information dialog!");

		filter_result = FilterResult(row_count, false);
	}

	//hide rows not passing filters
	for(int row=0; row<row_count; ++row)
	{
		ui->svs->setRowHidden(row, !filter_result.flags()[row]);
	}

	//Set number of filtered / total SVs
	ui->number_of_svs->setText(QByteArray::number(filter_result.flags().count(true)) + "/" + QByteArray::number(row_count));
}

int SvWidget::colIndexbyName(const QString& name)
{
	for(int i=0;i<ui->svs->columnCount();++i)
	{
		if(ui->svs->horizontalHeaderItem(i)->text() == name) return i;
	}
	return -1;
}

int SvWidget::pairedEndReadCount(int row)
{
	int i_format =colIndexbyName("FORMAT");
	if(i_format == -1) return -1;

	QByteArray desc = ui->svs->item(row,i_format)->text().toUtf8();
	QByteArray data = ui->svs->item(row,i_format+1)->text().toUtf8();

	QByteArrayList infos = getFormatEntryByKey("PR",desc,data).split(',');
	if(infos.count() != 2) return -1;
	//return paired end read alteration count
	bool success = false ;

	int pe_reads = infos[1].trimmed().toInt(&success);
	if(!success) return -1;
	return pe_reads;
}


double SvWidget::alleleFrequency(int row,const QByteArray& read_type)
{
	int i_format = colIndexbyName("FORMAT");
	if(i_format == -1 ) return -1.;

	if(ui->svs->columnCount() < i_format+1) return -1.;

	QByteArray desc = ui->svs->item(row,i_format)->text().toUtf8();
	QByteArray data = ui->svs->item(row,i_format+1)->text().toUtf8();

	int count_ref = 0;
	int count_alt = 0;

	QByteArrayList pr;
	if(read_type == "PR") pr = getFormatEntryByKey("PR",desc,data).split(',');
	else if(read_type == "SR") pr =  getFormatEntryByKey("SR",desc,data).split(',');
	else return -1.;

	if(pr.count() != 2) return -1.;

	bool success = false;
	count_ref = pr[0].toInt(&success);
	if(!success) return -1;
	count_alt = pr[1].toInt(&success);
	if(!success) return -1;

	if(count_alt+count_ref != 0) return (double)count_alt / (count_alt+count_ref);
	else return 0;
}

void SvWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui->svs);
}


void SvWidget::SvDoubleClicked(QTableWidgetItem *item)
{
	if (item==nullptr) return;

	int row = item->row();
	QString coords = sv_bedpe_file_[row].positionRange();

	emit openSvInIGV(coords);
}

void SvWidget::disableGUI(const QString& message)
{
	setEnabled(false);

	ui->error_label->setHidden(false);
	ui->error_label->setText("<font color='red'>" + message + "</font>");
}

QByteArray SvWidget::getFormatEntryByKey(const QByteArray& key, const QByteArray &format_desc, const QByteArray &format_data)
{
	QByteArray key_tmp = key.trimmed();

	QByteArrayList descs = format_desc.split(':');
	QByteArrayList datas = format_data.split(':');
	if(descs.count() != datas.count()) return "";

	for(int i=0;i<descs.count();++i)
	{
		if(descs[i] == key_tmp)
		{
			return datas[i];
		}
	}

	return "";
}

void SvWidget::SvSelectionChanged()
{
	if(sv_bedpe_file_.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL) return; //Skip somatic lists because info columns are expanded already

	QModelIndexList rows = ui->svs->selectionModel()->selectedRows();
	if(rows.count() != 1) return;

	int row = rows.at(0).row();

	int i_format = colIndexbyName("FORMAT");

	int i_format_data = i_format+1;
	//Check whether col with format data actually exists
	if(ui->svs->columnCount()-1 < i_format_data ) return;

	QStringList format = ui->svs->item(row,i_format)->text().split(":");
	QStringList data = ui->svs->item(row,i_format_data)->text().split(":");
	if(format.count() != data.count()) return;
	ui->sv_details->setRowCount(format.count());

	//Map with description of format field, e.g. GT <-> GENOTYPE
	QMap<QByteArray,QByteArray> format_description = sv_bedpe_file_.annotationDescriptionByID("FORMAT");

	for(int i=0;i<ui->sv_details->rowCount();++i)
	{
		ui->sv_details->setItem(i,0,new QTableWidgetItem(QString(format[i])));
		ui->sv_details->setItem(i,1,new QTableWidgetItem(QString(format_description.value(format.at(i).toLatin1()))));
		ui->sv_details->setItem(i,2,new QTableWidgetItem(QString(data[i])));
	}
	resizeQTableWidget(ui->sv_details);
	ui->sv_details->scrollToTop();

	setInfoWidgets("INFO_A",row,ui->info_a);
	resizeQTableWidget(ui->info_a);
	setInfoWidgets("INFO_B",row,ui->info_b);
	resizeQTableWidget(ui->info_b);

	//size
	int size = sv_bedpe_file_[row].size();
	ui->label_size->setText(size==-1 ? "" : "Size: " + QString::number(size));

	//Display Split Read AF of variant
	ui->label_sr_af->setText("Split Read AF: " + QString::number(alleleFrequency(row, "SR"), 'f',2));

	//Display Paired End Read AF of variant
	ui->label_pe_af->setText("Paired End Read AF: " + QString::number(alleleFrequency(row, "PR"), 'f',2));
}

void SvWidget::setInfoWidgets(const QByteArray &name, int row, QTableWidget* widget)
{
	int i_info = colIndexbyName(name);
	if(i_info == -1)
	{
		QMessageBox::warning(this,"Error parsing annotation","Could not parse annotation column " + name);
		return;
	}

	QStringList raw_data = ui->svs->item(row,i_info)->text().split(";");

	if(raw_data.count() > 1) widget->setRowCount(raw_data.count());
	else if(raw_data.count() == 1 && (raw_data[0] == "." || raw_data[0] == "MISSING")) widget->setRowCount(0);
	else widget->setRowCount(raw_data.count());

	QMap<QByteArray,QByteArray> descriptions = sv_bedpe_file_.annotationDescriptionByID("INFO");

	for(int i=0;i<raw_data.count();++i)
	{
		QStringList tmp = raw_data[i].split("=");
		if(tmp.count() != 2 ) continue;

		widget->setItem(i,0,new QTableWidgetItem(QString(tmp[0]))); // Key

		widget->setItem(i,1, new QTableWidgetItem(QString(descriptions.value(tmp[0].toUtf8())))); //Description
		widget->setItem(i,2,new QTableWidgetItem(QString(tmp[1]))); // Value
	}

	resizeQTableWidget(widget);
	widget->scrollToTop();
}

void SvWidget::showContextMenu(QPoint pos)
{
	QModelIndexList rows = ui->svs->selectionModel()->selectedRows();
	if(rows.count() != 1) return;

	//show menu
	QMenu menu(ui->svs);
	QAction* igv_pos1 = menu.addAction("Open position A in IGV");
	QAction* igv_pos2 = menu.addAction("Open position B in IGV");
	QAction* igv_split = menu.addAction("Open position A/B in IGV split screen");
	menu.addSeparator();
	QAction* copy_pos1 = menu.addAction("Copy position A to clipboard");
	QAction* copy_pos2 = menu.addAction("Copy position B to clipboard");

	QAction* action = menu.exec(ui->svs->viewport()->mapToGlobal(pos));
	if (action == nullptr) return;

	//open in IGV
	int index = rows.at(0).row();
	const BedpeLine& sv = sv_bedpe_file_[index];
	if (action == igv_pos1)
	{
		emit(openSvInIGV(sv.position1()));
	}
	else if (action == igv_pos2)
	{
		emit(openSvInIGV(sv.position2()));
	}
	else if (action == igv_split)
	{
		emit(openSvInIGV(sv.position1() + " " + sv.position2()));
	}
	else if (action == copy_pos1)
	{
		QApplication::clipboard()->setText(sv.position1());
	}
	else if (action == copy_pos2)
	{
		QApplication::clipboard()->setText(sv.position2());
	}
}
