#ifndef SVWIDGET_H
#define SVWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include <QByteArray>
#include <QByteArrayList>
#include "SvList.h"
#include "BedPeFile.h"

namespace Ui {
	class SvWidget;
}

///Widget for visualization and filtering of Structural Variants.
class SvWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SvWidget(const QStringList& bedpe_file_paths, QWidget *parent = 0);

private:
	Ui::SvWidget *ui;

	SvList svs_;

	BedpeFile sv_bedpe_file_;

	///List of annotations which are shown in the widget
	QByteArrayList annotations_to_show_;

	void addInfoLine(const QString text);

	///load bedpe data file and set display
	void loadSVs(const QString& file_name);

	void disableGui(const QString& message);

	///Returns column index of main QTableWidget svs_ by Name, -1 if not in widget
	int colIndexbyName(const QString& name);

	///File widgets with data from INFO_A and INFO_B column
	void setInfoWidgets(const QByteArray& name, int row, QTableWidget* widget);

	///resets all filters in widget
	void clearGUI();

	///resize widgets to cell content
	void resizeQTableWidget(QTableWidget* table_widget);

	///Returns paired read count supporting alternate Structural Variant
	int pairedEndReadCount(int row);

	///calculate AF of SV, either by paired end reads ("PR") or split reads ("SR");
	double alleleFrequency(int row, const QByteArray& read_type = "PR");

signals:
	void openSvInIGV(QString coords);

private slots:
	///copy filtered SV table to clipboard
	void copyToClipboard();

	///load new SV file if other bedpe file is selected
	void fileNameChanged();

	///update SV table if filter for types was changed
	void filtersChanged();

	void SvDoubleClicked(QTableWidgetItem* item);

	///update SV details and INFO widgets
	void SvSelectionChanged();

	///Context menu that shall appear if right click on variant
	void showContextMenu(QPoint pos);

	///Extracts entry of column following to "FORMAT" column.
	QByteArray getFormatEntryByKey(const QByteArray& key, const QByteArray& format_desc, const QByteArray& format_data);
};

#endif // SVWIDGET_H
