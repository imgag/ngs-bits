#ifndef FILTERSTATE_H
#define FILTERSTATE_H

#include <QObject>

class FilterState : public QObject
{
    Q_OBJECT
public:
    explicit FilterState(QObject *parent = nullptr);

signals:
};

#endif // FILTERSTATE_H
