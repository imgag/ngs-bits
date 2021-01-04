#ifndef SOMATICVARIANTINTERPRETERWIDGET_H
#define SOMATICVARIANTINTERPRETERWIDGET_H

#include <QWidget>
#include "VariantList.h"
#include "SomaticVariantInterpreter.h"

namespace Ui
{
class SomaticVariantInterpreterWidget;
}

class SomaticVariantInterpreterWidget : public QWidget
{
	Q_OBJECT
public:
	explicit SomaticVariantInterpreterWidget(const Variant& var, const VariantList& vl,  QWidget *parent = nullptr);
	~SomaticVariantInterpreterWidget();

private:
	Ui::SomaticVariantInterpreterWidget* ui_;

	const Variant& snv_;
	const VariantList& vl_;

	///sets selection for a group of radiobuttons by name
	void setSelection(QString name, SomaticViccData::state vicc_state);

	///returns state of a group of radiobuttons
	SomaticViccData::state getSelection(QString name);

	///enables/disables a group of radiobuttons by name
	void setSelectionEnabled(QString name, bool state);

signals:

public slots:
	void disableNGSD();

	///calculates score based on checkboxes of widget.
	void predict();

	///returns parameters selected by user
	SomaticViccData getParameters();

	///preselects radio buttons depending on input variant annotation
	void preselectFromInputAnno();
	///preselects radio buttons based on VICC interpretation stored in NGSD
	bool preselectFromNGSD();

private slots:
	///disables selection option depending on other selected properties
	void disableUnapplicableParameters();
	///stores/updates VICC interpretation in NGSD
	void storeInNGSD();
	///preselects radiobutton according parameters in input SomaticViccData
	void preselect(const SomaticViccData& data);
	///sets qt labels for metadata stored in NGSD (created by, comment...)
	void setNGSDMetaData();
};

#endif // SOMATICVARIANTINTERPRETERWIDGET_H
