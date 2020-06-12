#include "ExpressionDataWidget.h"
#include "ui_ExpressionDataWidget.h"
#include "TsvFile.h"
#include "Helper.h"
#include "GUIHelper.h"
#include <QFile>


ExpressionDataWidget::ExpressionDataWidget(QString tsv_filename, QWidget *parent) :
	QWidget(parent),
	tsv_filename_(tsv_filename),
	ui(new Ui::ExpressionDataWidget)

{
	ui->setupUi(this);
	loadExpressionData();
}

ExpressionDataWidget::~ExpressionDataWidget()
{
	delete ui;
}

void ExpressionDataWidget::loadExpressionData()
{
	// load TSV file
	TsvFile expression_data;
	QSharedPointer<QFile> expression_data_file = Helper::openFileForReading(tsv_filename_, false);

	while (!expression_data_file->atEnd())
	{
		QString line = expression_data_file->readLine().trimmed();
		if (line == "")
		{
			// skip empty lines
			continue;
		}
		else if	(line.startsWith("##"))
		{
			expression_data.addComment(line.mid(2));
		}
		else if (line.startsWith("#"))
		{
			foreach (const QString& header, line.mid(1).split('\t'))
			{
				expression_data.addHeader(header.trimmed());
			}
		}
		else
		{
			expression_data.addRow(line.split('\t'));
		}
	}

	// create table

	// set columns
	ui->expressionData->setColumnCount(4);
	ui->expressionData->setHorizontalHeaderItem(0, new QTableWidgetItem(QString("raw")));
	ui->expressionData->setHorizontalHeaderItem(1, new QTableWidgetItem(QString("cpm")));
	ui->expressionData->setHorizontalHeaderItem(2, new QTableWidgetItem(QString("tpm")));
	ui->expressionData->setHorizontalHeaderItem(3, new QTableWidgetItem(QString("gene_name")));



	//Fill rows
	ui->expressionData->setRowCount(expression_data.rowCount());

	//Fill table widget with expression data
	for(int row_idx=0; row_idx<expression_data.rowCount(); ++row_idx)
	{

		//TODO: make more robust (check header line)
		QStringList row = expression_data.row(row_idx);
		ui->expressionData->setItem(row_idx,0,new QTableWidgetItem(row.at(1)));
		ui->expressionData->setItem(row_idx,1,new QTableWidgetItem(row.at(2)));
		ui->expressionData->setItem(row_idx,2,new QTableWidgetItem(row.at(4)));
		ui->expressionData->setItem(row_idx,3,new QTableWidgetItem(row.at(5)));
	}



}
