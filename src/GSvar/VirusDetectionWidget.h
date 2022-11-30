#ifndef VIRUSDETECTIONWIDGET_H
#define VIRUSDETECTIONWIDGET_H

#include <QWidget>
#include "ui_VirusDetectionWidget.h"
#include "FileLocation.h"

class VirusDetectionWidget
	: public QWidget
{
	Q_OBJECT

public:
	VirusDetectionWidget(QString viral_file, QWidget* parent = 0);

Q_SIGNALS:
	void cellDoubleClicked(int row, int column);

protected slots:

	void callViewInIGV(int row, int col);
	void callCustomMenu(QPoint pos);
	void callViewInIGV();
	void callCopyToClipboard();

private:
	void openInIGV(int row);
	void populateTable();
	Ui::VirusDetectionWidget ui_;
	QString viral_file_;
	bool igv_initialized_;

};

#endif // VIRUSDETECTIONWIDGET_H
