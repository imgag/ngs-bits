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
	void disableGUI();

	///calculates score based on checkboxes of widget.
	void predict();

	///returns parameters selected by user
	SomaticViccData getParameters();

private slots:
	///disables selection option depending on other selected properties
	void disableUnapplicableParameters();
};

#endif // SOMATICVARIANTINTERPRETERWIDGET_H
