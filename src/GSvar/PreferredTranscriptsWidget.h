#ifndef PREFERREDTRANSCRIPTSWIDGET_H
#define PREFERREDTRANSCRIPTSWIDGET_H

#include <QWidget>
#include "ui_PreferredTranscriptsWidget.h"
#include "DelayedInitializationTimer.h"

class PreferredTranscriptsWidget
	: public QWidget
{
	Q_OBJECT

public:
	PreferredTranscriptsWidget(QWidget* parent = 0);

protected slots:
	void delayedInitialization();
	void updateTable();
	void addPreferredTranscript();
	void remove();
	void check();

private:
	Ui::PreferredTranscriptsWidget ui_;
	DelayedInitializationTimer init_timer_;
};

#endif // PREFERREDTRANSCRIPTSWIDGET_H
