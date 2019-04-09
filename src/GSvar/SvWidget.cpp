#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QClipboard>
#include <QBitArray>
#include <QByteArray>
#include <QMenu>
#include <QAction>
#include "SvWidget.h"
#include "ui_SvWidget.h"
#include "Helper.h"
#include "GUIHelper.h"

SvWidget::SvWidget(const QStringList& bedpe_file_paths, QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::SvWidget)
{
	ui->setupUi(this);
	ui->svs->setContextMenuPolicy(Qt::CustomContextMenu);

	//Setup signals and slots
	connect(ui->copy_to_clipboard,SIGNAL(clicked()),this,SLOT(copyToClipboard()));

	connect(ui->filter_type,SIGNAL(currentIndexChanged(int)),this,SLOT(filtersChanged()));
	connect(ui->filter_qual,SIGNAL(currentIndexChanged(int)),this,SLOT(filtersChanged()));
	connect(ui->filter_text,SIGNAL(textEdited(QString)),this,SLOT(filtersChanged()));
	connect(ui->filter_text_search_type,SIGNAL(currentTextChanged(QString)),this,SLOT(filtersChanged()));
	connect(ui->filter_quality_score,SIGNAL(valueChanged(int)),this,SLOT(filtersChanged()));
	connect(ui->ignore_special_chromosomes,SIGNAL(stateChanged(int)),this,SLOT(filtersChanged()));
	connect(ui->filter_pe_af,SIGNAL(valueChanged(double)),this,SLOT(filtersChanged()));
	connect(ui->filter_sr_af,SIGNAL(valueChanged(double)),this,SLOT(filtersChanged()));

	connect(ui->delly_mapq,SIGNAL(valueChanged(int)),this,SLOT(filtersChanged()));
	connect(ui->filter_pe_reads,SIGNAL(valueChanged(int)),this,SLOT(filtersChanged()));

	connect(ui->svs,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(SvDoubleClicked(QTableWidgetItem*)));

	connect(ui->svs,SIGNAL(itemSelectionChanged()),this,SLOT(SvSelectionChanged()));

	connect(ui->svs,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));

	connect(ui->file_names,SIGNAL(currentTextChanged(QString)),this,SLOT(fileNameChanged()));


	if(bedpe_file_paths.count() < 1)
	{
		disableGui("There are no SV files in sample folder.");
	}
	else
	{
		foreach(QString path, bedpe_file_paths)
		{
			ui->file_names->addItem(QFileInfo(path).fileName(),QFileInfo(path).absoluteFilePath());
		}

		//Load first bedpe file found in sample dir
		loadSVs(bedpe_file_paths[0]);
	}
}

void SvWidget::loadSVs(const QString &file_name)
{
	sv_bedpe_file_.clear();
	clearGUI();

	sv_bedpe_file_.load(file_name);

	if(sv_bedpe_file_.count() == 0)
	{
		disableGui("There are no SVs in data file ");
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

	//Set entries for combobox used for filtering SV type
	QStringList types;
	int i_type =  sv_bedpe_file_.annotationIndexByName("TYPE",false);
	if(i_type > -1)
	{
		types << "n/a";
		for(int row=0;row<sv_bedpe_file_.count();++row)
		{
			if(!types.contains(sv_bedpe_file_[row].annotations().at(i_type)))
			{
				types << sv_bedpe_file_[row].annotations().at(i_type);
			}
		}
		foreach(QString type,types)
		{
			ui->filter_type->addItem(type);
		}
	}
	else
	{
		ui->filter_type->setEnabled(false);
	}

	//set entries for combobox used for filtering SV quality
	QList<QByteArray> entries = sv_bedpe_file_.annotationDescriptionByID("FILTER").keys();

	if(!entries.contains("PASS")) entries.prepend("PASS");
	entries.prepend("n/a");

	foreach(QByteArray entry,entries) ui->filter_qual->addItem(QString(entry));


	//Enable/Disable caller specific filters
	if(sv_bedpe_file_.analysisType() == BedpeFile::SV_TYPE::DELLY)
	{
		ui->delly_mapq->setEnabled(true);
	}
	else
	{
		ui->delly_mapq->setEnabled(false);
	}

	//use different standard threshold for total count of paired end reads
	if(sv_bedpe_file_.analysisType() == BedpeFile::SV_TYPE::MANTA)
	{
		ui->filter_pe_reads->setValue(0);
	}


	//only whole rows can be selected, one row at a time
	ui->svs->setSelectionBehavior(QAbstractItemView::SelectRows);
	resizeQTableWidget(ui->svs);


	//filter rows according default filter values
	filtersChanged();
}


void SvWidget::clearGUI()
{
	ui->info_a->setRowCount(0);
	ui->info_b->setRowCount(0);
	ui->sv_details->setRowCount(0);

	//Clear table widget to cols / rows specified in .ui file
	ui->svs->setRowCount(0);
	ui->svs->setColumnCount(6);

	ui->filter_type->clear();
	ui->filter_qual->clear();
	ui->filter_quality_score->setValue(0);

	ui->filter_pe_af->setValue(0);
	ui->filter_sr_af->setValue(0);

	ui->label_pe_af->clear();
	ui->label_sr_af->clear();

	ui->ignore_special_chromosomes->setCheckState(Qt::CheckState::Checked);

	ui->filter_text->clear();
	ui->filter_text_search_type->setCurrentIndex(0);
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

void SvWidget::fileNameChanged()
{
	clearGUI();
	loadSVs(ui->file_names->currentData(Qt::UserRole).toString());
}

void SvWidget::addInfoLine(QString text)
{
	ui->info_box->layout()->addWidget(new QLabel(text));
}

void SvWidget::filtersChanged()
{
	int row_count = ui->svs->rowCount();
	QBitArray pass;
	pass.fill(true,row_count);

	//Free text filter
	if(!ui->filter_text->text().isEmpty() && (ui->filter_text_search_type->currentText() == "contains" || ui->filter_text_search_type->currentText() == "contains not"))
	{
		for(int row=0;row<row_count;++row)
		{
			if(!pass[row]) continue;

			bool in_any_item = false;

			for(int c=0;c<ui->svs->columnCount();++c)
			{
				QString item =  ui->svs->item(row,c)->text();
				if(item.contains(ui->filter_text->text(),Qt::CaseInsensitive))
				{
					in_any_item = true;
					break;
				}
			}
			if(ui->filter_text_search_type->currentText() == "contains" ) pass[row] = in_any_item;
			else if(ui->filter_text_search_type->currentText() == "contains not")  pass[row] = !in_any_item;
		}
	}

	//Skip special chromosomes
	if(ui->ignore_special_chromosomes->isChecked())
	{
		QByteArrayList special_chrs = {"GL","HS"};
		for(int row=0;row<row_count;++row)
		{
			if(!pass[row]) continue;

			foreach(QByteArray special_chr,special_chrs)
			{
				if(ui->svs->item(row,0)->text().contains(special_chr,Qt::CaseInsensitive) || ui->svs->item(row,3)->text().contains(special_chr,Qt::CaseInsensitive))
				{
					pass[row] = false;
					break;
				}
			}

		}
	}


	//SV type (e.g. DUP,DEL or INV)
	if(ui->filter_type->currentText() != "n/a")
	{
		int i_type = colIndexbyName("TYPE");

		if(i_type > -1)
		{
			for(int row=0;row<row_count;++row)
			{
				if(!pass[row]) continue;
				if(ui->svs->item(row,i_type)->text() != ui->filter_type->currentText()) pass[row] = false;
			}
		}
	}

	//SV quality (e.g. PASS, LowFreq etc)
	if(ui->filter_qual->currentText() != "n/a")
	{
		int i_qual_filter = colIndexbyName("FILTER");
		if(i_qual_filter > -1)
		{
			for(int row=0;row<row_count;++row)
			{
				if(!pass[row]) continue;
				if(!ui->svs->item(row,i_qual_filter)->text().contains(ui->filter_qual->currentText())) pass[row] = false;
			}
		}
	}

	//SV quality score
	if(ui->filter_quality_score->value() != 0)
	{
		int i_qual_score = colIndexbyName("QUAL");
		if(i_qual_score != -1)
		{
			for(int row=0;row<row_count;++row)
			{
				if(!pass[row]) continue;

				bool is_valid_number = false;
				double val = ui->svs->item(row,i_qual_score)->text().toDouble(&is_valid_number);
				if(!is_valid_number) continue;

				//skip values smaller than threshold
				if(val < (double) ui->filter_quality_score->value()) pass[row] = false;

			}
		}
	}

	//Paired End Reads Allele Frequency
	if(ui->filter_pe_af->value() != 0)
	{
		double upper_limit = ui->filter_pe_af->value() + 0.1;
		double lower_limit = ui->filter_pe_af->value() - 0.1;

		for(int row=0;row<row_count;++row)
		{
			if(!pass[row]) continue;
			double val = alleleFrequency(row,"PR");
			if(val > upper_limit || val < lower_limit) pass[row] = false;
		}
	}

	//Split Reads Allele Frequency Filter
	if(ui->filter_sr_af->value() != 0)
	{
		double upper_limit = ui->filter_sr_af->value() + 0.1;
		double lower_limit = ui->filter_sr_af->value() - 0.1;

		for(int row=0;row<row_count;++row)
		{
			if(!pass[row]) continue;
			double val = alleleFrequency(row,"SR");
			if(val > upper_limit || val < lower_limit) pass[row] = false;
		}
	}

	//Total number Paired End Reads
	if(ui->filter_pe_reads->value() != 0)
	{
		int pe_reads_thres = ui->filter_pe_reads->value();

		for(int row=0;row<row_count;++row)
		{
			if(!pass[row]) continue;
			if(pairedEndReadCount(row) < pe_reads_thres) pass[row] = false;
		}
	}


	/******************************
	 * SV CALLER SPECIFIC FILTERS *
	 ******************************/
	//Delly Mapping Quality
	if(ui->delly_mapq->isEnabled() && ui->delly_mapq->value() > 0)
	{
		int mapq_thres = ui->delly_mapq->value();

		//Mapping quality values are in annotations
		int i_info_a = colIndexbyName("INFO_A");

		for(int row=0;row<row_count;++row)
		{
			if(!pass[row]) continue;

			//Determine Mapping Quality from INFO_A column
			QStringList infos = ui->svs->item(row,i_info_a)->text().split(";");
			int mapq_val = -1;
			foreach(QString info,infos)
			{
				if(info.contains("MAPQ=",Qt::CaseInsensitive))
				{
					bool success = false;
					mapq_val = info.replace("MAPQ=","").toInt(&success);
					if(!success) mapq_val = -1;

					break;
				}
			}

			if(mapq_val < mapq_thres && mapq_val > -1) pass[row] = false;
		}
	}


	for(int row=0;row<row_count;row++)
	{
		ui->svs->setRowHidden(row,!pass[row]);
	}

	//Set number of filtered / total SVs
	int number_visible_rows = 0;
	for(int row=0;row<row_count;++row)
	{
		if(!ui->svs->isRowHidden(row)) ++number_visible_rows;
	}

	ui->number_of_svs->setText(QByteArray::number(number_visible_rows) + "/" + QByteArray::number(row_count));
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
	if(sv_bedpe_file_.analysisType() == BedpeFile::SV_TYPE::MANTA)
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

	if(sv_bedpe_file_.analysisType() == BedpeFile::SV_TYPE::DELLY)
	{
		int i_info_a = colIndexbyName("INFO_A");
		if(i_info_a == -1 ) return -1.;

		QStringList infos = ui->svs->item(row,i_info_a)->text().split(";");

		int pe_read_val = -1;
		foreach(QString info,infos)
		{
			if(info.contains("PE=",Qt::CaseInsensitive))
			{
				bool success = false;
				pe_read_val = info.replace("PE=","").toInt(&success);
				if(!success) pe_read_val = -1;
				break;
			}
		}

		return pe_read_val;
	}

	return -1.;
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


	if(sv_bedpe_file_.analysisType() == BedpeFile::SV_TYPE::MANTA)
	{
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
	}

	if(sv_bedpe_file_.analysisType() == BedpeFile::SV_TYPE::DELLY)
	{
		bool success = false;

		if(read_type ==  "PR") count_ref = getFormatEntryByKey("DR",desc,data).toInt(&success);
		else if(read_type == "SR") count_ref = getFormatEntryByKey("RR",desc,data).toInt(&success);
		else return -1.;

		if(!success) return -1.;
		if(read_type == "PR") count_alt = getFormatEntryByKey("DV",desc,data).toInt(&success);
		else if(read_type == "SR") count_alt = getFormatEntryByKey("RV",desc,data).toInt(&success);
		if(!success) return -1.;
	}

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

	QString coords = ui->svs->item(row,0)->text() + ":" + ui->svs->item(row,1)->text() + "-" + ui->svs->item(row,2)->text();
	coords.replace("-.","-" + QString::number(ui->svs->item(row,1)->text().toInt()+1));

	emit openSvInIGV(coords);
}

void SvWidget::disableGui(const QString& message)
{
	ui->filter_type->setEnabled(false);

	ui->filter_qual->setEnabled(false);
	ui->copy_to_clipboard->setEnabled(false);
	ui->svs->setEnabled(false);

	addInfoLine("<font color='red'>" + message + "</font>");
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

	//Display Split Read AF of variant
	ui->label_sr_af->setText("Split Read AF: " + QString::number(alleleFrequency(row,"SR"),'f',2));

	//Display Paired End Read AF of variant
	ui->label_pe_af->setText("Paired End Read AF: " + QString::number(alleleFrequency(row,"PR"),'f',2));
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

	int index = rows.at(0).row();

	QMenu menu(ui->svs);

	menu.addAction("open in IGV");
	menu.addAction("open mate in IGV");
	menu.addAction("open split screen in IGV");

	QAction* action = menu.exec(ui->svs->viewport()->mapToGlobal(pos));
	if(action == nullptr) return;

	QString chr = ui->svs->item(index,0)->text();
	QString start = ui->svs->item(index,1)->text();
	QString end = ui->svs->item(index,2)->text();
	if(action->text() == "open in IGV")
	{
		QString coords = chr + ":" + start + "-" + end;
		emit(openSvInIGV(coords));
	}

	QString chr2 = ui->svs->item(index,3)->text();
	QString start2 = ui->svs->item(index,4)->text();
	QString end2 = ui->svs->item(index,5)->text();
	if(action->text() == "open mate in IGV")
	{
		QString coords = chr2 + ":" + start2 + "-" + end2;
		emit(openSvInIGV(coords));
	}

	if(action->text() == "open split screen in IGV")
	{
		QString coords = chr + ":" + start + "-" + end + " " + chr2 + ":" + start2 + "-" + end2;
		emit(openSvInIGV(coords));
	}
}

