#ifndef COLUMNCONFIGWIDGET_H
#define COLUMNCONFIGWIDGET_H

#include <QWidget>
#include "ui_ColumnConfigWidget.h"
#include "ColumnConfig.h"


class ColumnConfigWidget
	: public QWidget
{
	Q_OBJECT

public:
	ColumnConfigWidget(QWidget* parent);

public slots:
	//stores the column configurations into the settings
	void store();
	void switchToType(QString new_type);

private slots:
	void addColumnsFromSample();
	void clearColumns();

	void moveRowUp();
	void moveRowDown();
	void deleteSelectedColumn();
	void sizeChanged(int row, int col);

	void exportCurrent();
	void exportAll();
	void import();

private:
	Ui::ColumnConfigWidget ui_;
	QString title_;
	QString current_type_;
	QHash<QString, ColumnConfig> configs_;

	//write back config of current type to configs_
	void writeBackCurrentConfig();
	//load config for the given tyoe from configs_
	void loadConfig(QString type);

	void swapRows(int from, int to);
	void addColumn(const QString& name);
	void addColumn(const QString& name, const ColumnInfo& info);

	static QString variantTypeToKey(QString type);
};

#endif // COLUMNCONFIGWIDGET_H
