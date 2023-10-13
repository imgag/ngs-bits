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
    connect(&(IgvSessionManager::get(index)), SIGNAL(historyUpdated(QStringList)), this, SLOT(updateTable(QStringList)));
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
