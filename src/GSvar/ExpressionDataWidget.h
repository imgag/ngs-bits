#ifndef EXPRESSIONDATAWIDGET_H
#define EXPRESSIONDATAWIDGET_H

#include <QTableWidget>
#include <QWidget>

namespace Ui {
class ExpressionDataWidget;
}

class ExpressionDataWidget : public QWidget
{
	Q_OBJECT

public:
	ExpressionDataWidget(QString tsv_filename, int sys_id, QString tissue_, QWidget *parent = 0);
	~ExpressionDataWidget();

private slots:
	void applyFilters();
	void copyToClipboard();
	void showBiotypeContextMenu(QPoint pos);
	void selectAllBiotypes(bool deselect=false);

private:
	void loadExpressionData();
	QString tsv_filename_;
	int sys_id_;
	QString tissue_;
	Ui::ExpressionDataWidget *ui_;

	//table info
	QStringList column_names_;
	QVector<bool> numeric_columns_;
	QVector<int> precision_;
};

#endif // EXPRESSIONDATAWIDGET_H
