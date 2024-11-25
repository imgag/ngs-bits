#ifndef METHYLATIONWIDGET_H
#define METHYLATIONWIDGET_H

#include <QTableWidgetItem>
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

private slots:
	void openMethylationPlot(int row_idx, int col_idx);

};

#endif // METHYLATIONWIDGET_H
