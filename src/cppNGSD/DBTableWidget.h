#ifndef DBTABLEWIDGET_H
#define DBTABLEWIDGET_H

#include <QTableWidget>

#include "cppNGSD_global.h"
#include "DBTable.h"

//Visualization of a database table.
class CPPNGSDSHARED_EXPORT DBTableWidget
	: public QTableWidget
{
	Q_OBJECT

public:
	DBTableWidget(QWidget* parent);

	void setData(const DBTable& tableName);
	void setColumnTooltips(int c, const QStringList& tooltips);
	QSet<int> selectedRows() const;
	const QString& getId(int r) const;
	const QString& tableName();

protected:
	QTableWidgetItem* createItem(const QString& text, int alignment = Qt::AlignVCenter|Qt::AlignLeft);	
	void keyPressEvent(QKeyEvent* event) override;

protected slots:
	void copyToClipboard();

protected:
	QString table_;
	QStringList ids_;

};

#endif // DBTABLEWIDGET_H
