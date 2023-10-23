#ifndef BURDENTESTWIDGET_H
#define BURDENTESTWIDGET_H

#include <NGSD.h>
#include <QSet>
#include <QTableWidget>
#include <QTextEdit>
#include <QWidget>

enum class Inheritance
{
	dominant,
	de_novo,
	recessive
};

namespace Ui {
class BurdenTestWidget;
}

class BurdenTestWidget : public QWidget
{
	Q_OBJECT

public:
	explicit BurdenTestWidget(QWidget *parent = 0);
	~BurdenTestWidget();
private slots:
	void loadCaseSamples();
	void loadControlSamples();
	void loadGeneList();
	void loadExcludedRegions();
	void clearExcludedRegions();
	void loadBedFile();
	void validateInputData();
	void updateSampleCounts();
	void updateGeneCounts();
	void updateExcludedRegions();
//	void updateGeneSelectionMenu();
	void performBurdenTest();
	void copyToClipboard();
	void copyWarningsToClipboard();
	void validateCCRGenes();
	void initCCR();


private:
	Ui::BurdenTestWidget *ui_;
	QSet<int> case_samples_;
	QSet<int> control_samples_;
	bool cases_initialized_ = false;
	bool controls_initialized_ = false;
	GeneSet selected_genes_;
	GeneSet ccr_genes_;
	bool gene_set_initialized_ = false;
	NGSD db_;
	BedFile excluded_regions_;
	QMap<QByteArray,BedFile> ccr80_region_;
	bool test_running = false;
	QTextEdit* te_excluded_regions_;
	QStringList excluded_regions_file_names;
	QTableWidget* tw_warnings_;
	QStringList createChromosomeQueryList(int max_ngsd, double max_gnomad_af, const BedFile& regions, const QStringList& impacts, bool predict_pathogenic, bool include_mosaic);
	int countOccurences(const QSet<int>& variant_ids, const QSet<int>& ps_ids, const QMap<int, QSet<int> >& detected_variants, Inheritance inheritance, QStringList& ps_names);
	QSet<int> loadSampleList(const QString& type, const QSet<int>& selected_ps_ids=QSet<int>());
	QSet<int> getVariantsForRegion(int max_ngsd, double max_gnomad_af, const BedFile& regions, const QString& gene_symbol, const QStringList& impacts, bool predict_pathogenic);
	QString createGeneQuery(int max_ngsd, double max_gnomad_af, const BedFile& regions, const QStringList& impacts, bool predict_pathogenic);
	int getNewestProcessedSample(const QSet<int>& ps_list);
	int getNewestSample(const QSet<int>& s_list);
	QSet<int> removeDiseaseStatus(const QSet<int>& s_list, const QStringList& stati_to_remove);



};

#endif // BURDENTESTWIDGET_H
