#ifndef COLUMNCONFIGWIDGET_H
#define COLUMNCONFIGWIDGET_H

#include <QWidget>
#include "ui_ColumnConfigWidget.h"


class ColumnConfigWidget
	: public QWidget
{
	Q_OBJECT

public:
	ColumnConfigWidget(QWidget* parent);

private slots:
	void importColumns();
	void clearColumns();

	void addColumn(QString name);
	void moveRowUp();
	void moveRowDown();
	void sizeChanged(int row, int col);

	void store();

private:
	Ui::ColumnConfigWidget ui_;
	QString title_;

	void swapRows(int from, int to);
};

#endif // COLUMNCONFIGWIDGET_H
