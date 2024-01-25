#ifndef SOMATICREPORTDIALOG_H
#define SOMATICREPORTDIALOG_H

#include <QDialog>
#include "SomaticReportSettings.h"
#include "NGSD.h"
#include "BamReader.h"
#include "ui_SomaticReportDialog.h"
#include "MainWindow.h"
#include "HttpHandler.h"

class SomaticReportDialog
		: public QDialog
{
	Q_OBJECT

public:
	///constructor
	SomaticReportDialog(QString project_filename, SomaticReportSettings& settings, const CnvList& cnvs, const VariantList& germl_variants, QWidget* parent = 0);

	enum report_type
	{
		DNA,
		RNA,
		cfDNA
	};

	///Returns whether report type shall be DNA or RNA
	report_type getReportType();

	///Returns RNA id for RNA report (as selected by user)
	QString getRNAid();

	///enable option to choose between DNA or RNA report
	void enableChoiceRnaReportType(bool enabled);
	void enableChoicecfDnaReportType(bool enabled);

	///adds RNA ids to combo box
	void setRNAids(const QStringList& rna_ids);

private:
	Ui::SomaticReportDialog ui_;
	NGSD db_;
	SomaticReportSettings& settings_;
	CnvList cnvs_;
	const VariantList& germl_variants_;
	QString target_region_;

	double tum_cont_snps_;
	double tum_cont_max_clonality_;
	double tum_cont_histological_;

	QString limitations_;
	QString project_filename_;


public slots:
	///writes settings into "settings" passed to constructur
	void writeBackSettings();

	///for debugging purposes: returns whether report config shall be stored to NGSD
	bool skipNGSD();

private slots:
	///Deactivates all GUI elements (i.e. disable all tabs, e.g. if user chooses RNA report)
	void disableGUI();
	///Enables all GUI tabs
	void enableGUI();

	///enables/disables combobox with RNA ids.
	void rnaSampleSelection();

	///Disable/Enable CIN tab and uncheck all chromosomes. Depends whether CNV burden is checked
	void cinState();

	///Disable/Enable limitations text field
	void limitationState();

	///Disable/Enable quality comments to be checkable.
	void qualityState();

	///Creates screenshot with somatic tracks from IGV
	void createIgvScreenshot();

	///Updates hint whether IGV snapshot file is available.
	void updateIgvText();

	///Returns list of all chromosomes checked in CIN tab
	QList<QString> resolveCIN();

};

#endif // SOMATICREPORTDIALOG_H
