#ifndef REPEATEXPANSIONWIDGET_H
#define REPEATEXPANSIONWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include "ui_RepeatExpansionWidget.h"

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

class RepeatExpansionWidget
	: public QWidget
{
	Q_OBJECT

public:
	RepeatExpansionWidget(QString vcf_filename, bool is_exome=false, QWidget *parent = 0);

private slots:
    ///Context menu that shall appear if right click on repeat expansion
    void showContextMenu(QPoint pos);
	///Open the region in IGV if a cell is double-clicked
	void cellDoubleClicked(int row, int col);

protected:
	///Override copy command
	void keyPressEvent(QKeyEvent* event) override;

private:
	Ui::RepeatExpansionWidget ui_;
	QString vcf_filename_;
	bool is_exome_;

	void loadRepeatExpansionData();
};

#endif // REPEATEXPANSIONWIDGET_H
