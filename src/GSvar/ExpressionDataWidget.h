#ifndef EXPRESSIONDATAWIDGET_H
#define EXPRESSIONDATAWIDGET_H

#include "NGSD.h"
#include <GeneSet.h>
#include <QTableWidget>
#include <QWidget>

namespace Ui {
class ExpressionDataWidget;
}

class ExpressionDataWidget : public QWidget
{
	Q_OBJECT

public:
	ExpressionDataWidget(QString tsv_filename, int sys_id, QString tissue, const QString& variant_gene_filter = "", const GeneSet& variant_gene_set = GeneSet(), const QString& project = "",
						 const QString& ps_id = "", RnaCohortDeterminationStategy cohort_type_=RNA_COHORT_GERMLINE, QWidget *parent = 0);
	~ExpressionDataWidget();

private slots:
	void applyFilters();
	void copyToClipboard();
	void showBiotypeContextMenu(QPoint pos);
	void selectAllBiotypes(bool deselect=false);
	void showHistogram(int row_idx);
	void showExpressionTableContextMenu(QPoint pos);
	void showCohort();

private:
	void loadExpressionData();
	QString tsv_filename_;
	int sys_id_;
	QString tissue_;
	GeneSet variant_gene_set_;
	QString project_;
	QString ps_id_;
	RnaCohortDeterminationStategy cohort_type_;
	QSet<int> cohort_;
	Ui::ExpressionDataWidget *ui_;


	//table info
	QStringList column_names_;
	QVector<bool> numeric_columns_;
	QVector<int> precision_;
};

#endif // EXPRESSIONDATAWIDGET_H
