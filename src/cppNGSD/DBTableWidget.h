#ifndef DBTABLEWIDGET_H
#define DBTABLEWIDGET_H

#include <QTableWidget>

#include "cppNGSD_global.h"
#include "DBTable.h"

//Visualization of a database table.
class CPPNGSDSHARED_EXPORT DBTableWidget
	: public QTableWidget
{
public:
	DBTableWidget(QWidget* parent);

	void setData(const DBTable& table);

	void setColumnTooltips(int c, const QStringList& tooltips);

protected:
	QTableWidgetItem* createItem(const QString& text, int alignment = Qt::AlignVCenter|Qt::AlignLeft);

protected slots:
	void copyToClipboard();

};

#endif // DBTABLEWIDGET_H
