#include "TestFramework.h"
#include "Graph.h"
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QFile>

TEST_CLASS(Graph_Test)
{
Q_OBJECT
    int lines{8000000};
    QByteArray filename{"D://Dateien//Documents//Eigene Dateien//Studium//Tübingen//HiWi-Job//Daten//9606.protein.links.v11.0.txt"};

private slots:
    void createIntGraph()
    {
        Graph<int, int> graph;
        graph.addNode("first", 10);
        graph.addNode("second", 2);
        graph.getNode("first");
        graph.addEdge("third", 5, "fourth", 2, 4);
        graph.addEdge("third", 5, "fourth", 2, 4);
        graph.getEdge("third", "fourth");
        graph.addEdge(graph.getNode("first"), graph.getNode("third"), 10);
    }

    void parseStringDbFile()
    {
        QFile file(TESTDATA(filename));

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }

        Graph<int, int> graph;

        QTextStream in(&file);
        int i{0};

        // skip header line
        if(!in.atEnd())
        {
            in.readLine();
        }

        //QString content{""};

        // read file line by line, adding line to string
        while (!in.atEnd() && i <= lines)
        {
            //content.append(in.readLine());
            in.readLine();
            ++i;
        }
    }

    void createStringDbGraph()
    {
        QFile file(TESTDATA(filename));

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }

        Graph<int, int> graph;

        QTextStream in(&file);
        int i{0};

        // skip header line
        if(!in.atEnd())
        {
            in.readLine();
        }

        // read file line by line, adding an edge for each line
        while (!in.atEnd() && i <= lines)
        {
            QStringList line = in.readLine().split(" ");
            if(line.size() == 3)
            {
                graph.addEdge(line.at(0), 0, line.at(1), 0, line.at(2).toInt());
            }
            ++i;
        }
    }
};
