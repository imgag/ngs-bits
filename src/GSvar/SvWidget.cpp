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

SvWidget::SvWidget(const QStringList& bedpe_file_paths, FilterWidget* filter_widget, QHash<QByteArray, BedFile>& cache, QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::SvWidget)
	, filter_widget_(filter_widget)
	, gene2region_cache_(cache)
{
	ui->setupUi(this);
	ui->svs->setContextMenuPolicy(Qt::CustomContextMenu);

	//Setup signals and slots
	connect(ui->copy_to_clipboard,SIGNAL(clicked()),this,SLOT(copyToClipboard()));

	connect(ui->filter_type,SIGNAL(currentIndexChanged(int)),this,SLOT(applyFilters()));
	connect(ui->filter_qual,SIGNAL(currentIndexChanged(int)),this,SLOT(applyFilters()));
	connect(ui->filter_geno,SIGNAL(currentIndexChanged(int)),this,SLOT(applyFilters()));
	connect(ui->filter_text,SIGNAL(textEdited(QString)),this,SLOT(applyFilters()));
	connect(ui->filter_text_search_type,SIGNAL(currentTextChanged(QString)),this,SLOT(applyFilters()));
	connect(ui->filter_quality_score,SIGNAL(valueChanged(int)),this,SLOT(applyFilters()));
	connect(ui->ignore_special_chromosomes,SIGNAL(stateChanged(int)),this,SLOT(applyFilters()));
	connect(ui->filter_pe_af,SIGNAL(valueChanged(double)),this,SLOT(applyFilters()));
	connect(ui->filter_sr_af,SIGNAL(valueChanged(double)),this,SLOT(applyFilters()));

	connect(ui->filter_pe_reads,SIGNAL(valueChanged(int)),this,SLOT(applyFilters()));

	connect(ui->svs,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(SvDoubleClicked(QTableWidgetItem*)));

	connect(ui->svs,SIGNAL(itemSelectionChanged()),this,SLOT(SvSelectionChanged()));

	connect(ui->svs,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));

	FilterWidget::loadTargetRegions(ui->roi);
	connect(ui->roi, SIGNAL(currentIndexChanged(int)), this, SLOT(roiSelectionChanged(int)));
	connect(ui->roi_import, SIGNAL(clicked(bool)), this, SLOT(importROI()));

	connect(ui->hpo, SIGNAL(clicked(QPoint)), this, SLOT(editPhenotypes()));
	connect(ui->hpo, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showPhenotypeContextMenu(QPoint)));
	ui->hpo->setEnabled(Settings::boolean("NGSD_enabled", true));
	connect(ui->hpo_import, SIGNAL(clicked(bool)), this, SLOT(importHPO()));

	//clear GUI
	clearGUI();

	if(bedpe_file_paths.isEmpty())
	{
		disableGUI("There is no SV file in the sample folder.");
	}
	else
	{
		loadSVs(bedpe_file_paths[0]);
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

	//set entries for combobox used for filtering SV quality
	QList<QByteArray> entries = sv_bedpe_file_.annotationDescriptionByID("FILTER").keys();
	entries.removeAll("PASS");
	entries.prepend("PASS");
	entries.prepend("n/a");
	foreach(QByteArray entry, entries)
	{
		ui->filter_qual->addItem(QString(entry));
	}

	//use different standard threshold for total count of paired end reads
	ui->filter_pe_reads->setValue(0);


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

	//clear filters
	ui->filter_type->setCurrentIndex(0);
	ui->filter_qual->clear();
	ui->filter_quality_score->setValue(0);
	ui->filter_geno->setCurrentIndex(0);

	ui->filter_pe_af->setValue(0);
	ui->filter_sr_af->setValue(0);

	ui->ignore_special_chromosomes->setCheckState(Qt::CheckState::Checked);

	ui->filter_text->clear();
	ui->filter_text_search_type->setCurrentIndex(0);

	ui->roi->setCurrentIndex(1); //first is search mode, second is 'none'

	ui->hpo->clear();
	phenotypes_.clear();
	phenotypes_roi_.clear();
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

void SvWidget::applyFilters()
{
	//skip if not necessary
	if (loading_svs_) return;
	int row_count = ui->svs->rowCount();
	if (row_count==0) return;

	QBitArray pass;
	pass.fill(true, row_count);

	//Skip special chromosomes
	if(ui->ignore_special_chromosomes->isChecked())
	{
		for(int row=0; row<row_count; ++row)
		{
			if(!pass[row]) continue;

			const BedpeLine& sv = sv_bedpe_file_[row];
			if (!(sv.chr1().isNonSpecial() && sv.chr2().isNonSpecial())) pass[row] = false;
		}
	}

	//SV type (e.g. DUP,DEL or INV)
	QByteArray filter_type = ui->filter_type->currentText().toLatin1();
	if(filter_type!="n/a")
	{
		StructuralVariantType type = BedpeFile::stringToType(filter_type);
		for(int row=0; row<row_count; ++row)
		{
			if(!pass[row]) continue;

			const BedpeLine& sv = sv_bedpe_file_[row];
			if(sv.type()!=type) pass[row] = false;
		}
	}

	//Free text filter
	QString filter_text_type = ui->filter_text_search_type->currentText().trimmed();
	QString filter_text = ui->filter_text->text().trimmed();
	if(!filter_text.isEmpty() && !filter_text_type.isEmpty())
	{
		bool contains = filter_text_type == "contains";

		for(int row=0; row<row_count; ++row)
		{
			if(!pass[row]) continue;

			bool in_any_item = false;
			for(int c=0;c<ui->svs->columnCount();++c)
			{
				if(ui->svs->item(row,c)->text().contains(filter_text, Qt::CaseInsensitive))
				{
					in_any_item = true;
					break;
				}
			}

			if(contains)
			{
				pass[row] = in_any_item;
			}
			else
			{
				pass[row] = !in_any_item;
			}
		}
	}

	//Genotype
	QString filter_geno = ui->filter_geno->currentText();
	if(filter_geno!="")
	{
		if (filter_geno=="hom")
		{
			filter_geno = "1/1";
		}
		else if (filter_geno=="het")
		{
			filter_geno = "0/1";
		}
		else
		{
			THROW(ProgrammingException, "Unhandled filter genotype: " + filter_geno);
		}

		int i_format = colIndexbyName("FORMAT");
		if(i_format > -1)
		{
			for(int row=0; row<row_count; ++row)
			{
				if(!pass[row]) continue;

				QByteArray desc = ui->svs->item(row, i_format)->text().toUtf8();
				QByteArray data = ui->svs->item(row, i_format+1)->text().toUtf8();
				QByteArray gt = getFormatEntryByKey("GT", desc, data);

				if (gt!=filter_geno) pass[row] = false;
			}
		}
	}

	//SV quality (e.g. PASS, LowFreq etc)
	QString filter_quality_col = ui->filter_qual->currentText();
	if(filter_quality_col != "n/a")
	{
		int i_qual_filter = colIndexbyName("FILTER");
		if(i_qual_filter > -1)
		{
			for(int row=0; row<row_count; ++row)
			{
				if(!pass[row]) continue;

				if(!ui->svs->item(row,i_qual_filter)->text().contains(filter_quality_col)) pass[row] = false;
			}
		}
	}

	//SV quality score
	double filter_quality_score = ui->filter_quality_score->value();
	if(filter_quality_score > 0)
	{
		int i_qual_score = colIndexbyName("QUAL");
		if(i_qual_score != -1)
		{
			for(int row=0; row<row_count; ++row)
			{
				if(!pass[row]) continue;

				bool ok = false;
				double val = ui->svs->item(row,i_qual_score)->text().toDouble(&ok);
				if(!ok) continue;

				//skip values smaller than threshold
				if(val < filter_quality_score) pass[row] = false;
			}
		}
	}

	//Paired End Reads Allele Frequency
	if(ui->filter_pe_af->value() != 0)
	{
		double upper_limit = ui->filter_pe_af->value() + 0.1;
		double lower_limit = ui->filter_pe_af->value() - 0.1;

		for(int row=0; row<row_count; ++row)
		{
			if(!pass[row]) continue;
			double val = alleleFrequency(row, "PR");
			if(val > upper_limit || val < lower_limit) pass[row] = false;
		}
	}

	//Split Reads Allele Frequency Filter
	if(ui->filter_sr_af->value() != 0)
	{
		double upper_limit = ui->filter_sr_af->value() + 0.1;
		double lower_limit = ui->filter_sr_af->value() - 0.1;

		for(int row=0; row<row_count; ++row)
		{
			if(!pass[row]) continue;
			double val = alleleFrequency(row, "SR");
			if(val > upper_limit || val < lower_limit) pass[row] = false;
		}
	}

	//Total number Paired End Reads
	if(ui->filter_pe_reads->value() != 0)
	{
		int pe_reads_thres = ui->filter_pe_reads->value();

		for(int row=0; row<row_count; ++row)
		{
			if(!pass[row]) continue;
			if(pairedEndReadCount(row) < pe_reads_thres) pass[row] = false;
		}
	}

	//filter by ROI
	QString roi = ui->roi->toolTip();
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
			if(!pass[row]) continue;

			if (!sv_bedpe_file_[row].intersectsWith(roi_)) pass[row] = false;
		}
	}

	//filter by phenotype
	if (!phenotypes_.isEmpty())
	{
		for(int row=0; row<row_count; ++row)
		{
			if(!pass[row]) continue;

			if (!sv_bedpe_file_[row].intersectsWith(phenotypes_roi_)) pass[row] = false;
		}
	}

	//hide rows not passing filters
	for(int row=0; row<row_count; ++row)
	{
		ui->svs->setRowHidden(row, !pass[row]);
	}

	//Set number of filtered / total SVs
	ui->number_of_svs->setText(QByteArray::number(pass.count(true)) + "/" + QByteArray::number(row_count));
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

void SvWidget::importHPO()
{
	phenotypes_ = filter_widget_->phenotypes();
	phenotypesChanged();
}

void SvWidget::importROI()
{
	ui->roi->setCurrentText(filter_widget_->targetRegionName());
}

void SvWidget::roiSelectionChanged(int index)
{
	//delete old completer
	QCompleter* completer_old = ui->roi->completer();
	if (completer_old!=nullptr)
	{
		completer_old->deleteLater();
	}

	//create completer for search mode
	if (index==0)
	{
		ui->roi->setEditable(true);

		QCompleter* completer = new QCompleter(ui->roi->model(), ui->roi);
		completer->setCompletionMode(QCompleter::PopupCompletion);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		completer->setFilterMode(Qt::MatchContains);
		completer->setCompletionRole(Qt::DisplayRole);
		ui->roi->setCompleter(completer);
	}
	else
	{
		ui->roi->setEditable(false);
	}


	ui->roi->setToolTip(ui->roi->itemData(index).toString());

	//update
	if(index!=0)
	{
		applyFilters();
	}
}

void SvWidget::editPhenotypes()
{
	//edit
	PhenotypeSelectionWidget* selector = new PhenotypeSelectionWidget(this);
	selector->setPhenotypes(phenotypes_);
	auto dlg = GUIHelper::createDialog(selector, "Select HPO terms", "", true);

	//update
	if (dlg->exec()==QDialog::Accepted)
	{
		phenotypes_ = selector->selectedPhenotypes();
		phenotypesChanged();
	}
}

void SvWidget::phenotypesChanged()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//update GUI
	QByteArrayList tmp;
	foreach(const Phenotype& pheno, phenotypes_)
	{
		tmp << pheno.name();
	}

	ui->hpo->setText(tmp.join("; "));

	QString tooltip = "Phenotype/inheritance filter based on HPO terms.<br><br>Notes:<br>- This functionality is only available when NGSD is enabled.<br>- Filters based on the phenotype-associated gene loci including 5000 flanking bases.";
	if (!phenotypes_.isEmpty())
	{
		tooltip += "<br><br><nobr>Currently selected HPO terms:</nobr>";
		foreach(const Phenotype& pheno, phenotypes_)
		{
			tooltip += "<br><nobr>" + pheno.toString() + "</nobr>";
		}
	}
	ui->hpo->setToolTip(tooltip);

	//get phenotype-based gene list
	NGSD db;
	GeneSet pheno_genes;
	foreach(const Phenotype& pheno, phenotypes_)
	{
		pheno_genes << db.phenotypeToGenes(pheno, true);
	}

	//convert genes to ROI (using a cache to speed up repeating queries)
	phenotypes_roi_.clear();
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
		phenotypes_roi_.add(gene2region_cache_[gene]);
	}
	phenotypes_roi_.merge();

	QApplication::restoreOverrideCursor();

	applyFilters();
}

void SvWidget::SvSelectionChanged()
{
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


void SvWidget::showPhenotypeContextMenu(QPoint pos)
{
	//set up
	QMenu menu;
	QAction* a_clear = menu.addAction("clear");


	//exec
	QAction* action = menu.exec(ui->hpo->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action == a_clear)
	{
		phenotypes_.clear();
		phenotypesChanged();
	}
}
