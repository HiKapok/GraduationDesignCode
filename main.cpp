#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()

#include "common.h"
#include "kimagecvt.h" //for image convert
#include "kpicinfo.h"

#include <QCoreApplication>
#include <QDebug>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStringList>
#include <QObject>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("ImageClassfication-Test");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("ImageClassfication");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("SourceImage",QObject::tr("\tthe full path of the source input image"));
    parser.addPositionalArgument("DestinationImage",QObject::tr("\tthe path of the output image without a extendname and the format don't support are convert to bmp"));

    QCommandLineOption textureType(QStringList()<<"t"<<"type",QObject::tr("the texture to be calculate"));
    parser.addOption(textureType);

    parser.process(app);

    const QStringList argslist = parser.positionalArguments();

    QString fileinput("D:\\test.tif");
    QString fileoutput("D:\\test.tif");

    if(argslist.length()>2)
    {
        std::cout<<"the more position args are discard"<<std::endl;
        fileinput = argslist[0];
        fileoutput = argslist[1];
    }else if(argslist.length() != 0){
        fileinput = argslist[0];
        if(2 == argslist.length()){
            fileoutput = argslist[1];
        }else{
            const QStringList tempList = fileinput.split(".");

            fileoutput.clear();
            for(int index = 0;index < tempList.length() - 1;++index)
            {
                fileoutput += tempList[index];
                fileoutput += ".";
            }
//            fileoutput = fileoutput.left(fileoutput.length()-1)+"-out.";
//            fileoutput += tempList[tempList.length()-1];
            fileoutput = fileoutput.left(fileoutput.length()-1)+"-out";
            std::cout<<"the output filename are set default: "<<(fileoutput/*+"."+tempList[tempList.length()-1]*/).toStdString()<<std::endl;
        }
    }else{
        std::cout<<"you must give the input and output filename"<<std::endl;
        exit( 1 );
    }
    qDebug()<<"the input filepath"<<fileinput;
    qDebug()<<"the output filepath"<<fileoutput;

    GDALDataset *piDataset = NULL;
    GDALAllRegister();

    piDataset = (GDALDataset *) GDALOpen( fileinput.toUtf8().constData(), GA_ReadOnly );

    K_OPEN_ASSERT(piDataset,fileinput.toStdString());

    KPicInfo::dataAttach(piDataset);
    KPicInfo::getInstance()->build();

    GDALDataset *poDataset = NULL;

    if((poDataset = KImageCvt::img2gray(piDataset,poDataset,fileoutput)) == NULL) { std::cout<<"image convert failed\a"<<std::endl; exit( 1 ); }

    KImageCvt::colorReduce(poDataset,poDataset,128);

    GDALClose(piDataset);
    GDALClose(poDataset);

    std::cout<<"app run over..."<<std::endl;

    return app.exec();
}

