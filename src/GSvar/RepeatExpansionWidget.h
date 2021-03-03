#ifndef REPEATEXPANSIONWIDGET_H
#define REPEATEXPANSIONWIDGET_H

#include <QWidget>
#include <QTableWidget>

namespace Ui {
class RepeatExpansionWidget;
}

// helper struct to store repeat cutoff values
struct RepeatCutoffInfo
{
	QByteArray repeat_id;
	QByteArray repeat_unit;
	int max_normal = -1;
	int min_pathogenic = -1;
	QByteArray inheritance;
	bool reliable_in_exomes = true;
	QByteArrayList additional_info;
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
	RepeatExpansionWidget(QString vcf_filename, bool is_exome=false, QWidget *parent = 0);
	~RepeatExpansionWidget();


private:
	void loadRepeatExpansionData();
	QString vcf_filename_;
	bool is_exome_;
	Ui::RepeatExpansionWidget *ui_;
};

#endif // REPEATEXPANSIONWIDGET_H
