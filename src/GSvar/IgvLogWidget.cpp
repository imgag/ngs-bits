#include "IgvLogWidget.h"
#include "IgvSessionManager.h"
#include "GUIHelper.h"

IgvLogWidget::IgvLogWidget(QWidget* parent)
    : QWidget(parent)
    , ui_()
{
    ui_.setupUi(this);
	ui_.table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    for (int i = 0; i < IgvSessionManager::count(); i++)
    {
        ui_.igv_instance->addItem(IgvSessionManager::get(i).getName(), i);
    }

    connect(ui_.igv_instance, SIGNAL(currentIndexChanged(int)), this, SLOT(switchCurrentSession(int)));
	if (IgvSessionManager::count()>0) switchCurrentSession(0);
}

void IgvLogWidget::switchCurrentSession(int index)
{
	IGVSession& session = IgvSessionManager::get(index);
	updateTable(session.getName(), session.getCommands());
	connect(&session, SIGNAL(historyUpdated(QString, QList<IGVCommand>)), this, SLOT(updateTable(QString, QList<IGVCommand>)));
	connect(&session, SIGNAL(initializationStatusChanged(bool)), this, SLOT(updateInitStatus(bool)));
	ui_.initialized->setText(session.isInitialized() ? "yes" : "no");
	ui_.port->setText(QString::number(session.getPort()));
}

void IgvLogWidget::updateTable(QString name, QList<IGVCommand> commands)
{
	//set instance in combobox
	ui_.igv_instance->blockSignals(true);
	ui_.igv_instance->setCurrentText(name);
	ui_.igv_instance->blockSignals(false);

	//update table
	ui_.table->setRowCount(0);
	int row = 0;
	foreach(const IGVCommand& command, commands)
    {
		if (command.command.startsWith("SetAccessToken ")) continue;

		ui_.table->setRowCount(row+1);
		ui_.table->setItem(row, 0, GUIHelper::createTableItem(command.command));
		QTableWidgetItem* item = GUIHelper::createTableItem(IGVSession::statusToString(command.status));
		item->setBackgroundColor(IGVSession::statusToColor(command.status));
		ui_.table->setItem(row, 1, item);
		ui_.table->setItem(row, 2, GUIHelper::createTableItem(QString::number(command.execution_time_sec, 'f', 2)));
		ui_.table->setItem(row, 3, GUIHelper::createTableItem(command.answer));
		++row;
	}

	//scroll to last entry
	ui_.table->scrollToBottom();
}

void IgvLogWidget::updateInitStatus(bool is_inizialized)
{
	ui_.initialized->setText(is_inizialized ? "yes" : "no");
}
