#ifndef VARIANTVALIDATIONWIDGET_H
#define VARIANTVALIDATIONWIDGET_H

#include <QWidget>
#include "ui_VariantValidationWidget.h"
#include "DelayedInitializationTimer.h"


class VariantValidationWidget
	: public QWidget
{
	Q_OBJECT

public:
	VariantValidationWidget(QWidget* parent = 0);

protected slots:
	void delayedInitialization();
	void updateTable();
	void edit();
	void remove();
	void openPrimerDesign();

private:
	Ui::VariantValidationWidget ui_;
	DelayedInitializationTimer init_timer_;
};

#endif // VARIANTVALIDATIONWIDGET_H
