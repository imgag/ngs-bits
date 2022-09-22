#ifndef CFDNAREMOVEDREGIONS_H
#define CFDNAREMOVEDREGIONS_H

#include <BedFile.h>
#include <NGSD.h>
#include <QDialog>

namespace Ui {
class cfDNARemovedRegions;
}

class cfDNARemovedRegions : public QDialog
{
	Q_OBJECT

public:
	explicit cfDNARemovedRegions(const QString& processed_sample_name, QWidget *parent = nullptr);
	~cfDNARemovedRegions();

private:
	Ui::cfDNARemovedRegions *ui_;
	void initGui();

	QString processed_sample_name_;
	CfdnaPanelInfo cfdna_panel_info_;

private slots:
	void importInNGSD();
};

#endif // CFDNAREMOVEDREGIONS_H
