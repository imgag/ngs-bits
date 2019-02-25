#ifndef SEQUENCINGRUNWIDGET_H
#define SEQUENCINGRUNWIDGET_H

#include <QWidget>
#include <QAction>
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

private:
	Ui::SequencingRunWidget* ui_;
	QString run_id_;
	NGSD db_;
};

#endif // SEQUENCINGRUNWIDGET_H
