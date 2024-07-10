#include "PipelineSettings.h"
#include "Exceptions.h"
#include "Settings.h"
#include <QTextStream>
#include <QFile>
#include "Log.h"

int PipelineSettings::integer(QString key)
{
    QVariant raw_value = getValueByKey(key);
    if (raw_value.canConvert(QMetaType::Int))
    {
        return raw_value.toInt();
    }

    THROW(ArgumentException, "Could not convert '"+key+"' to integer");
}

QString PipelineSettings::string(QString key, bool optional)
{
    if (optional && !contains(key)) return "";

    QVariant raw_value = getValueByKey(key);
    if (raw_value.canConvert(QMetaType::QString))
    {
        return raw_value.toString();
    }

    THROW(ArgumentException, "Could not convert '"+key+"' to string");
}

QStringList PipelineSettings::stringList(QString key, bool optional)
{
    if (optional && !contains(key)) return QStringList();

    QVariant raw_value = getValueByKey(key);
    if (raw_value.canConvert(QMetaType::QStringList))
    {
        return raw_value.toStringList();
    }

    THROW(ArgumentException, "Could not convert '"+key+"' to string list");
}

bool PipelineSettings::boolean(QString key, bool optional)
{
    if (optional && !contains(key)) return false;

    QVariant raw_value = getValueByKey(key);
    if (raw_value.canConvert(QMetaType::Bool))
    {
        return raw_value.toBool();
    }

    THROW(ArgumentException, "Could not convert '"+key+"' to boolean");
}

QMap<QString, QVariant> PipelineSettings::map(QString key, bool optional)
{
    if (optional && !contains(key)) QMap<QString, QVariant>();

    QVariant raw_value = getValueByKey(key);

    return raw_value.toMap();
}

QStringList PipelineSettings::allKeys()
{
    return instance().pipeline_settings_.keys();
}


bool PipelineSettings::contains(QString key)
{
    return instance().pipeline_settings_.contains(key);
}

PipelineSettings::PipelineSettings()
    :pipeline_settings_()
{
    QString file_name = Settings::string("megsap_settings_ini", true);
    if (!file_name.isEmpty())
    {
        pipeline_settings_ = parseIniFile(Settings::string("megsap_settings_ini", true));
    }
}

QMap<QString, QVariant> PipelineSettings::parseIniFile(const QString &config_file)
{
    Log::info("Reading the config file: " + config_file);
    QMap<QString, QVariant> settings;

    QFile file(config_file);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        THROW(FileAccessException, "Could not open settings file: " + config_file);
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // Skip comments, empty lines, and section headers
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#') || (line.startsWith('[') && line.endsWith(']')))
        {
            continue;
        }

        // Key-value pair
        QStringList keyValue = line.split('=', QString::SkipEmptyParts);
        if (keyValue.size() == 2)
        {
            QString key = keyValue.at(0).trimmed();
            QString value = keyValue.at(1).trimmed();

            // Handle array keys with their own keys
            bool has_array = false;
            QString array_name;
            QString array_key;

            if (key.endsWith("]"))
            {
                int key_pos = key.indexOf("[");
                if (key_pos>-1)
                {
                    array_key = key.mid(key_pos+1, key.length()-key_pos-2);
                    array_key = array_key.replace("\"", "");
                    array_key = array_key.replace("\'", "");
                    array_name = key.mid(0, key_pos);

                    has_array = true;
                }
            }

            if (has_array)
            {
                QMap<QString, QVariant> arrayValues = settings.value(array_name).toMap();
                arrayValues.insert(array_key, value);
                settings[array_name] = arrayValues;
            } else
            {
                settings[key] = value;
            }
        }
    }

    file.close();
    return settings;
}

QVariant PipelineSettings::getValueByKey(QString key)
{
    if (instance().pipeline_settings_.contains(key))
    {
        return instance().pipeline_settings_.value(key);
    }

    THROW(ProgrammingException, "Requested key '" + key + "' not found in settings!");
}

PipelineSettings& PipelineSettings::instance()
{
    static PipelineSettings pipeline_settings;
    return pipeline_settings;
}
