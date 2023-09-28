#include "IgvLogWidget.h"
#include "IgvSessionManager.h"
#include "GUIHelper.h"

IgvLogWidget::IgvLogWidget(QWidget* parent, int index)
    : QWidget(parent)
    , ui_()
{
    ui_.setupUi(this);
    updateTable(IgvSessionManager::getCommandExecutor(index).data()->getHistory());
    connect(IgvSessionManager::getCommandExecutor(index).data(), SIGNAL(historyUpdated(QStringList)), this, SLOT(updateTable(QStringList)));
}

void IgvLogWidget::updateTable(QStringList updated_history)
{
    ui_.table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui_.table->setRowCount(updated_history.count());
    int i = 0;
    foreach(QString item, updated_history)
    {
        QList<QString> fragments = item.split("\t");
        if (fragments.count()==3)
        {
            ui_.table->setItem(i, 0, GUIHelper::createTableItem(fragments[0]));
            ui_.table->setItem(i, 1, GUIHelper::createTableItem(fragments[1]));
            ui_.table->setItem(i, 2, GUIHelper::createTableItem(fragments[2]));
        }
        i++;
    }
}
