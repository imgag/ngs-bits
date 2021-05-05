#ifndef CFDNAPANELWIDGET_H
#define CFDNAPANELWIDGET_H

#include "BedFile.h"
#include "NGSD.h"
#include <QWidget>


namespace Ui {
class cfDNAPanelWidget;
}

class CfDNAPanelWidget : public QWidget
{
	Q_OBJECT

public:
	explicit CfDNAPanelWidget(const CfdnaPanelInfo& panel_info, QWidget *parent = 0);
	~CfDNAPanelWidget();

private slots:
    void copyToClipboard();
	void exportBed();

private:
	Ui::cfDNAPanelWidget *ui_;
	void loadBedFile();
	CfdnaPanelInfo panel_info_;
	BedFile bed_file_;

};

#endif // CFDNAPANELWIDGET_H
