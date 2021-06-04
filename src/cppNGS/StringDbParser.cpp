#include "StringDbParser.h"

/**
 * @brief StringDbParser::StringDbParser
 * @param string_db_file path to String-DB file with interactions
 * @param alias_file path to String-DB file with identifier aliases
 * @param threshold float, between 0 and 1, lower threshold for combined interaction scores; default is medium confidence threshold 0.4
 */
StringDbParser::StringDbParser(const QByteArray& string_db_file, const QByteArray& alias_file, float threshold)
    : string_db_file_(string_db_file),
      alias_file_(alias_file),
      hgnc_translator_(),
      interaction_network_()
{
    if(threshold >= 0 && threshold <= 1)
    {
        threshold_ = (int) (threshold * 1000);
    }
    else
    {
        THROW(ArgumentException, "Threshold for interaction score should be between 0 and 1");
    }

    parseAliasFile();
    parseStringDbFile();
}

void StringDbParser::parseAliasFile()
{
    //QTextStream out(stdout);
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

        QStringList line = in.readLine().split("\t", QString::SkipEmptyParts);

        if(line.size() == 3)
        {
            if(line.at(2) == "Ensembl_HGNC_HGNC_ID" && line.at(1).startsWith("HGNC:"))
            {
                /*if(hgnc_translator_.contains(line.at(0)))
                {
                    out << line.at(0) << endl;
                }*/
                hgnc_translator_.insert(line.at(0), line.at(1));
            }
        }
    }
}

void StringDbParser::parseStringDbFile()
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
        QStringList line = in.readLine().split(" ", QString::SkipEmptyParts);

        if(line.size() == 3)
        {
            // only add edges for interactions between nodes with HGNC identifier and above the threshold
            if(hgnc_translator_.contains(line.at(0)) && hgnc_translator_.contains(line.at(1)) && line.at(2).toInt() >= threshold_)
            {
                interaction_network_.addEdge(hgnc_translator_[line.at(0)], 0, hgnc_translator_[line.at(1)], 0, line.at(2).toInt());
            }
        }
    }
}

Graph<int, int>& StringDbParser::interactionNetwork()
{
    return interaction_network_;
}
