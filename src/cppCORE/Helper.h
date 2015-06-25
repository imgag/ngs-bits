#ifndef HELPER_H
#define HELPER_H

#include "cppCORE_global.h"
#include <QTime>
#include <QString>
#include <QFile>
#include <QStringList>
#include <QDebug>

///Auxilary helper functions class.
class CPPCORESHARED_EXPORT Helper
{
public:
	///Returns a random string.
	static QString randomString(int length, const QString& chars="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
	///Returns the elapsed time as a human-readable string.
	static QString elapsedTime(QTime elapsed);

	///Returns an opened file pointer, or throws an error if it cannot be opened. Make sure the file is deleted when it is not needed anymore, e.g. by assigning the returned pointer to a QScopedPointer<QFile> variable.
	static QFile* openFileForReading(QString file_name, bool stdin_if_empty=false);
	///Returns an opened file pointer, or throws an error if it cannot be opened. Make sure the file is deleted when it is not needed anymore, e.g. by assigning the returned pointer to a QScopedPointer<QFile> variable.
	static QFile* openFileForWriting(QString file_name, bool stdout_if_file_empty=false, bool append=false);

	///Loads a text file. If @p trim_lines is given, the lines are trimmed. If @p skip_header_char is given, header lines starting with that character are skipped. If @p skip_empty_lines is given, empty lines are skipped.
	static QStringList loadTextFile(QString file_name, bool trim_lines = false, QChar skip_header_char = QChar::Null, bool skip_empty_lines = false);
	///Stores a string list as a text file. To each line '\n' is appended as newline character.
	static void storeTextFile(QString file_name, const QStringList& lines, bool stdout_if_file_empty=false, bool append=false);

	///Returns a temporary file name. Make sure you delete the file when it is no longer needed to avoid name clashes!
	static QString tempFileName(QString extension, int length=16);

	///Find files recursively.
	static void findFiles(const QString& directory, const QString& pattern, QStringList& output, bool first_call=true);
	///Find folders recursively.
	static void findFolders(const QString& directory, const QString& pattern, QStringList& output, bool first_call=true);

	///Returns the Levenshtein-distance of two strings
	static int levenshtein(const QString& s1, const QString& s2);

	///Gets the user name of the current user from the environment variables.
	static QString userName();

	///Returns the current date and time in the given format. If the format is a empty string, the ISO format "yyyy-MM-ddTHH:mm:ss" is returned.
	static QString dateTime(QString format = "dd.MM.yyyy hh:mm:ss");

protected:
	///Constructor declared away.
	Helper();
};

#endif // HELPER_H
