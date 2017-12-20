#ifndef RohWidget_H
#define RohWidget_H

#include <QWidget>
#include <QTableWidgetItem>
#include "RohList.h"
#include "BedFile.h"
#include "GeneSet.h"
#include "FilterDockWidget.h"
#include "VariantList.h"

namespace Ui {
class RohWidget;
}

///Widget for visualization and filtering of ROHs.
class RohWidget
	: public QWidget
{
	Q_OBJECT

public:
	explicit RohWidget(QString filename, FilterDockWidget* filter_widget, QWidget *parent = 0);
	~RohWidget();

signals:
	void openRegionInIGV(QString region);

private slots:
	void rohDoubleClicked(QTableWidgetItem* item);
	void filtersChanged();
	void variantFiltersChanged();
	void copyToClipboard();
	void annotationFilterColumnChanged();
	void annotationFilterOperationChanged();
	void showContextMenu(QPoint p);

private:
	void loadROHs(QString filename);
	void disableGUI();
	void addInfoLine(QString text);
	void updateStatus(int shown, double size);

	FilterDockWidget* var_filters;
	Ui::RohWidget *ui;
	RohList rohs;
	QMap<QString, int> annotation_col_indices_;
};

#endif // RohWidget_H
