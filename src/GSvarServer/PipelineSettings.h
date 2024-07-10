#ifndef PIPELINESETTINGS_H
#define PIPELINESETTINGS_H

#include <QVariant>

class PipelineSettings
{
public:
    static int integer(QString key);

    ///Read access for string settings. If the string starts with 'encrypted:' it is decrypted before returning the key. If optional and not present, an empty string is returned.
    static QString string(QString key, bool optional=false);

    ///Read access for string list settings. If optional and not present, an empty string list is returned.
    static QStringList stringList(QString key, bool optional=false);

    ///Read access for boolean settings. If optional and not present, 'false' is retured.
    static bool boolean(QString key, bool optional=false);

    ///Read access for map settings. If optional and not present, an empty map is returned.
    static QMap<QString,QVariant> map(QString key, bool optional=false);

    ///Returns all available keys.
    static QStringList allKeys();
    ///Returns if a key is present and the value is not empty.
    static bool contains(QString key);

protected:
    PipelineSettings();
    static QMap<QString, QVariant> parseIniFile(const QString &config_file);
    static QVariant getValueByKey(QString key);

private:
    static PipelineSettings& instance();
    QMap<QString, QVariant> pipeline_settings_;

};

#endif // PIPELINESETTINGS_H
