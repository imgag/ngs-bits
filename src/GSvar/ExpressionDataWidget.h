#ifndef EXPRESSIONDATAWIDGET_H
#define EXPRESSIONDATAWIDGET_H

#include <QTableWidget>
#include <QWidget>

namespace Ui {
class ExpressionDataWidget;
}

// custom QTableWidgetItem class to allow inplace sorting of doubles
class NumericWidgetItem: public QTableWidgetItem
{
public:
	NumericWidgetItem(QString text):QTableWidgetItem(text){}
	bool operator< (const QTableWidgetItem &other) const;
};

class ExpressionDataWidget : public QWidget
{
	Q_OBJECT

public:
	ExpressionDataWidget(QString tsv_filename, QWidget *parent = 0);
	~ExpressionDataWidget();

protected slots:
	void applyFilters();

private:
	void loadExpressionData();
	QString tsv_filename_;
	Ui::ExpressionDataWidget *ui_;

	//table info
	QStringList column_names_;
	QVector<bool> numeric_columns_;
};

#endif // EXPRESSIONDATAWIDGET_H
