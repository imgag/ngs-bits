#ifndef FILTERCASCADEWIDGET_H
#define FILTERCASCADEWIDGET_H

#include <QWidget>
#include "ui_FilterCascadeWidget.h"
#include "FilterCascade.h"

///Widget for managing filter cascades
class FilterCascadeWidget
	: public QWidget
{
	Q_OBJECT

public:
	///Constructor
	FilterCascadeWidget(QWidget* parent = 0);
	///Sets the filter subject
	void setSubject(VariantType subject);
	///Sets the valid entries for small variant list 'filter' column.
	void setValidFilterEntries(const QStringList& filter_entries);

	///Returns the filters
	const FilterCascade& filters() const;
	///Sets the filters.
	void setFilters(const FilterCascade& filters);
	///Clears filters
	void clear();

	///Visually marks filters that failed (call this after applying the filters).
	void markFailedFilters();

signals:
	///Signal that is emitted when the filter cascade has changed.
	void filterCascadeChanged();

protected:
	///Sets the focus to the given indes (and handles border cases)
	void focusFilter(int index);
	/// Returns the row index of the currently selected filter, or -1 if none is selected;
	int currentFilterIndex() const;

protected slots:
	void updateGUI();
	void updateButtons();
	void addFilter();
	void editSelectedFilter();
	void deleteSelectedFilter();
	void moveUpSelectedFilter();
	void moveDownSelectedFilter();
	void toggleSelectedFilter(QListWidgetItem* item);

private:
	Ui::FilterCascadeWidget ui_;
	FilterCascade filters_;
	VariantType subject_;
	QStringList valid_filter_entries_;
};

#endif // FILTERCASCADEWIDGET_H
