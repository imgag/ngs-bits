#ifndef SEQUENCINGRUNWIDGET_H
#define SEQUENCINGRUNWIDGET_H

#include <QWidget>
#include <QAction>
#include <QTableWidgetItem>
#include "NGSD.h"

namespace Ui {
class SequencingRunWidget;
}

class SequencingRunWidget : public QWidget
{
	Q_OBJECT

public:
	SequencingRunWidget(QWidget* parent, QString run_id);
	~SequencingRunWidget();

signals:
	void openProcessedSampleTab(QString ps_name);

protected slots:
	void updateGUI();
	void openSelectedSamples();
	void updateReadQualityTable();
	void updateRunSampleTable();
	void setQuality();

private:
	QTableWidgetItem* createItem(const QString& text, bool highlight = false, int alignment = Qt::AlignVCenter|Qt::AlignRight);

	Ui::SequencingRunWidget* ui_;
	QString run_id_;
	NGSD db_;
};

#endif // SEQUENCINGRUNWIDGET_H
