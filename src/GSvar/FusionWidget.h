#ifndef FUSIONWIDGET_H
#define FUSIONWIDGET_H

#include <QWidget>

namespace Ui {
class FusionWidget;
}

class FusionWidget : public QWidget
{
	Q_OBJECT

public:
	explicit FusionWidget(const QString& filename, QWidget *parent = 0);
	~FusionWidget();

private:
	void loadFusionData();


	QString filename_;
	Ui::FusionWidget *ui_;

	//table info
	QStringList column_names_;
	QVector<bool> numeric_columns_;
};

#endif // FUSIONWIDGET_H
