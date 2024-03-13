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
	RepeatExpansionWidget(QWidget* parent, QString vcf);

private slots:
    ///Context menu that shall appear if right click on repeat expansion
    void showContextMenu(QPoint pos);
	///Open the region in IGV if a cell is double-clicked
	void cellDoubleClicked(int row, int col);

protected:
	///Override copy command
	void keyPressEvent(QKeyEvent* event) override;

	///Sets the text value of a cell. Return the item pointer.
	QTableWidgetItem* setCell(int row, QString column, QString value);
	///Returns the text value of a cell.
	QString getCell(int row, QString column);

	///Sets the cell decoration
	void setCellDecoration(int row, QString column, QString tooltip, QColor bg_color=QColor());

private:
	Ui::RepeatExpansionWidget ui_;
	QColor red_ = QColor(255, 0, 0, 128);
	QColor orange_ = QColor(255, 135, 60, 128);

	void loadDataFromVCF(QString vcf);
	void loadMetaDataFromNGSD();
	void colorRepeatCountBasedOnCutoffs();
	void updateFilters();
};

#endif // REPEATEXPANSIONWIDGET_H
