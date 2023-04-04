#ifndef EXPRESSIONGENEWIDGET_H
#define EXPRESSIONGENEWIDGET_H

#include "NGSD.h"
#include <GeneSet.h>
#include <QTableWidget>
#include <QWidget>
#include <TsvFile.h>

namespace Ui {
class ExpressionGeneWidget;
}

struct DBExpressionValues
{
	double cohort_mean;
	double log2fc;
	double zscore;
	double pvalue;
};

class ExpressionGeneWidget : public QWidget
{
	Q_OBJECT

public:
	ExpressionGeneWidget(QString tsv_filename, int sys_id, QString tissue, const QString& variant_gene_filter = "", const GeneSet& variant_gene_set = GeneSet(), const QString& project = "",
						 const QString& ps_id = "", RnaCohortDeterminationStategy cohort_type_=RNA_COHORT_GERMLINE, QWidget *parent = 0);
	~ExpressionGeneWidget();

private slots:
	void applyFilters(int max_rows=5000);
	void updateTable(int max_rows=5000);
	void copyToClipboard();
	void showBiotypeContextMenu(QPoint pos);
	void selectAllBiotypes(bool deselect=false);
	void showHistogram(int row_idx);
	void showExpressionTableContextMenu(QPoint pos);
	void showCohort();
	void copyCohortToClipboard();
	void OpenInIGV(QTableWidgetItem* item);
	void showCustomCohortDialog();
	void toggleUICustomCohort();
	void toggleCohortStats(bool enable = false);

private:
	void updateCohort();
	void loadExpressionData();
	void initTable();
	void updateQuery();
	void initBiotypeList();
	bool getGeneStats(const QByteArray& gene, double tpm);
	QStringList getQualityFilter();
	//file
	QString tsv_filename_;
	TsvFile expression_data_;

	//info
	int sys_id_;
	QString tissue_;
	GeneSet variant_gene_set_;
	QString project_;
	QString ps_id_;
	RnaCohortDeterminationStategy cohort_type_;
	QSet<int> custom_cohort_;
	QStringList exclude_quality_;

	//db
	NGSD db_;
	QMap<QByteArray, QByteArray> ensg_mapping_;
	QMap<int, QByteArray> id2gene_;
	QMap<QByteArray, int> gene2id_;
	QMap<QByteArray, int> gene_id_mapping_;
	SqlQuery query_gene_stats_ = db_.getQuery();
	SqlQuery query_gene_stats_old_ = db_.getQuery();
	QMap<QByteArray, DBExpressionValues> ngsd_expression;
	QSet<int> cohort_;

	//gui
	QTableWidget* cohort_table_ = nullptr;
	Ui::ExpressionGeneWidget *ui_;

	//table info
	QStringList column_names_;
	QStringList db_column_names_;
	QVector<bool> numeric_columns_;
	QVector<int> precision_;
	FilterResult filter_result_;

	//status
	bool filtering_in_progress_ = false;

	static QVector<double> calculateRanks(const QVector<double>& values);

};

#endif // EXPRESSIONGENEWIDGET_H
