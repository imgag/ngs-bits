#ifndef STRINGDBPARSER_H
#define STRINGDBPARSER_H

#include "cppNGS_global.h"
#include "Exceptions.h"
#include "Graph.h"
#include <QString>
#include <QHash>
#include <QFile>
#include <QStringList>
#include <cmath>

template <typename NodeType, typename EdgeType>
class CPPNGSSHARED_EXPORT StringDbParser
{
    public:
        StringDbParser(const QString& string_db_file, const QString& alias_file, double threshold);
        StringDbParser(const QString& string_db_file, const QString& alias_file);

        Graph<NodeType, EdgeType>& interactionNetwork();

    private:
        // lower threshold for combined score of putative interactions; ranges from 0 to 1000
        int threshold_;

        QString string_db_file_;
        QString alias_file_;

        QHash<QString, QString> hgnc_translator_;
        Graph<NodeType, EdgeType> interaction_network_;

        void parseAliasFile();
        void parseStringDbFile();
};

/**
 * @brief StringDbParser::StringDbParser
 * @param string_db_file path to String-DB file with interactions
 * @param alias_file path to String-DB file with identifier aliases
 * @param threshold double, between 0 and 1, lower threshold for combined interaction scores; default is medium confidence threshold 0.4
 */
template <typename NodeType, typename EdgeType>
StringDbParser<NodeType, EdgeType>::StringDbParser(const QString& string_db_file, const QString& alias_file, double threshold)
    : string_db_file_(string_db_file),
      alias_file_(alias_file),
      hgnc_translator_(),
      interaction_network_()
{
    if(threshold >= 0.0 && threshold <= 1.0)
    {
        threshold_ = (int) round(threshold * 1000);
    }
    else
    {
        THROW(ArgumentException, "Threshold for interaction score should be between 0 and 1");
    }

    parseAliasFile();
    parseStringDbFile();
}

template <typename NodeType, typename EdgeType>
StringDbParser<NodeType, EdgeType>::StringDbParser(const QString& string_db_file, const QString& alias_file)
    : StringDbParser(string_db_file, alias_file, 0.4)
{
}

template <typename NodeType, typename EdgeType>
void StringDbParser<NodeType, EdgeType>::parseAliasFile()
{
    QFile file(alias_file_);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&file);

    // skip header line
    if(!in.atEnd())
    {
        in.readLine();
    }

    // read file line by line, adding HGNC entries to the translator
    while(!in.atEnd())
    {

        QStringList line = in.readLine().split("\t", Qt::SkipEmptyParts);

        if(line.size() == 3)
        {
            if(line.at(2) == "Ensembl_HGNC_HGNC_ID" && line.at(1).startsWith("HGNC:"))
            {
                hgnc_translator_.insert(line.at(0), line.at(1));
            }
        }
    }
}

template <typename NodeType, typename EdgeType>
void StringDbParser<NodeType, EdgeType>::parseStringDbFile()
{
    QFile file(string_db_file_);

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&file);

    // skip header line
    if(!in.atEnd())
    {
        in.readLine();
    }

    // read file line by line, adding all proteins with a HGNC alias and their interactions to the graph
    while(!in.atEnd())
    {
        QStringList line = in.readLine().split(" ", Qt::SkipEmptyParts);

        if(line.size() == 3)
        {
            // only add edges for interactions between nodes with HGNC identifier and above the threshold
            int edge_score = line.at(2).toInt();
            if(hgnc_translator_.contains(line.at(0)) && hgnc_translator_.contains(line.at(1)) && edge_score >= threshold_)
            {
                NodeType node_content_1{};
                NodeType node_content_2{};
                EdgeType edge_content{};
                edge_content.weight = edge_score;
                interaction_network_.addEdge(hgnc_translator_[line.at(0)], node_content_1,\
                                            hgnc_translator_[line.at(1)], node_content_2, edge_content);
            }
        }
    }
}

template <typename NodeType, typename EdgeType>
Graph<NodeType, EdgeType>& StringDbParser<NodeType, EdgeType>::interactionNetwork()
{
    return interaction_network_;
}

#endif // STRINGDBPARSER_H
