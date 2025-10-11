#ifndef VARIANTTABLE_H
#define VARIANTTABLE_H

#include <QTableWidget>
#include "FilterCascade.h"
#include "ReportSettings.h"
#include "SomaticReportSettings.h"

//GUI representation of (filtered) variant table
class VariantTable
	: public QTableWidget
{
	Q_OBJECT

public:
	VariantTable(QWidget* parent);

	///Update table
	void update(VariantList& variants, const FilterResult& filter_result, const ReportSettings& report_settings, int max_variants);
	///Update table, determine report icons from SomaticReportSettings
	void update(VariantList& variants, const FilterResult& filter_result, const SomaticReportSettings& report_settings, int max_variants);
	///Update header icon (report config)
	void updateVariantHeaderIcon(const ReportSettings& report_settings, int variant_index);
	///Update header icon (SOMATIC report config)
	void updateVariantHeaderIcon(const SomaticReportSettings& report_settings, int variant_index);

	///Returns the current variant index, or -1 if no/several variants are selected. If @p gui_indices is true, GUI table indices are returned instead of variant list index.
	int selectedVariantIndex(bool gui_indices = false) const;

	///Returns a sorted list of selected variants indices in the variant list. If @p gui_indices is true, GUI table indices are returned instead of variant list index.
	QList<int> selectedVariantsIndices(bool gui_indices = false) const;

	///Convert table row to variant index.
	int rowToVariantIndex(int row) const;
	///Convert variant index to table row (attention this can be slow because it is done by linear search).
	int variantIndexToRow(int index) const;

	///Creates table widget items, or nullptr if the text is empty.
	QTableWidgetItem* createTableItem(const QString& text) const
	{
		//no text > no item (optimization for WGS - empty items are big and take a lot of RAM)
		if (text.isEmpty()) return nullptr;

		return new QTableWidgetItem(text);
	}

	///Add custom context menu actions
	void addCustomContextMenuActions(QList<QAction*> actions);

	///Returns the current column widths.
	QList<int> columnWidths() const;

	///Sets column widths.
	void setColumnWidths(const QList<int>& widths);

	///Returns report config icon
	static QIcon reportIcon(bool show_in_report, bool causal);


public slots:
	void customContextMenu(QPoint pos);

	///Clear contents
	void clearContents();

	///Set the row heights
	void adaptRowHeights();

	///Resize table cells for better readability.
	void adaptColumnWidths();
	///Show all columns.
	void showAllColumns();

	///Copy table to clipboard
	void copyToClipboard(bool split_quality=false, bool include_header_one_row=false);

	///Update phenotypes that are currently active in the filter
	void updateActivePhenotypes(PhenotypeList phenotypes);

signals:
	///An added context menu action was triggered
	void customActionTriggered(QAction* action, int var_index);
	///Publish to Clinvar menu action triggered
	void publishToClinvarTriggered(int index1, int index2=-1);
	///Signal emitted when Alamut should be opened
	void alamutTriggered(QAction* action);
	///Signal to show CNVs/SVs matching a variant
	void showMatchingCnvsAndSvs(BedLine region);

protected:

	///This method provides generic functionality independent of ReportSettings/SomaticReportSettings
	void updateTable(VariantList& variants, const FilterResult& filter_result, const QHash<int, bool>& index_show_report_icon, const QSet<int>& index_causal, int max_variants);

	///Override copy command
	void keyPressEvent(QKeyEvent* event) override;

private:
	VariantList* variants_;
	QList<QAction*> registered_actions_;
	PhenotypeList active_phenotypes_;
};

#endif // VARIANTTABLE_H
