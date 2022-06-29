#ifndef EXPRESSIONEXONWIDGET_H
#define EXPRESSIONEXONWIDGET_H

#include <QWidget>

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
	Ui::ExpressionExonWidget *ui_;

	QString tsv_filename_;
	GeneSet variant_gene_set_;

	//table info
	QStringList column_names_;
	QVector<bool> numeric_columns_;
	QVector<int> precision_;


	void loadExpressionFile();
	void initBiotypeList();
};

#endif // EXPRESSIONEXONWIDGET_H
