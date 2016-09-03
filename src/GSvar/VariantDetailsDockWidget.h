#ifndef VARIANTDETAILSDOCKWIDGET_H
#define VARIANTDETAILSDOCKWIDGET_H

#include <QDockWidget>
#include <QLabel>
#include "VariantList.h"

namespace Ui {
class VariantDetailsDockWidget;
}

//Variant details widget
class VariantDetailsDockWidget
	: public QDockWidget
{
	Q_OBJECT

public:
	explicit VariantDetailsDockWidget(QWidget* parent = 0);
	~VariantDetailsDockWidget();

	//Sets preferred transcripts
	void setPreferredTranscripts(QMap<QString, QString> data);

	//Updates the widget to a new variant.
	void updateVariant(const VariantList& vl, int index);
	//Clears the widget (no variant selected).
	void clear();

signals:
	void jumbToRegion(QString region);

private slots:
	void nextTanscript();
	void previousTanscript();
	void variantClicked(QString link);
    void exacClicked(QString link);

private:
	//Database annotation datastructure
	struct DBEntry
	{
		QString id;
		QString details;
	};
	enum Color
	{
		NONE,
		RED,
		ORANGE,
		GREEN
	};

	//Returns the annotation.
	void setAnnotation(QLabel* label, const VariantList& vl, int index, QString name);
	//Convert color to string.
	static QString colorToString(Color color);
	//Format clickable link for a label.
	static QString formatLink(QString text, QString url, Color bgcolor = NONE);
	//Format colored text for a label.
	static QString formatText(QString text, Color bgcolor);
	//Parse database entries (OMIM, ClinVar, HGMD, ...) to a map (ID=>details).
	static QList<DBEntry> parseDB(QString anno);
	//Returns 'nobr' paragraph start for tooltips
	static QString nobr();
    //Return reference bases from hg19 (1-based closed range).
    QString getRef(const Chromosome& chr, int start, int end);

	//Init transcript details
	void initTranscriptDetails(const VariantList& vl, int index);
	//Set transcript details
	void setTranscript(int index);
	//current transcript
	int trans_curr;
	//Transcript data
	QList<QStringList> trans_data;

	//GUI
	Ui::VariantDetailsDockWidget *ui;
	//Preferred transcript list per gene
	QMap<QString, QString> preferred_transcripts;
	//Flag that caches if NGSD-support is enabled (from settings)
	bool ngsd_enabled;
};

#endif // VARIANTDETAILSDOCKWIDGET_H
