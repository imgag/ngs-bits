#ifndef VIRUSDETECTIONWIDGET_H
#define VIRUSDETECTIONWIDGET_H

#include <QTableWidget>

class VirusDetectionWidget
	: public QTableWidget
{
	Q_OBJECT

public:
	VirusDetectionWidget(QString viral_file, QWidget* parent = 0);

signals:
	void cellDoubleClicked(int row, int column);

protected slots:

	void callViewInIGV(int row, int col);
	void callCustomMenu(QPoint pos);
	void callViewInIGV();
	void callCopyToClipboard();

protected:
	///Override copy command
	void keyPressEvent(QKeyEvent* event) override;

private:
	void openInIGV(int row);
	void populateTable();
	QString viral_file_;
	bool igv_initialized_;

};

#endif // VIRUSDETECTIONWIDGET_H
