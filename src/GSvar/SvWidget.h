#ifndef SVWIDGET_H
#define SVWIDGET_H

#include <QWidget>
#include "SvList.h"

namespace Ui {
	class SvWidget;
}


class SvWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SvWidget(QString file_name, QWidget *parent = 0);

private:
	Ui::SvWidget *ui;

	SvList svs_;

	void addInfoLine(QString text);

	void loadSVs(QString file_name);

signals:
	close();

public slots:

private slots:
	void copyToClipboard();

	///update SV table if filter for types was changed
	void filtersChanged();
};

#endif // SVWIDGET_H
