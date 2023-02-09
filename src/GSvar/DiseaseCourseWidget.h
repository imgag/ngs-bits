#ifndef DISEASECOURSEWIDGET_H
#define DISEASECOURSEWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include <TsvFile.h>
#include "VcfFile.h"
#include "NGSD.h"
#include "LoginManager.h"
#include "GSvarHelper.h"

struct cfDnaColumn
{
	QString name;
	QDate date;
	VcfFile variants;
	QMap<QByteArray, const VcfLine*> lookup_table;
	TsvFile mrd;

	bool operator<(const cfDnaColumn& other) const {
		return date < other.date; // sort by date
	}
};

namespace Ui {
class DiseaseCourseWidget;
}

class DiseaseCourseWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DiseaseCourseWidget(const QString& tumor_sample_name, QWidget *parent = 0);
	~DiseaseCourseWidget();

protected slots:
	void VariantDoubleClicked(QTableWidgetItem* item);
	void copyToClipboard();


private:
	void createTableView();
	QStringList annotateVariant(const VcfLine& variant, GeneSet& genes);
	Ui::DiseaseCourseWidget *ui_;
	NGSD db_;
	CfdnaDiseaseCourseTable table_data_;
	QString tumor_sample_name_;
	bool igv_initialized_ = false;
};

#endif // DISEASECOURSEWIDGET_H
