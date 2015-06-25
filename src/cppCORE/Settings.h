#ifndef SETTINGS_H
#define SETTINGS_H

#include "cppCORE_global.h"
#include <QSettings>
#include <QStringList>
#include <QCoreApplication>


///Application settings handler
class CPPCORESHARED_EXPORT Settings
{
  public:
    ///read access for integer settings
    static int integer(QString key, int default_value = -1);
    ///write access for integer settings
	static void setInteger(QString key, int valueWithFallback);

    ///read access for string settings
    static QString string(QString key, QString default_value = "");
    ///write access for string settings
	static void setString(QString key, QString valueWithFallback);

    ///read access for string settings
    static QStringList stringList(QString key, QStringList default_value = QStringList());
    ///write access for string settings
	static void setStringList(QString key, QStringList valueWithFallback);

    ///read access for integer settings
    static bool boolean(QString key, bool default_value = false);
    ///write access for integer settings
	static void setBoolean(QString key, bool valueWithFallback);

	///read access for map settings
    static QMap<QString,QVariant> map(QString key, QMap<QString,QVariant> default_value = QMap<QString,QVariant>());
	///write access for map settings
	static void setMap(QString key, QMap<QString,QVariant> valueWithFallback);

    ///Returns the path associated with this key. The path always contains a directroy separator. If the path is not defined, the home folder is returned.
	static QString path(QString key, QString default_value = QCoreApplication::applicationDirPath());
    ///Sets the path associated with this key. If the path contains a file name, it is stripped off.
    static void setPath(QString key, QString path);

    ///Removes all settings
    static void clear();
    ///Removes a key
    static void remove(QString key);

	///Returns all available keys
	static QStringList allKeys();
	///Returns the settings file name
	static QString fileName();

  protected:
	///Default constructor "declared away"
    Settings();
	///Retreives a value from default INI file "[appname].ini" or from fallback INI file "settings.ini"
	static QVariant valueWithFallback(const QString& key, const QVariant& defaultValue = QVariant());

    ///returns the settings object
	static QSettings& settings();
	///returns the fallback settings object
	static QSettings& settingsFallback();
};

#endif //SETTINGS_H
