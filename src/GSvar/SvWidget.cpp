#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QClipboard>
#include <QBitArray>
#include "SvWidget.h"
#include "ui_SvWidget.h"
#include "Helper.h"
#include "GUIHelper.h"

SvWidget::SvWidget(QString file_name, QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::SvWidget)
{
	ui->setupUi(this);

	//Setup signals and slots
	connect(ui->copy_to_clipboard,SIGNAL(clicked()),this,SLOT(copyToClipboard()));

	connect(ui->filter_type,SIGNAL(currentIndexChanged(int)),this,SLOT(filtersChanged()));
	connect(ui->qual_filter,SIGNAL(currentIndexChanged(int)),this,SLOT(filtersChanged()));

	connect(ui->filter_size,SIGNAL(valueChanged(double)),this,SLOT(filtersChanged()));

	connect(ui->filter_tumor_frequency_paired_reads,SIGNAL(valueChanged(double)),this,SLOT(filtersChanged()));
	connect(ui->filter_tumor_frequency_single_reads,SIGNAL(valueChanged(double)),this,SLOT(filtersChanged()));
	connect(ui->filter_tumor_depth_paired_reads,SIGNAL(valueChanged(int)),this,SLOT(filtersChanged()));
	connect(ui->filter_tumor_depth_single_reads,SIGNAL(valueChanged(int)),this,SLOT(filtersChanged()));


	connect(ui->filter_normal_frequency_paired_reads,SIGNAL(valueChanged(double)),this,SLOT(filtersChanged()));
	connect(ui->filter_normal_frequency_single_reads,SIGNAL(valueChanged(double)),this,SLOT(filtersChanged()));

	connect(ui->filter_normal_depth_paired_reads,SIGNAL(valueChanged(int)),this,SLOT(filtersChanged()));
	connect(ui->filter_normal_depth_single_reads,SIGNAL(valueChanged(int)),this,SLOT(filtersChanged()));

	connect(ui->svs,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(SvDoubleClicked(QTableWidgetItem*)));

	QString path = QFileInfo(file_name).absolutePath();
	QStringList sv_files = Helper::findFiles(path, "*_var_structural.tsv", false);

	if(sv_files.isEmpty())
	{
		THROW(FileAccessException, "Could not find file with structural variants in " + path);
	}

	ui->filter_tumor_frequency_paired_reads->setVisible(false);
	ui->label_tumor_frequency_paired_reads->setVisible(false);

	ui->filter_tumor_frequency_single_reads->setVisible(false);
	ui->label_tumor_frequency_single_reads->setVisible(false);

	ui->filter_tumor_depth_paired_reads->setVisible(false);
	ui->label_tumor_depth_paired_reads->setVisible(false);

	ui->filter_tumor_depth_single_reads->setVisible(false);
	ui->label_tumor_depth_single_reads->setVisible(false);

	ui->filter_normal_frequency_paired_reads->setVisible(false);
	ui->label_normal_frequency_paired_reads->setVisible(false);

	ui->filter_normal_frequency_single_reads->setVisible(false);
	ui->label_normal_frequency_single_reads->setVisible(false);

	ui->filter_normal_depth_paired_reads->setVisible(false);
	ui->label_normal_depth_paired_reads->setVisible(false);

	ui->filter_normal_depth_single_reads->setVisible(false);
	ui->label_normal_depth_single_reads->setVisible(false);

	loadSVs(sv_files[0]);


	ui->svs->setSelectionBehavior(QAbstractItemView::SelectRows);

	//filter rows according default filter values
	QMetaObject::invokeMethod(this,"filtersChanged");


	ui->svs->setSortingEnabled(true);
}

void SvWidget::addInfoLine(QString text)
{
	ui->info_box->layout()->addWidget(new QLabel(text));
}

void SvWidget::filtersChanged()
{
	int row_count = svs_.count();

	QBitArray pass;
	pass.fill(true,row_count);


	//"type" filter
	if(ui->filter_type->currentText() != "n/a")
	{
		QByteArray chosen_type = ui->filter_type->currentText().toLatin1();

		for(int row=0;row<row_count;row++)
		{
			pass[row] = (svs_[row].type() == chosen_type);
		}
	}

	//"size" filter
	double size_filter = ui->filter_size->value() * 1000;
	for(int row=0;row<row_count;row++)
	{
		if(!pass[row]) continue;

		if(svs_[row].size() < 0) continue;
		pass[row] = (svs_[row].size() >= size_filter);
	}


	//qual filter - filtering
	if(ui->qual_filter->currentText() != "n/a")
	{
		QByteArray chosen_qual_filter = ui->qual_filter->currentText().toLatin1();
		for(int row=0;row<row_count;row++)
		{
			//skip entries that are already hidden
			if(!pass[row]) continue;

			pass[row] = (svs_[row].getFilter() == chosen_qual_filter);
		}
	}


	//filtering of annotation columns

	//tumor paired reads frequency filter
	if(ui->filter_tumor_frequency_paired_reads->isVisible())
	{
		filterAnnotationsForNumber("tumor_PR_freq",ui->filter_tumor_frequency_paired_reads->value(),pass);
	}

	//tumor single reads frequency filter
	if(ui->filter_tumor_frequency_single_reads->isVisible())
	{
		filterAnnotationsForNumber("tumor_SR_freq",ui->filter_tumor_frequency_single_reads->value(),pass);
	}

	//tumor paired reads depth filter
	if(ui->filter_tumor_depth_paired_reads->isVisible())
	{
		filterAnnotationsForNumber("tumor_PR_depth",ui->filter_tumor_depth_paired_reads->value(),pass);
	}

	//tumor single reads depth filter
	if(ui->filter_tumor_depth_single_reads->isVisible())
	{
		filterAnnotationsForNumber("tumor_SR_depth",ui->filter_tumor_depth_single_reads->value(),pass);
	}

	//normal paired read frequency
	if(ui->filter_normal_frequency_paired_reads->isVisible())
	{
		filterAnnotationsForNumber("normal_PR_freq",ui->filter_normal_frequency_paired_reads->value(),pass);
	}

	//normal single read frequency
	if(ui->filter_normal_frequency_single_reads->isVisible())
	{
		filterAnnotationsForNumber("normal_SR_freq",ui->filter_normal_frequency_single_reads->value(),pass);
	}

	//normal paired read depth
	if(ui->filter_normal_depth_paired_reads->isVisible())
	{
		filterAnnotationsForNumber("normal_PR_depth",ui->filter_normal_depth_paired_reads->value(),pass);
	}

	//normal single read depth
	if(ui->filter_normal_depth_single_reads->isVisible())
	{
		filterAnnotationsForNumber("normal_SR_depth",ui->filter_normal_depth_single_reads->value(),pass);
	}

	for(int row=0;row<row_count;row++)
	{
		ui->svs->setRowHidden(row,!pass[row]);
	}
}

void SvWidget::filterAnnotationsForNumber(QByteArray anno_name, double filter_thresh, QBitArray &pass)
{
	if(pass.count() != svs_.count()) return;

	int i_anno = svs_.annotationIndexByName(anno_name,false);
	if(i_anno == -1) return;

	for(int row=0;row<pass.count();++row)
	{
		bool entry_is_number;
		double entry_as_number = svs_[row].annotations().at(i_anno).toDouble(&entry_is_number);

		//some entries are non-numeric: ignore
		if(!entry_is_number) continue;
		//if entry already skipped: Do not change entry
		if(!pass[row]) continue;

		pass[row] = entry_as_number >=filter_thresh;
	}
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


void SvWidget::loadSVs(QString file_name)
{
	svs_.load(file_name);

	if(svs_.count() == 0)
	{
		THROW(FileParseException,"Warning: File for structural variants does not contain any data");
	}

	//print comments into SV calling info_box
	foreach(QString comment, svs_.comments())
	{
		addInfoLine(comment);
	}

	QList<int> annotation_indices;

	for(int i=0; i<svs_.annotationHeaders().count(); ++i)
	{
		if(svs_.annotationHeaders()[i] == "length") continue;

		QByteArray header = svs_.annotationHeaders()[i];
		ui->svs->setColumnCount(ui->svs->columnCount() + 1);
		ui->svs->setHorizontalHeaderItem(ui->svs->columnCount() -1, new QTableWidgetItem(QString(header)));

		annotation_indices.append(i);
	}
	ui->svs->setRowCount(svs_.count());

	for(int row=0;row<svs_.count();row++)
	{
		ui->svs->setItem(row,0,new QTableWidgetItem(QString::fromUtf8(svs_[row].chr().str())) );
		ui->svs->setItem(row,1,new QTableWidgetItem(QString::number(svs_[row].start())) );

		if(svs_[row].end() == -1)
		{
			ui->svs->setItem(row,2,new QTableWidgetItem(QString('.')));
		}
		else
		{
			ui->svs->setItem(row,2,new QTableWidgetItem(QString::number(svs_[row].end())) );
		}

		ui->svs->setItem(row,3,new QTableWidgetItem(QString::fromUtf8(svs_[row].type())) );

		if(svs_[row].size() == -1)
		{
			ui->svs->setItem(row,4,new QTableWidgetItem(QString('.')));
		}
		else
		{
			//size in kilo bases
			ui->svs->setItem(row,4,new QTableWidgetItem(QString::number(svs_[row].size()/1000.)) );
		}

		ui->svs->setItem(row,5,new QTableWidgetItem(QString::number(svs_[row].score())) );


		ui->svs->setItem(row,6,new QTableWidgetItem(QString::fromUtf8(svs_[row].getFilter())) );

		ui->svs->setItem(row,7,new QTableWidgetItem(QString::fromUtf8(svs_[row].mateChr().str())) );
		if(svs_[row].matePos() == -1)
		{
			ui->svs->setItem(row,8,new QTableWidgetItem(QString('.')) );
		}
		else
		{
			ui->svs->setItem(row,8,new QTableWidgetItem(QString::number(svs_[row].matePos())) );
		}
		ui->svs->setItem(row,9,new QTableWidgetItem(QString::fromUtf8(svs_[row].getMateFilter())) );

		//column number for annotations
		int c = 10;

		foreach(int index, annotation_indices)
		{
			bool is_number;
			svs_[row].annotations()[index].toFloat(&is_number);
			QTableWidgetItem* item;

			if(!is_number)
			{
				item = new QTableWidgetItem(QString(svs_[row].annotations()[index]));
			}
			else //in case annotation is number: round to 3 digits
			{
				item = new QTableWidgetItem(QString::number(svs_[row].annotations()[index].toFloat(),'f',3 ));
			}

			ui->svs->setItem(row, c++, item);
		}
	}

	//check annotation header whether columns for allele frequencies and sequencing depths exist, remove certain buttons if not
	if(svs_.annotationIndexByName("tumor_PR_freq",false) >= 0)
	{
		ui->filter_tumor_frequency_paired_reads->setVisible(true);
		ui->label_tumor_frequency_paired_reads->setVisible(true);
	}
	if(svs_.annotationIndexByName("tumor_SR_freq",false) >= 0)
	{
		ui->filter_tumor_frequency_single_reads->setVisible(true);
		ui->label_tumor_frequency_single_reads->setVisible(true);
	}
	if(svs_.annotationIndexByName("tumor_PR_depth",false) >= 0)
	{
		ui->filter_tumor_depth_paired_reads->setVisible(true);
		ui->label_tumor_depth_paired_reads->setVisible(true);
	}
	if(svs_.annotationIndexByName("tumor_SR_depth",false) >= 0)
	{
		ui->filter_tumor_depth_single_reads->setVisible(true);
		ui->label_tumor_depth_single_reads->setVisible(true);
	}
	if(svs_.annotationIndexByName("normal_PR_freq",false) >= 0)
	{
		ui->filter_normal_frequency_paired_reads->setVisible(true);
		ui->label_normal_frequency_paired_reads->setVisible(true);
	}
	if(svs_.annotationIndexByName("normal_SR_freq",false) >= 0)
	{
		ui->filter_normal_frequency_single_reads->setVisible(true);
		ui->label_normal_frequency_single_reads->setVisible(true);
	}
	if(svs_.annotationIndexByName("normal_PR_depth",false) >= 0)
	{
		ui->filter_normal_depth_paired_reads->setVisible(true);
		ui->label_normal_depth_paired_reads->setVisible(true);
	}
	if(svs_.annotationIndexByName("normal_SR_depth",false) >= 0)
	{
		ui->filter_normal_depth_single_reads->setVisible(true);
		ui->label_normal_depth_single_reads->setVisible(true);
	}
}

