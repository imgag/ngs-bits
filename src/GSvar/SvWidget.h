#ifndef SVWIDGET_H
#define SVWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include <QByteArray>
#include <NGSD.h>
#include "BedpeFile.h"
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
	//Constructor for germline
	SvWidget(QWidget* parent, const BedpeFile& bedpe_file, QString ps_id, QSharedPointer<ReportConfiguration> rep_conf, const GeneSet& het_hit_genes);
	//Constructor for somatic
	SvWidget(QWidget* parent, const BedpeFile& bedpe_file, QSharedPointer<SomaticReportConfiguration> som_rep_conf, const GeneSet& het_hit_genes);
	~SvWidget();

signals:
	void updateSomaticReportConfiguration();

protected slots:
	///copy filtered SV table to clipboard
	void copyToClipboard();

	///update SV table if filter for types was changed
	void applyFilters(bool debug_time = false);

	void svDoubleClicked(QTableWidgetItem* item);

	///update SV details and INFO widgets
	void updateFormatAndInfoTables();

	///Context menu that shall appear if right click on variant
	void showContextMenu(QPoint pos);

	///Import phenotypes from NGSD
	void importPhenotypesFromNGSD();

	///Edit report config
	void svHeaderDoubleClicked(int row);

	///Show context menu to add/remove SV to/from report config
	void svHeaderContextMenu(QPoint pos);

	void openColumnSettings();
	void adaptColumnWidthsAndHeights();
	void showAllColumns();

private slots:
	void updateReportConfigHeaderIcon(int row);
	void editReportConfiguration(int row);
	///Loads the gene file to a given target region BED file
	void annotateTargetRegionGeneOverlap();
	///Removes the calculated gene overlap tooltips
	void clearTooltips();

private:

	///set up user interface
	void setupUI();

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
	void resizeTableContent(QTableWidget* table_widget);

	///calculate AF of SV, either by paired end reads ("PR") or split reads ("SR");
	double alleleFrequency(int row, QByteArray sample, QByteArray read_type);

	///Edit validation status of current sv
	void editSvValidation(int row);

	void editGermlineReportConfiguration(int row);
	void editSomaticReportConfiguration(int row);

	///Upload structural variant to ClinVar
	void uploadToClinvar(int index1, int index2=-1);

	Ui::SvWidget* ui;
	BedpeFile svs_;
	QString ps_id_; // processed sample id for validation, ClinVar upload and loading phenotypes (germline only)
	QStringList ps_names_; //processed sample names (germline only)
	GeneSet var_het_genes_;

	QSharedPointer<ReportConfiguration> report_config_;
	QSharedPointer<SomaticReportConfiguration> som_report_config_;

    bool ngsd_user_logged_in_;
	bool rc_enabled_;

	bool is_somatic_;
	bool is_multisample_;
};

#endif // SVWIDGET_H
