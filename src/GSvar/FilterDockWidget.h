#ifndef FILTERDOCKWIDGET_H
#define FILTERDOCKWIDGET_H

#include <QDockWidget>
#include "ui_FilterDockWidget.h"
#include "BedFile.h"
#include "GeneSet.h"
#include "Phenotype.h"

///Filter manager dock widget
class FilterDockWidget
	: public QDockWidget
{
	Q_OBJECT
	
public:
	/// Default constructor
	FilterDockWidget(QWidget *parent = 0);

	/// Resets to initial state (uncheck boxes, no ROI)
	void reset(bool clear_roi, bool clear_off_target);
	/// Sets filter columns present in the open file
	void setFilterColumns(const QMap<QString, QString>& filter_cols);

	/// Returns if the MAF filter is enabled.
	bool applyMaf() const;
	/// Returns the maximum MAF percentage filter value.
	double mafPerc() const;

	/// Returns if the MAF filter for sub-populations is enabled.
	bool applyMafSub() const;
	/// Returns the maximum MAF percentage filter value for sub-populations.
	double mafSubPerc() const;

	/// Returns if the impact filter is enabled.
	bool applyImpact() const;
	/// Returns valid impact.
	QStringList impact() const;

	/// Returns if the classification filter is enabled.
	bool applyClassification() const;
	/// Returns the minimum classification.
	int classification() const;

	/// Returns if the genotype filter is enabled (affected samples).
	bool applyGenotypeAffected() const;
	/// Returns the requested genotype (affected samples).
	QString genotypeAffected() const;

	/// Returns if the genotype filter is enabled (control samples).
	bool applyGenotypeControl() const;
	/// Returns the requested genotype (control samples).
	QString genotypeControl() const;

	/// Returns if the IHDB filter is enabled.
	bool applyIhdb() const;
	/// Returns the maximum IHDB filter value.
	int ihdb() const;
	/// Returns the if the genotype should be ignored for the IHDB filter.
	int ihdbIgnoreGenotype() const;

	/// Returns variants with a classification >= X should be kept (returns -1 if unset).
	int keepClassGreaterEqual() const;
	/// Returns variants with a classification 'M' (modifier) should be kept.
	bool keepClassM() const;

	/// Returns ExAC pLI score filter is enabled.
	bool applyPLI() const;
	/// Returns the minumum ExAC pLI score.
	double pli() const;

	///Returns the filter column terms to keep.
	QStringList filterColumnsKeep() const;
	///Returns the filter column terms to remove.
	QStringList filterColumnsRemove() const;
    ///Returns the filter column terms to filter.
	QStringList filterColumnsFilter() const;

	///Returns the target region BED file name or an empty string if unset.
	QString targetRegion() const;
	///Sets the target region BED file.
	void setTargetRegion(QString roi_file);

	/// Returns the gene names filter.
	GeneSet genes() const;
	/// Returns the single target region filter, or an empty string if unset.
	QString region() const;
	/// Sets the single target region filter, or an empty string if unset.
	void setRegion(QString region);


	///Returns selected phenotype terms.
	const QList<Phenotype>& phenotypes() const;
	///Sets selected phenotype terms.
	void setPhenotypes(const QList<Phenotype>& phenotypes);

	/// Returns the reference sample name or an empty string if unset.
	QString referenceSample() const;

	/// Returns a string representations of the applied filters
	QMap<QString, QString> appliedFilters() const;

	/// Loads filter target regions (Processing systems from NGSD, Sub-panels from file system and additional target regions from INI file)
	void loadTargetRegions();

public slots:

	/// Applies predefined default filters (germline).
	void applyDefaultFilters();
	/// Applies predefined default filters (germline - recessive).
	void applyDefaultFiltersRecessive();
	/// Applies predefined default filters (trio).
	void applyDefaultFiltersTrio();
	/// Applies predefined default filters (multi-sample).
	void applyDefaultFiltersMultiSample();
	/// Applies predefined default filters (somatic).
	void applyDefaultFiltersSomatic();
	/// Applies predefined default filters (carrier).
	void applyDefaultFiltersCarrier();

signals:
	/// Signal that is emitted when an annotation filter changes its checkbox state, or if the ROI changes, or if the gene changes
	void filtersChanged();
	/// Signal is emitted when the target region changes
	void targetRegionChanged();
	/// Signal that an import of phenotype data from GenLab was requested (this cannot be done inside the widget, because it knows nothing about the sample)
	void phenotypeDataImportRequested();

protected slots:
	void addRoi();
	void addRoiTemp();
	void removeRoi();
	void roiSelectionChanged(int index);
	void addRef();
	void removeRef();
	void referenceSampleChanged(int index);
	void geneChanged();
	void regionChanged();
	void phenotypesChanged();
	void filterColumnStateChanged();
	void showTargetRegionDetails();
	void updateGeneWarning();
	void editPhenotypes();
	void showPhenotypeContextMenu(QPoint pos);

private:
	/// Loads the reference file list of IGV
	void loadReferenceFiles();

    /// Resets the filters without blocking signals.
	void resetSignalsUnblocked(bool clear_roi, bool clear_off_target);

	Ui::FilterDockWidget ui_;
	bool ngsd_enabled;
	GeneSet last_genes_;
	QList<Phenotype> phenotypes_;
};

#endif // FILTERDOCKWIDGET_H
