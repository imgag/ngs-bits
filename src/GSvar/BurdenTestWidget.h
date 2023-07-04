#ifndef BURDENTESTWIDGET_H
#define BURDENTESTWIDGET_H

#include <NGSD.h>
#include <QSet>
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
	void validateInputData();
	void updateSampleCounts();
	void updateGeneCounts();
//	void updateGeneSelectionMenu();
	void performBurdenTest();
	void copyToClipboard();

private:
	Ui::BurdenTestWidget *ui_;
	QSet<int> case_samples_;
	QSet<int> control_samples_;
	bool cases_initialized_ = false;
	bool controls_initialized_ = false;
	GeneSet selected_genes_;
	bool gene_set_initialized_ = false;
	NGSD db_;
//	SqlQuery variant_query_;
	bool test_running = false;
//	void prepareSqlQuery(int max_ngsd, double max_gnomad_af, const QStringList& impacts, bool predict_pathogenic);
	QStringList createChromosomeQueryList(int max_ngsd, double max_gnomad_af, const BedFile& regions, const QStringList& impacts, bool predict_pathogenic, bool include_mosaic);
	int countOccurences(const QSet<int>& variant_ids, const QSet<int>& ps_ids, const QMap<int, QSet<int> >& detected_variants, Inheritance inheritance, QStringList& ps_names);

	QSet<int> loadSampleList(const QString& type, const QSet<int>& selected_ps_ids=QSet<int>());

//	QSet<int> getVariantsForRegion(const BedLine& region, const QString& gene_symbol, const QStringList& impacts, const QSet<int>& valid_variant_ids, bool predict_pathogenic);
	QSet<int> getVariantsForRegion(int max_ngsd, double max_gnomad_af, const BedFile& regions, const QString& gene_symbol, const QStringList& impacts, bool predict_pathogenic);
	QString createGeneQuery(int max_ngsd, double max_gnomad_af, const BedFile& regions, const QStringList& impacts, bool predict_pathogenic);


};

#endif // BURDENTESTWIDGET_H
