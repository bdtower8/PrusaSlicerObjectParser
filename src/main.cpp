#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QDebug>

int main(int argc, char *argv[])
{
    if(argc < 4) {
        qDebug() << "Expected usage: \"./PrusaSlicerObjectParser \"gcode_filename\" x y";
        return -1;
    }

    // variables to test

    // jerk
    const float jerkMin = 300.0f;
    const float jerkMax = 1500.0f;
    const float jerkRange = jerkMax - jerkMin;

    // accel
    const float accelMin = 600.0f;
    const float accelMax = 4000.0f;
    const float accelRange = accelMax - accelMin;

    // no magic-numbers later in the code
    const int travelAccel = 5000;
    const int retractAccel = 5000;
    const int zJerk = 60;
    const int eJerk = 300;

    // specify a gcode file
    QCoreApplication app(argc, argv);
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
    QMap<int, bool> objectIDs;

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
            objectIDs.insert(objectID.toInt(), true);

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

    // span jerk range across x
    const int numX = app.arguments().at(2).toInt();
    const float jerkStep = jerkRange / float(numX - 1);

    // span accel range across y
    const int numY = app.arguments().at(3).toInt();
    const float accelStep = accelRange / float(numY - 1);

    // create macros for each ID
    foreach(const int& objectID, objectIDs.keys())
    {
        // get the x/y location
        int xLoc = objectID / numX;
        int yLoc = objectID % numX;

        // calculate object-specific settings
        int travelJerk = (xLoc * jerkStep) + jerkMin;
        int printAccel = (yLoc * accelStep) + accelMin;

        // create the custom macro
        QFile macroFile(macroDir.absoluteFilePath(QString::number(objectID)));
        if(macroFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
        {
            QTextStream macroTextStream(&macroFile);
            macroTextStream << "; insert object-specific commands for id:" + QString::number(objectID) + "\n";
            macroTextStream << "M204 R" + QString::number(retractAccel) + " T" + QString::number(travelAccel) + " P" + QString::number(printAccel) + "\n";
            macroTextStream << "M566 X" + QString::number(travelJerk) + " Y" + QString::number(travelJerk) + " Z" + QString::number(zJerk) + " E" + QString::number(eJerk) + "\n";
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
