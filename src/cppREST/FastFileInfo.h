#ifndef FASTFILEINFO_H
#define FASTFILEINFO_H

#include "cppREST_global.h"
#include <QObject>
#include <QDateTime>

class CPPRESTSHARED_EXPORT FastFileInfo : public QObject
{
    Q_OBJECT

public:
    FastFileInfo(QString absolute_file_path);
    qint64 size();
    bool exists();
    QString absoluteFilePath();
    QString absolutePath();
    QString fileName();
    QDateTime lastModified();

private:
    QString absolute_file_path_;
    QString absolute_path_;
    QString filename_;
    qint64 size_;
    bool exists_;
    QDateTime last_modified_;
};

#endif // FASTFILEINFO_H
