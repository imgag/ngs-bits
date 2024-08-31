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

private slots:
	void importColumns();
	void clearColumns();

	void addColumn(const QString& name);
	void addColumn(const QString& name, const ColumnInfo& info);
	void moveRowUp();
	void moveRowDown();
	void sizeChanged(int row, int col);
	void typeChanged(QString new_type);

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

	static QString variantTypeToKey(QString type);
};

#endif // COLUMNCONFIGWIDGET_H
