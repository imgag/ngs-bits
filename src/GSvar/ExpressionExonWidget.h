#ifndef EXPRESSIONEXONWIDGET_H
#define EXPRESSIONEXONWIDGET_H

#include <FilterCascade.h>
#include <QWidget>

#include "TsvFile.h"
#include "GeneSet.h"

namespace Ui {
class ExpressionExonWidget;
}

class ExpressionExonWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ExpressionExonWidget(QString tsv_filename, const QString& variant_gene_filter = "", const GeneSet& variant_gene_set = GeneSet(), QWidget *parent = 0);
	~ExpressionExonWidget();

private:
	QString tsv_filename_;
	QString variant_gene_filter_;
	GeneSet variant_gene_set_;
	TsvFile expression_data_;
	FilterResult filter_result_;

	Ui::ExpressionExonWidget *ui_;

	//table info
	QStringList column_names_;
	QVector<bool> numeric_columns_;
	QVector<int> precision_;


	void loadExpressionFile();
	void initFilter();
	void initTable();
	void updateTable(int max_rows = 10000);
	void initBiotypeList();

private slots:
	void applyFilters();

};

#endif // EXPRESSIONEXONWIDGET_H
