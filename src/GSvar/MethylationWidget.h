#ifndef METHYLATIONWIDGET_H
#define METHYLATIONWIDGET_H

#include <QWidget>
#include "TsvFile.h"

namespace Ui {
class MethylationWidget;
}

class MethylationWidget : public QWidget
{
	Q_OBJECT

public:
	explicit MethylationWidget( QString filename, QWidget *parent = nullptr);
	~MethylationWidget();

private:
	Ui::MethylationWidget *ui_;
	QString filename_;
	TsvFile data_;
	void loadFile();

	QColor red_ = QColor(255, 0, 0, 128);
	QColor orange_ = QColor(255, 135, 60, 128);
	QColor yellow_ = QColor(255, 255, 0, 128);


private slots:
	void openMethylationPlot(int row_idx, int);
	void updateTable();

};

#endif // METHYLATIONWIDGET_H
