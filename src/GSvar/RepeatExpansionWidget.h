#ifndef REPEATEXPANSIONWIDGET_H
#define REPEATEXPANSIONWIDGET_H

#include <QWidget>
#include <QTableWidget>

namespace Ui {
class RepeatExpansionWidget;
}

// helper struct to store repeat cutoff values
struct RepeatCutoff
{
	QByteArray repeat_id;
	QByteArray repeat_unit;
	int max_normal;
	int min_pathogenic;
	QByteArray inheritance;
};


// custom QTableWidgetItem class to allow inplace sorting of doubles
class NumericWidgetItem: public QTableWidgetItem
{
public:
	NumericWidgetItem(QString text);
	bool operator< (const QTableWidgetItem &other) const;
};

class RepeatExpansionWidget : public QWidget
{
	Q_OBJECT

public:
	RepeatExpansionWidget(QString vcf_filename, QWidget *parent = 0);
	~RepeatExpansionWidget();


private:
	void loadRepeatExpansionData();
	void colorRepeatCell(QTableWidgetItem* cell, const QMap<QPair<QByteArray,QByteArray>, QPair<int,int>>& cutoffs);
	QString vcf_filename_;
	Ui::RepeatExpansionWidget *ui_;
};

#endif // REPEATEXPANSIONWIDGET_H
