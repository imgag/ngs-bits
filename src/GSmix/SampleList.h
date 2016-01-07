#ifndef SAMPLELIST_H
#define SAMPLELIST_H

#include <QTableWidget>
#include "ui_SampleList.h"

///Three-column sample list (name, i7-adapter, i5-adapter)
class SampleList
	: public QTableWidget
{
	Q_OBJECT

public:
	SampleList(QWidget *parent = 0);

	///Append read-only sample
	void appendSampleRO(QString name, QString a1="", QString a2="");
	///Append read-write sample
	void appendSampleRW(QString name, QString a1="", QString a2="");
	///Append multiple read-write samples
	void appendSamplesFromText(QString text);

protected:
	static QTableWidgetItem* createItem(bool read_only, QString text);
	void appendSample(bool read_only, QString name, QString a1="", QString a2="");
	void keyPressEvent(QKeyEvent* event) override;
	void dropEvent(QDropEvent * event) override;

private:
	Ui::SampleList ui;
};

#endif // SAMPLELIST_H
