#ifndef EXPRESSIONEXONWIDGET_H
#define EXPRESSIONEXONWIDGET_H

#include <FilterCascade.h>
#include <QTableWidget>
#include <QWidget>

#include "TsvFile.h"
#include "GeneSet.h"
#include "NGSD.h"

namespace Ui {
class ExpressionExonWidget;
}

struct DBExpressionValues
{
	double cohort_mean;
	double log2fc;
	double zscore;
	double pvalue;
};

class ExpressionExonWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ExpressionExonWidget(QString tsv_filename, int sys_id, QString tissue, const QString& variant_gene_filter = "", const GeneSet& variant_gene_set = GeneSet(), const QString& project = "",
								  const QString& ps_id = "", RnaCohortDeterminationStategy cohort_type=RNA_COHORT_GERMLINE, QWidget *parent = 0);
	~ExpressionExonWidget();

private:
	// parameter
	QString tsv_filename_;
	int sys_id_;
	QString tissue_;
	QString variant_gene_filter_;
	GeneSet variant_gene_set_;
	QString project_;
	QString ps_id_;
	RnaCohortDeterminationStategy cohort_type_;

	Ui::ExpressionExonWidget *ui_;

	//file info
	TsvFile expression_data_;
	FilterResult filter_result_;	

	//NGSD
//	NGSD db_;
//	QSet<int> cohort_;
//	SqlQuery exon_query_ = db_.getQuery();
//	QMap<QByteArray,DBExpressionValues> db_expression_data_;
//	QSet<QByteArray> valid_exons_;

	//table info
	QStringList column_names_;
	QVector<bool> numeric_columns_;
	QVector<int> precision_;
//	QStringList db_column_names_;

	//cohort
//	QTableWidget* cohort_table_ = nullptr;


	void loadExpressionFile();
	void initFilter();
	void initTable();
	void updateTable();
	void initBiotypeList();
	void getDBExpressionData(int line_idx);

private slots:
	void applyFilters();
	void copyToClipboard();
	void showBiotypeContextMenu(QPoint pos);
	void selectAllBiotypes(bool deselect=false);
	void showHistogram(int row_idx);
	void showExpressionTableContextMenu(QPoint pos);
	void showCohort();
	void copyCohortToClipboard();
	void updateCohort();


};

#endif // EXPRESSIONEXONWIDGET_H
