#ifndef CFDNAPANELWIDGET_H
#define CFDNAPANELWIDGET_H

#include "BedFile.h"
#include <QWidget>

namespace Ui {
class cfDNAPanelWidget;
}

class CfDNAPanelWidget : public QWidget
{
	Q_OBJECT

public:
	explicit CfDNAPanelWidget(const QString& bed_file_path, const QString& tumor_sample_name, QWidget *parent = 0);
	~CfDNAPanelWidget();

private slots:
    void copyToClipboard();
	void exportBed();

private:
	Ui::cfDNAPanelWidget *ui_;
	void loadBedFile();
	QString bed_file_path_;
	QString tumor_sample_name_;
	BedFile bed_file_;

};

#endif // CFDNAPANELWIDGET_H
