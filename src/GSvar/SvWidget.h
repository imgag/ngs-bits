#ifndef SVWIDGET_H
#define SVWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include <QByteArray>
#include <QByteArrayList>
#include "BedpeFile.h"
#include "FilterWidget.h"

namespace Ui {
	class SvWidget;
}

///Widget for visualization and filtering of Structural Variants.
class SvWidget
	: public QWidget
{
	Q_OBJECT

public:
	SvWidget(const QStringList& bedpe_file_paths, FilterWidget* filter_widget, QHash<QByteArray, BedFile>& cache, QWidget *parent = 0);

signals:
	void openSvInIGV(QString coords);

protected slots:
	///copy filtered SV table to clipboard
	void copyToClipboard();

	///load new SV file if other bedpe file is selected
	void fileNameChanged();

	///update SV table if filter for types was changed
	void applyFilters();

	void SvDoubleClicked(QTableWidgetItem* item);

	///update SV details and INFO widgets
	void SvSelectionChanged();

	///Context menu that shall appear if right click on variant
	void showContextMenu(QPoint pos);

	///Extracts entry of column following to "FORMAT" column.
	QByteArray getFormatEntryByKey(const QByteArray& key, const QByteArray& format_desc, const QByteArray& format_data);

	void importROI();
	void roiSelectionChanged(int index);

	void importHPO();
	void editPhenotypes();
	void phenotypesChanged();
	void showPhenotypeContextMenu(QPoint pos);

private:
	Ui::SvWidget* ui;

	bool loading_svs_ = false;

	BedpeFile sv_bedpe_file_;

	///List of annotations which are shown in the widget
	QByteArrayList annotations_to_show_;

	BedFile roi_;
	QString roi_filename_;

	QList<Phenotype> phenotypes_;
	BedFile phenotypes_roi_; //cache for phenotype target region (avoid re-calculating it every time the filters are applied)

	FilterWidget* filter_widget_;

	QHash<QByteArray, BedFile>& gene2region_cache_;

	///load bedpe data file and set display
	void loadSVs(const QString& file_name);

	void disableGUI(const QString& message);

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

};

#endif // SVWIDGET_H
