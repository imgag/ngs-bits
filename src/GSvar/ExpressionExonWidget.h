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
	Ui::ExpressionExonWidget *ui_;

	// parameter
	QString tsv_filename_;
	int sys_id_;
	QString tissue_;
	QString variant_gene_filter_;
	GeneSet variant_gene_set_;
	QString project_;
	QString ps_id_;
	RnaCohortDeterminationStategy cohort_type_;

	//file info
	TsvFile expression_data_;
	FilterResult filter_result_;	

	//table info
	QStringList column_names_;
	QVector<bool> numeric_columns_;
	QVector<int> precision_;



	void loadExpressionFile();
	void initFilter();
	void initTable();
	void updateTable();
	void initBiotypeList();

private slots:
	void applyFilters();
	void copyToClipboard();
	void showBiotypeContextMenu(QPoint pos);
	void selectAllBiotypes(bool deselect=false);
	void showHistogram(int row_idx);
	void showExpressionTableContextMenu(QPoint pos);



};

#endif // EXPRESSIONEXONWIDGET_H
