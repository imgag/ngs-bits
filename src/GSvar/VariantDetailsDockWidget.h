#ifndef VARIANTDETAILSDOCKWIDGET_H
#define VARIANTDETAILSDOCKWIDGET_H

#include <QWidget>
#include <QLabel>
#include "VariantList.h"

namespace Ui {
class VariantDetailsDockWidget;
}

//Variant details widget
class VariantDetailsDockWidget
	: public QWidget
{
	Q_OBJECT

public:
	explicit VariantDetailsDockWidget(QWidget* parent);
	~VariantDetailsDockWidget();

	//Set preferred transcripts for highlighting
	void setPreferredTranscripts(const QMap<QString, QStringList>& pt);

	//Sets tooltips of labels
	void setLabelTooltips(const VariantList& vl);

	//Updates the widget to a new variant.
	void updateVariant(const VariantList& vl, int index);
	//Clears the widget (no variant selected).
	void clear();

	//Database annotation datastructure
	struct DBEntry
	{
		QString id;
		QString details;
	};
	//Parse database entries (OMIM, ClinVar, HGMD, ...) to a map (ID=>details).
	static QList<DBEntry> parseDB(QString anno, QString sep="];");

signals:
	void jumbToRegion(QString region);
	void editVariantClassification();
	void editVariantValidation();
	void editVariantComment();
	void showVariantSampleOverview();

private slots:
	void nextTanscript();
	void previousTanscript();
	void variantClicked(QString link);
	void gnomadClicked(QString link);
	void editClassification();
	void editValidation();
	void editComment();
	void variantSampleOverview();

private:
	enum Color
	{
		NONE,
		RED,
		ORANGE,
		GREEN
	};

	//Returns the annotation.
	void setAnnotation(QLabel* label, const VariantList& vl, int index, QString name);
	//Returns the maximum allele frequency of the variant.
	double maxAlleleFrequency(const VariantList& vl, int index) const;
	//Convert color to string.
	static QString colorToString(Color color);
	//Format clickable link for a label.
	static QString formatLink(QString text, QString url, Color bgcolor = NONE);
	//Format colored text for a label.
	static QString formatText(QString text, Color bgcolor);

	//Returns 'nobr' paragraph start for tooltips
	static QString nobr();

	//Init transcript details
	void initTranscriptDetails(const VariantList& vl, int index);
	//Set transcript details
	void setTranscript(int index);
	//current transcript
	int trans_curr;
	//Transcript data
	QList<VariantTranscript> trans_data;

	//GUI
	Ui::VariantDetailsDockWidget *ui;
	//Preferred transcript list per gene
	QMap<QString, QStringList> preferred_transcripts;
};

#endif // VARIANTDETAILSDOCKWIDGET_H
