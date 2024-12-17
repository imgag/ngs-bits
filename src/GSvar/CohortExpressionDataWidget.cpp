#include "CohortExpressionDataWidget.h"
#include "ui_CohortExpressionDataWidget.h"
#include "TsvFile.h"
#include "VersatileFile.h"
#include "Helper.h"
#include "GUIHelper.h"


CohortExpressionDataWidget::CohortExpressionDataWidget(QString tsv_filename, QWidget *parent, QString project_name, QString processing_system_name) :
	QWidget(parent),
	tsv_filename_(tsv_filename),
	ui_(new Ui::CohortExpressionDataWidget)
{
	ui_->setupUi(this);

	//set info fields
	ui_->l_project_name->setText(project_name);
	ui_->l_processing_system_name->setText(processing_system_name);

	loadExpressionData();
}

CohortExpressionDataWidget::~CohortExpressionDataWidget()
{
	delete ui_;
}

void CohortExpressionDataWidget::loadExpressionData()
{
	//load TSV file
	TsvFile cohort_expression_data;
	QSharedPointer<VersatileFile> expression_data_file = Helper::openVersatileFileForReading(tsv_filename_, false);

	//parse TSV file
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
			cohort_expression_data.addComment(line.mid(2));
		}
		else if (line.startsWith("#"))
		{
			foreach (const QString& header, line.mid(1).split('\t'))
			{
				cohort_expression_data.addHeader(header.trimmed());
			}
		}
		else
		{
			cohort_expression_data.addRow(line.split('\t'));
		}
	}

	//set dimensions
	ui_->tw_cohort_data->setRowCount(cohort_expression_data.count());
	ui_->tw_cohort_data->setColumnCount(cohort_expression_data.headers().size());

	// create header
	QStringList tsv_header = cohort_expression_data.headers();
	for (int col_idx = 0; col_idx < tsv_header.size(); ++col_idx)
	{
		ui_->tw_cohort_data->setHorizontalHeaderItem(col_idx, new QTableWidgetItem(tsv_header.at(col_idx)));
	}

	//fill table
	for(int row_idx=0; row_idx<cohort_expression_data.count(); ++row_idx)
	{
		const QStringList& row = cohort_expression_data[row_idx];
		for (int col_idx = 0; col_idx < tsv_header.size(); ++col_idx)
		{
			if(col_idx > 0)
			{
				// add numeric QTableWidgetItem
				double value = Helper::toDouble(row.at(col_idx));
				ui_->tw_cohort_data->setItem(row_idx, col_idx, GUIHelper::createTableItem(value));
			}
			else
			{
				// add standard QTableWidgetItem
				ui_->tw_cohort_data->setItem(row_idx, col_idx, new QTableWidgetItem(row.at(col_idx)));
			}
		}

	}

	//hide vertical header
	ui_->tw_cohort_data->verticalHeader()->setVisible(false);

	//enable sorting
	ui_->tw_cohort_data->setSortingEnabled(true);

	//optimize table view
	GUIHelper::resizeTableCellWidths(ui_->tw_cohort_data, 200);
	GUIHelper::resizeTableCellHeightsToFirst(ui_->tw_cohort_data);

}
