#ifndef SVWIDGET_H
#define SVWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include <QByteArray>
#include <QByteArrayList>
#include <NGSD.h>
#include "BedpeFile.h"
#include "FilterWidget.h"
#include "ReportConfiguration.h"

namespace Ui {
	class SvWidget;
}

///Widget for visualization and filtering of Structural Variants.
class SvWidget
	: public QWidget
{
	Q_OBJECT

public:
	SvWidget(QWidget* parent, const BedpeFile& bedpe_file, QString ps_id, QSharedPointer<ReportConfiguration> rep_conf, const GeneSet& het_hit_genes);
	~SvWidget();

	//constructor with somatic report config
	SvWidget(QWidget* parent, const BedpeFile& bedpe_file, QString ps_id, SomaticReportConfiguration& som_rep_conf, const GeneSet& het_hit_genes);

signals:
	void updateSomaticReportConfiguration();

protected slots:
	///copy filtered SV table to clipboard
	void copyToClipboard();

	///update SV table if filter for types was changed
	void applyFilters(bool debug_time = false);

	void SvDoubleClicked(QTableWidgetItem* item);

	///update SV details and INFO widgets
	void SvSelectionChanged();

	///Context menu that shall appear if right click on variant
	void showContextMenu(QPoint pos);


	///Extracts entry of column following to "FORMAT" column.
	QByteArray getFormatEntryByKey(const QByteArray& key, const QByteArray& format_desc, const QByteArray& format_data);

	///Import phenotypes from NGSD
	void importPhenotypesFromNGSD();

	///Edit report config
	void svHeaderDoubleClicked(int row);

	///Show context menu to add/remove SV to/from report config
	void svHeaderContextMenu(QPoint pos);

private slots:
	void updateReportConfigHeaderIcon(int row);
	void editReportConfiguration(int row);

	///Loads the gene file to a given target region BED file
	void annotateTargetRegionGeneOverlap();
	///Removes the calculated gene overlap tooltips
	void clearTooltips();

	///Set custom column widths
	void adaptColumnWidthsCustom();

private:
	///load bedpe data file and set display
	void initGUI();

	void disableGUI(const QString& message);

	///Fill widgets with data from INFO_A and INFO_B column
	void setInfoWidgets(const QByteArray& name, int row, QTableWidget* widget);

	///Fill widgets with somatic infos for better overview
	void setSomaticInfos(int row);

	///resets all filters in widget
	void clearGUI();

	///resize widgets to cell content
	void resizeQTableWidget(QTableWidget* table_widget);

	///Returns paired read count supporting alternate Structural Variant
	int pairedEndReadCount(int row);

	///calculate AF of SV, either by paired end reads ("PR") or split reads ("SR");
	double alleleFrequency(int row, const QByteArray& read_type = "PR", int sample_idx = 0);

	///Edit validation status of current sv
	void editSvValidation(int row);

	void editGermlineReportConfiguration(int row);
	void editSomaticReportConfiguration(int row);

	///Upload structural variant to ClinVar
	void uploadToClinvar(int index1, int index2=-1);

	Ui::SvWidget* ui;
	BedpeFile sv_bedpe_file_;
	QStringList ps_ids_; //processed sample database IDs (only trio/multi). '' if unknown or NGSD is disabled.
	QString ps_id_; // processed sample id for the report config
	QStringList ps_names_; // processed sample names
	GeneSet var_het_genes_;

	QSharedPointer<ReportConfiguration> report_config_;
	SomaticReportConfiguration* som_report_config_;

	bool ngsd_enabled_;
	bool rc_enabled_;

	///List of annotations which are shown in the widget
	QByteArrayList annotations_to_show_;

	BedFile roi_;
	BedFile roi_genes_;
	ChromosomalIndex<BedFile> roi_gene_index_;

	//multisample
	bool is_somatic_;
	bool is_multisample_;
};

#endif // SVWIDGET_H
