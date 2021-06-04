#ifndef STRINGDBPARSER_H
#define STRINGDBPARSER_H

#include "cppNGS_global.h"
#include "Exceptions.h"
#include "Graph.h"
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QFile>
#include <QStringList>

class CPPNGSSHARED_EXPORT StringDbParser
{
    public:
        StringDbParser(const QByteArray& string_db_file, const QByteArray& alias_file, float threshold = 0.4);

        Graph<int, int>& interactionNetwork();

    private:
        // lower threshold for combined score of putative interactions; ranges from 0 to 1000
        int threshold_;

        QByteArray string_db_file_;
        QByteArray alias_file_;

        QHash<QString, QString> hgnc_translator_;
        Graph<int, int> interaction_network_;

        void parseAliasFile();
        void parseStringDbFile();
};

#endif // STRINGDBPARSER_H
