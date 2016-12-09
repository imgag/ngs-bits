#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "ui_MainWindow.h"
#include "VariantList.h"
#include "FilterDockWidget.h"
#include "VariantDetailsDockWidget.h"
#include "VariantFilter.h"
#include "BedFile.h"
#include "NGSD.h"
#include "FileWatcher.h"
#include "BusyDialog.h"

///Main window class
class MainWindow
		: public QMainWindow
{
	Q_OBJECT
	
public:
	///Constructor
	MainWindow(QWidget* parent = 0);
	///Loads a variant list
	void loadFile(QString filename);
	///Updates the GUI after the variant list changed
	void variantListChanged();
	///Sends a command to IGV through the default socket
	void executeIGVCommand(QString command);
	///Returns the LOG files corresponding to the variant list.
	QStringList getLogFiles();
	///Returns the BAM file corresponding to the variant list (single sample folder). If no or more BAM files are found, an empty string is returned.
	QString getBamFile();
	///Returns the BAM files of a trio folder. Order is the same as in the folder name. If no or more BAM files are found, an empty list is returned.
	QStringList getBamFilesTrio();
	///Returns if the current file is a trio file
	bool isTrio();
	///Adds a file to the recent file list
	void addToRecentFiles(QString filename);
	///Updates recent files menu
	void updateRecentFilesMenu();
	///Updates IGV menu
	void updateIGVMenu();
	///Updates preferred transcripts from settings.ini
	void updatePreferredTranscripts();
	///Updates menu and toolbar according to NGSD-support
	void updateNGSDSupport();
	///Copy selected variants to clipboard
	void copyToClipboard(bool split_quality);
	///Returns 'nobr' paragraph start for Qt tooltips
	static QString nobr();
	///Upload variant to LOVD
	void uploadtoLovd(int variant_index);
	///Returns the target file name without extension and date part prefixed with '_', or an empty string if no target file is set
	QString targetFileName() const;


public slots:
	///Open dialog
	void on_actionOpen_triggered();
	///Open dialog for NGSD
	void on_actionOpenNGSD_triggered();
	///ChangeLog action
	void on_actionChangeLog_triggered();
	///About dialog
	void on_actionAbout_triggered();
	///Resizes columns accoding to content
	void on_actionResize_triggered();
	///Report dialog
	void on_actionReport_triggered();
	///Database annotation
	void on_actionDatabase_triggered();
    ///NGSD link
	void on_actionNGSD_triggered();
	///Sample information
	void on_actionSampleInformation_triggered();
	///Gender determination
	void on_actionGenderXY_triggered();
	///Gender determination
	void on_actionGenderHet_triggered();
	///File information BED
	void on_actionStatisticsBED_triggered();
	///File information FASTA
	void on_actionStatisticsFastA_triggered();
	///Copy to clipboard
	void on_actionCopy_triggered();
	///Copy to clipboard and split quality column
	void on_actionCopySplit_triggered();
	///Sample correlation BAM
	void on_actionSampleCorrelationBAM_triggered();
	///Sample correlation TSV
	void on_actionSampleCorrelationTSV_triggered();
	///Sample difference
	void on_actionSampleDiff_triggered();
	///Trio analysis
	void on_actionTrio_triggered();
	///Lookup gaps in low-coverage BED file
	void on_actionGapsLookup_triggered();
	///Calculate gaps based on current target region
	void on_actionGapsRecalculate_triggered();
	///VCF export
	void on_actionExportVCF_triggered();
	///GSvar export
	void on_actionExportGSvar_triggered();
	///Preferred transcript list
	void on_actionShowTranscripts_triggered();
	///Preferred transcript import
	void on_actionImportTranscripts_triggered();
	///Opens online documentation
	void on_actionOpenDocumentation_triggered();
	///Approved symbols dialog
	void on_actionConvertHgnc_triggered();
	///Germline gene information dialog
	void on_actionGeneInfo_triggered();
	///HPO to regions conversion dialog
	void on_actionPhenoToGenes_triggered();
	///Genes to regions conversion dialog
	void on_actionGenesToRegions_triggered();
	///Subpanel design dialog
	void on_actionDesignSubpanel_triggered();
	///Subpanel archive dialog
	void on_actionArchiveSubpanel_triggered();
	///Close current file
	void on_actionClose_triggered();
	///Force IGV initializazion
	void on_actionIgvInit_triggered();
	///Open CNV dialog
	void on_actionCNV_triggered();
	///Open gene picker dialog
	void on_actionGeneSelector_triggered();
	///Create sample overview file
	void on_actionSampleOverview_triggered();

	///Finished the report generation
	void reportGenerationFinished(bool success);
	///Finished NGSD annotation
	void databaseAnnotationFinished(bool success);
	///Shows the variant list contect menu
	void varsContextMenu(QPoint pos);
	///Updated the variant context menu
	void updateVariantDetails();
	///Updates the visible rows after filters have changed
	void filtersChanged();
	///Opens the recent file defined by the sender action text
	void openRecentFile();
	///Loads the command line input file.
	void delayedInizialization();
	///Handles the re-loading the variant list when the file changes.
	void handleInputFileChange();
	///A variant has been double-clicked > open in IGV
	void variantDoubleClicked(QTableWidgetItem* item);
	///Open region in IGV
	void openInIGV(QString region);

	///Returns a gene list where the inheritance information is not set (selected variants only)
	QStringList geneInheritanceMissing(QBitArray selected);

	///Adds a modeless dialog
	void addModelessDialog(QSharedPointer<QDialog> ptr);
	///Removes all modeless dialogs that have been closed
	void cleanUpModelessDialogs();

    ///Default filters
    void applyDefaultFiltersGermline();
	void applyDefaultFiltersTrio();
	void applyDefaultFiltersMultiSample();
    void applyDefaultFiltersSomatic();
    void clearFilters();

protected:
	virtual void dragEnterEvent(QDragEnterEvent* e);
	virtual void dropEvent(QDropEvent* e);

private:
	//GUI
	Ui::MainWindow ui_;
	FilterDockWidget* filter_widget_;
	int var_last_;
	VariantDetailsDockWidget* var_widget_;
	BusyDialog* busy_dialog_;

	//DATA
	QString filename_;
	FileWatcher filewatcher_;
	bool db_annos_updated_;
	bool igv_initialized_;
	VariantList variants_;
	QMap<QString, QString> link_columns_;
	QSet<int> link_indices_;
	QString last_roi_filename_;
	BedFile last_roi_;
	QString last_report_path_;
	QMap<QString, QString> preferred_transcripts_;
	QList<QSharedPointer<QDialog>> modeless_dialogs_;

	//SPECIAL
	///Timer to delay some initialization, e.g. load CLI argument after the main window is visible (otherwise the sample info dialog is shown before the main window)
	QTimer delayed_init_timer_;
};

#endif // MAINWINDOW_H
