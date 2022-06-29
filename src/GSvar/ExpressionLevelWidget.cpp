#include "ExpressionLevelWidget.h"
#include "RepeatExpansionWidget.h"
#include "ui_ExpressionLevelWidget.h"

#include "LoginManager.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include <QMessageBox>

struct ExpressioData
{
	double tpm_mean;
	double tpm_stdev;
	double tpm_01_perc;
	double tpm_1_perc;
};

ExpressionLevelWidget::ExpressionLevelWidget(const GeneSet& genes, QWidget *parent) :
	genes_(genes),
	QWidget(parent),
	ui_(new Ui::ExpressionLevelWidget)
{
	// skipp if no NGSD is available
	if (!LoginManager::active())
	{
		QMessageBox::warning(this, "Expression Level Widget", "Widget requires NGSD access!");
		return;
	}

	ui_->setupUi(this);

	loadData();
}

ExpressionLevelWidget::~ExpressionLevelWidget()
{
	delete ui_;
}

void ExpressionLevelWidget::loadData()
{
	try
	{
		// debug
		QTime timer;
		timer.start();

		QApplication::setOverrideCursor(Qt::BusyCursor);
		NGSD db;

		//get processing systems
		QSet<int> sys_ids = db.getValuesInt("SELECT `id` FROM `processing_system` WHERE `type`='RNA'").toSet();
		QStringList tissues = db.getEnum("sample", "tissue");

		foreach (int id, sys_ids)
		{
			qDebug() << db.getProcessingSystemData(id).name_short << db.getProcessingSystemData(id).name;
		}


		//get processed sample IDs for each processing system and tissue
		QMap<QPair<QPair<int,QString>, QString>, ExpressioData> expression;
		QList<QPair<int, QString>> valid_processing_systems;
		foreach (int sys_id, sys_ids)
		{
			foreach (const QString& tissue, tissues)
			{
				QSet<int> current_cohort = db.getRNACohort(sys_id, tissue);

				// skip columns with no samples
				if(current_cohort.size() == 0) continue;

				valid_processing_systems << QPair<int, QString>(sys_id, tissue);

				foreach (const QByteArray& gene, genes_)
				{
					//get expression data for each gene
					QVector<double> tpm_values = db.getExpressionValues(gene, current_cohort);
					// QVector<double> tpm_values2 = db.getExpressionValues(gene, sys_id, tissue, true);
					ExpressioData data;
					if (tpm_values.size() > 0)
					{
						data.tpm_mean = BasicStatistics::mean(tpm_values);
						data.tpm_stdev = BasicStatistics::stdev(tpm_values, data.tpm_mean);
					}
					else
					{
						data.tpm_mean = -1;
						data.tpm_stdev = -1;
					}


					int n_tpm01=0, n_tpm1=0;
					foreach (double tpm, tpm_values)
					{
						if(tpm >= 0.1)
						{
							n_tpm01++;
							if(tpm >= 1.0)
							{
								n_tpm1++;
							}
						}
					}

					data.tpm_01_perc = (double) n_tpm01 / tpm_values.size();
					data.tpm_1_perc = (double) n_tpm1 / tpm_values.size();

					expression.insert(QPair<QPair<int,QString>, QString>(QPair<int, QString>(sys_id, tissue), gene), data);
				}
			}
		}


		// init table
		ui_->tw_expression->setSortingEnabled(false);
		ui_->tw_expression->setColumnCount(1 + (valid_processing_systems.size() * 4));
		ui_->tw_expression->setRowCount(genes_.count());

		int col_idx = 0;

		ui_->tw_expression->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("\n\ngene"));
		//iterate over all processing systems with data
		foreach (auto column, valid_processing_systems)
		{
			int sys_id = column.first;
			const QString& tissue = column.second;
			ui_->tw_expression->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem(db.getProcessingSystemData(sys_id).name + "\nTissue: " + tissue + "\nmean"));
			ui_->tw_expression->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("\n\nstdev"));
			ui_->tw_expression->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("\n\n(tpm > 0.1) %"));
			ui_->tw_expression->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("\n\n(tpm > 1) %"));

		}

		// fill table
		int row_idx = 0;
		foreach (const QString& gene, genes_)
		{
			col_idx = 0;
			ui_->tw_expression->setItem(row_idx, col_idx++, new QTableWidgetItem(gene));

			foreach (auto column, valid_processing_systems)
			{
				const ExpressioData& data = expression.value(QPair<QPair<int,QString>, QString>(column, gene));
				ui_->tw_expression->setItem(row_idx, col_idx++, new NumericWidgetItem(QString::number(data.tpm_mean, 'f', 3)));
				ui_->tw_expression->setItem(row_idx, col_idx++, new NumericWidgetItem(QString::number(data.tpm_stdev, 'f', 3)));
				ui_->tw_expression->setItem(row_idx, col_idx++, new NumericWidgetItem(QString::number(data.tpm_01_perc * 100, 'f', 1)));
				ui_->tw_expression->setItem(row_idx, col_idx++, new NumericWidgetItem(QString::number(data.tpm_1_perc * 100, 'f', 1)));
			}
			row_idx++;
			qDebug() << "row: " << row_idx;
		}

		//hide vertical header
		ui_->tw_expression->verticalHeader()->setVisible(false);

		//enable sorting
		ui_->tw_expression->setSortingEnabled(true);

		GUIHelper::resizeTableCells(ui_->tw_expression);

		QApplication::restoreOverrideCursor();

		qDebug() << "Get processing system expression level: " << Helper::elapsedTime(timer);

	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error opening RNA expression file.");
	}
}
