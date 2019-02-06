#ifndef SEQUENCINGRUNWIDGET_H
#define SEQUENCINGRUNWIDGET_H

#include <QWidget>
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

protected slots:
	void updateGUI();

private:
	Ui::SequencingRunWidget* ui_;
	QString run_id_;
	NGSD db_;
};

#endif // SEQUENCINGRUNWIDGET_H
