#include "IgvLogWidget.h"
#include "IgvSessionManager.h"
#include "GUIHelper.h"

IgvLogWidget::IgvLogWidget(QWidget* parent)
    : QWidget(parent)
    , ui_()
{
    ui_.setupUi(this);

    for (int i = 0; i < IgvSessionManager::count(); i++)
    {
        ui_.igv_instance->addItem(IgvSessionManager::get(i).getName(), i);
    }

    connect(ui_.igv_instance, SIGNAL(currentIndexChanged(int)), this, SLOT(switchCurrentSession(int)));
    if (IgvSessionManager::count()>0) switchCurrentSession(0);
}

void IgvLogWidget::switchCurrentSession(int index)
{
    updateTable(IgvSessionManager::get(index).getHistory());
	connect(&(IgvSessionManager::get(index)), SIGNAL(historyUpdated(QList<IGVCommand>)), this, SLOT(updateTable(QList<IGVCommand>)));
}

void IgvLogWidget::updateTable(const QList<IGVCommand>& updated_history)
{
    ui_.table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui_.table->setRowCount(updated_history.count());
    int i = 0;
	foreach(const IGVCommand& command, updated_history)
    {
		ui_.table->setItem(i, 0, GUIHelper::createTableItem(command.command));
		QTableWidgetItem* item = GUIHelper::createTableItem(IGVSession::statusToString(command.status));
		item->setBackgroundColor(IGVSession::statusToColor(command.status));
		ui_.table->setItem(i, 1, item);
		ui_.table->setItem(i, 2, GUIHelper::createTableItem(QString::number(command.execution_time_sec, 'f', 2)));
		ui_.table->setItem(i, 3, GUIHelper::createTableItem(command.answer));
		++i;
    }
}
