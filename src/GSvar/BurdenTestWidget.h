#ifndef BURDENTESTWIDGET_H
#define BURDENTESTWIDGET_H

#include <NGSD.h>
#include <QSet>
#include <QWidget>

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
	void updateGeneSelectionMenu();
	void performBurdenTest();

private:
	Ui::BurdenTestWidget *ui_;
	QSet<int> case_samples_;
	QSet<int> control_samples_;
	GeneSet selected_genes_;
	NGSD db_;
	SqlQuery variant_query_;
	bool test_running = false;
	void prepareSqlQuery(int max_ngsd, double max_gnomad_af, const QStringList& ps_ids, const QStringList& impacts);

	QSet<int> loadSampleList(const QString& type, const QSet<int>& selected_ps_ids=QSet<int>());

	QSet<int> getVariantsForRegion(const BedLine& region, const QString& gene_symbol, const QStringList& impacts, const QSet<int>& valid_variant_ids);


};

#endif // BURDENTESTWIDGET_H
