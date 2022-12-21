#ifndef BURDENTESTWIDGET_H
#define BURDENTESTWIDGET_H

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
	void validateSamples();
	void updateSampleCounts();
private:
	Ui::BurdenTestWidget *ui_;
	QSet<int> case_samples_;
	QSet<int> control_samples_;

	QSet<int> loadSampleList(const QString& type, const QSet<int>& selected_ps_ids=QSet<int>());

	void performBurdenTest();
};

#endif // BURDENTESTWIDGET_H
