#ifndef EXPRESSIONDATAWIDGET_H
#define EXPRESSIONDATAWIDGET_H

#include <QWidget>

namespace Ui {
class ExpressionDataWidget;
}

class ExpressionDataWidget : public QWidget
{
	Q_OBJECT

public:
	ExpressionDataWidget(QString tsv_filename, QWidget *parent = 0);
	~ExpressionDataWidget();


private:
	void loadExpressionData();
	QString tsv_filename_;
	Ui::ExpressionDataWidget *ui;
};

#endif // EXPRESSIONDATAWIDGET_H
