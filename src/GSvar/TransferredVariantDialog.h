#ifndef TRANSFERREDVARIANTDIALOG_H
#define TRANSFERREDVARIANTDIALOG_H

#include <QWidget>
#include <QTableWidgetItem>
#include "NGSD.h"


namespace Ui {
class TransferredVariantDialog;
}

class TransferredVariantDialog
	: public QWidget
{
	Q_OBJECT

public:
	TransferredVariantDialog(int ps_id, QWidget *parent = nullptr);
	~TransferredVariantDialog();

protected slots:
	///Context menu that shall appear if right click on variant
	void showContextMenu(QPoint pos);
	//shows a detailed form view of the selected variant
	void showDetailedView(int id);
	//updates the status of a variant
	void changeVariantStatus(int id, const QString& status);

	void varDoubleClicked(QTableWidgetItem* item);

private:
	Ui::TransferredVariantDialog *ui_;
	NGSD db_;
	int ps_id_;
	QMap<int,QMap<QString,QString>> transferred_variant_data_;

	void updateTable();


};

#endif // TRANSFERREDVARIANTDIALOG_H
