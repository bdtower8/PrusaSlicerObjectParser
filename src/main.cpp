#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QDebug>

int main(int argc, char *argv[])
{
    if(argc < 2) {
        qDebug() << "Expected usage: \"./PrusaSlicerObjectParser \"gcode_filename\"";
        return -1;
    }

    QCoreApplication app(argc, argv);

    // specify a gcode file
    QString gcodeFilename = app.arguments().at(1);
    QString gcodeOutputFilename = gcodeFilename + ".updated";
    QFile gcodeFile(gcodeFilename);

    qDebug() << "Parsing " + gcodeFilename;

    // check if it exists
    if(!gcodeFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Failed to open gcode file " + gcodeFilename;
        return -1;
    }

    // grab all the lines of text from the file
    QStringList slAllLines;
    while(!gcodeFile.atEnd())
    {
        slAllLines.append(gcodeFile.readLine());
    }

    // close the file
    gcodeFile.close();

    // use a map so the IDs can be sorted
    QMap<QString, bool> objectIDs;

    // go through all the lines, identify the number of objects
    for(QStringList::iterator lineIter = slAllLines.begin();lineIter != slAllLines.end();++lineIter)
    {
        QString sLine = *lineIter;
        // if we are starting a new object
        if(sLine.startsWith("; printing object"))
        {
            // parse the id from the gcode line
            QString objectID = sLine.mid(sLine.lastIndexOf(":") + 1, sLine.indexOf(" ", sLine.lastIndexOf(":")) - sLine.lastIndexOf(":") - 1);

            // use a map to keep a unique set of objects
            objectIDs.insert(objectID, true);

            // call the macro before starting its commands
            QString sMacroLine = "M98 P\"/macros/by-object/" + objectID + "\"\n";
            slAllLines.insert(lineIter + 1, sMacroLine);
        }
    }

    // create a directory to store the macro stubs
    QDir appDir("./");
    QString macroDirName = "macros/by-object";
    appDir.mkpath(macroDirName);
    QDir macroDir(appDir.absoluteFilePath(macroDirName));

    // create macros for each ID
    foreach(const QString& objectID, objectIDs.keys())
    {
        QFile macroFile(macroDir.absoluteFilePath(objectID));
        if(macroFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
        {
            QTextStream macroTextStream(&macroFile);
            macroTextStream << "; insert object-specific commands for id:" + objectID;
        }
        else
        {
            qDebug() << "Failed to write macro file: " + macroFile.fileName();
        }
    }

    // save the modified file
    QFile gcodeOutputFile(gcodeOutputFilename);
    if(gcodeOutputFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QTextStream outputStream(&gcodeOutputFile);
        foreach(const QString& sLineOut, slAllLines)
        {
            outputStream << sLineOut;
        }
    }
    else
    {
        qDebug() << "Failed to open file for writing";
    }

    // close the file
    gcodeOutputFile.close();

    return 0;
}
