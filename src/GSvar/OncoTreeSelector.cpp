#include "OncoTreeSelector.h"
#include <QInputDialog>
#include <QMessageBox>

OncoTreeSelector::OncoTreeSelector(QWidget* parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);

	initAutoCompletion();
}

void OncoTreeSelector::setLabel(QString label)
{
	ui_.label->setText(label);
}

void OncoTreeSelector::setSelection(QString oncotree_code)
{
	ui_.oncotree_code->setText(oncotree_code);
}

bool OncoTreeSelector::isValidSelection() const
{
	return  ui_.oncotree_code->isValidSelection();
}

void OncoTreeSelector::initAutoCompletion()
{
	QString query = "SELECT id, CONCAT(oncotree_code,' (',name,')') FROM oncotree_term";
	ui_.oncotree_code->fill(db_.createTable("oncotree_code", query));
}

QString OncoTreeSelector::oncotreeCodeId() const
{
	return  ui_.oncotree_code->getId();
}

QString OncoTreeSelector::oncotreeCode() const
{
	QString code_id = oncotreeCodeId();
	if (code_id=="") return "";

	return db_.getValue("SELECT oncotree_code FROM oncotree_term WHERE id='" + code_id+"'").toString();
}
