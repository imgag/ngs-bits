#ifndef REPEATEXPANSIONWIDGET_H
#define REPEATEXPANSIONWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include "PhenotypeList.h"
#include "ui_RepeatExpansionWidget.h"

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
	///Show/hide rows according to filters
	void updateRowVisibility();

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
};

#endif // REPEATEXPANSIONWIDGET_H
