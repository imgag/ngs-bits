#include "ToolBase.h"
#include "Graph.h"
#include "StringDbParser.h"

struct NodeContent
{
    double score;
    double score_increment;

    NodeContent()
        : score(0.0),
          score_increment(0.0)
    {
    }
};

struct EdgeContent
{
    double weight;

    EdgeContent()
        : weight(0.0)
    {
    }
};

class ConcreteTool
        : public ToolBase
{
    Q_OBJECT

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    virtual void setup()
    {
        setDescription("Creates simple representation of String-DB interaction graph.");
		addInfile("string", "String-DB file with protein interactions (https://stringdb-static.org/download/protein.links.v11.5/9606.protein.links.v11.5.txt.gz).", false);
		addInfile("alias", "Input TSV file with aliases for String protein IDs (https://stringdb-static.org/download/protein.aliases.v11.5/9606.protein.aliases.v11.5.txt.gz).", false);
		addOutfile("out", "Output TSV file with edges.", false);
        //optional
		addFloat("min_score", "Minimum confidence score cutoff for String-DB interaction (0-1).", true, 0.4);
    }

    virtual void main()
    {
        // init
		StringDbParser<NodeContent, EdgeContent> string_parser(getInfile("string"), getInfile("alias"), getFloat("min_score"));
        Graph<NodeContent, EdgeContent> interaction_network = string_parser.interactionNetwork();

        interaction_network.store(getOutfile("out"));
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

