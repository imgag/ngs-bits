#ifndef SAMPLEDISEASEINFOWIDGET_H
#define SAMPLEDISEASEINFOWIDGET_H

#include "NGSD.h"
#include <QWidget>

namespace Ui {
class SampleDiseaseInfoWidget;
}

//Sample disease info dialog
class SampleDiseaseInfoWidget
	: public QWidget
{
	Q_OBJECT

public:
	SampleDiseaseInfoWidget(QString sample_name, QWidget* parent = 0);
	~SampleDiseaseInfoWidget();

	void setDiseaseInfo(const QList<SampleDiseaseInfo>& disease_info);
	const QList<SampleDiseaseInfo>& diseaseInfo() const;

protected slots:
	void updateDiseaseInfoTable();
	void addDiseaseInfo();
	void removeDiseaseInfo();
	void importDiseaseInfoFromGenLab();

private:
	Ui::SampleDiseaseInfoWidget* ui_;
	QString sample_name_;
	QList<SampleDiseaseInfo> disease_info_;
	NGSD db_;
};

#endif // SAMPLEDISEASEINFOWIDGET_H
