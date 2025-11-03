#ifndef VARIANTDETAILSDOCKWIDGET_H
#define VARIANTDETAILSDOCKWIDGET_H

#include <QWidget>
#include <QLabel>
#include "VariantList.h"
#include "KeyValuePair.h"

namespace Ui {
class VariantDetailsDockWidget;
}

//Variant details widget
class VariantDetailsDockWidget
	: public QWidget
{
	Q_OBJECT

public:
	VariantDetailsDockWidget(QWidget* parent);
	~VariantDetailsDockWidget();

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

		//Splits the details by name.
		QList<KeyValuePair> splitByName() const;
	};
	//Parse database entries (OMIM, ClinVar, HGMD, ...) to a map (ID=>details).
	static QList<DBEntry> parseDB(QString anno, char sep);

	//Opens a overview table with details and link (is url_prefix is set)
	static void showOverviewTable(QString title, QString text, char sep, QByteArray url_prefix = "");

private slots:
	void nextTanscript();
	void previousTanscript();
	void variantClicked(QString link);
	void gnomadClicked(QString variant_string);
	void transcriptClicked(QString link);
	void pubmedClicked(QString link);
	void variantButtonClicked();
	void gnomadContextMenu(QPoint pos);
	void spliceaiContextMenu(QPoint pos);

private:
	enum Color
	{
		NONE,
		RED,
		ORANGE,
		YELLOW,
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

	//variant string representation (empty if no variant selected)
	QString variant_str;
	//current transcript
	int trans_curr;
	//Transcript data
	QList<VariantTranscript> trans_data;

	//

	//GUI
	Ui::VariantDetailsDockWidget* ui;
};

#endif // VARIANTDETAILSDOCKWIDGET_H
