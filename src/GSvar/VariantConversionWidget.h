#ifndef VARIANTCONVERSIONWIDGET_H
#define VARIANTCONVERSIONWIDGET_H

#include <QWidget>
#include "ui_VariantConversionWidget.h"

class VariantConversionWidget
	: public QWidget
{
	Q_OBJECT

public:
	enum ConversionMode
	{
		VCF_TO_GSVAR,
		HGVSC_TO_GSVAR,
		NONE
	};
	VariantConversionWidget(QWidget* parent = 0);
	void setMode(ConversionMode mode);

private slots:
	void loadInputFromFile();
	void convert();

private:
	Ui::VariantConversionWidget ui_;
	ConversionMode mode_;
};

#endif // VARIANTCONVERSIONWIDGET_H
