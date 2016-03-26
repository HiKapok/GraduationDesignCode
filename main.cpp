#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()

#include "common.h"
#include "kimagecvt.h" //for image convert
#include "kpicinfo.h"
#include "kfeaturelbp.h"
#include "ksplitimage.h"

#include "kmakelbp_svmtable.h"

#include <QCoreApplication>
#include <QDebug>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStringList>
#include <QObject>
#include <QProcess>

#include "kcalpmk.h"

//#include <QRegularExpression>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("ImageClassfication-Test");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("ImageClassfication");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("TrainImages",QObject::tr("\tthe parent directorypath of the train images folders"));
    parser.addPositionalArgument("TestImages",QObject::tr("\tthe parent directorypath of the test images folders"));
    parser.addPositionalArgument("PredictOut",QObject::tr("\tthe filepath of the predict output"));

    QCommandLineOption textureType(QStringList()<<"t"<<"type",QObject::tr("the texture to be calculate"));
    parser.addOption(textureType);

    QCommandLineOption beOverwriteCache("o",QObject::tr("whether to overwrite the caches"));
    parser.addOption(beOverwriteCache);

    parser.process(app);

    const QStringList argslist = parser.positionalArguments();

//    QString fileinput("D:\\test.tif");
//    QString fileoutput("D:\\test.tif");

    QString TrainImages("");
    QString TestImages("");
    QString PredictOut("");
    if(argslist.length()>3)
    {
        std::cout<<"the more position args are discarded"<<std::endl;
        TrainImages = argslist[0];
        TestImages = argslist[1];
        PredictOut = argslist[2];
    }else if(argslist.length() > 1){
        if(3 == argslist.length()){
            TrainImages = argslist[0];
            TestImages = argslist[1];
            PredictOut = argslist[2];

        }else{
            TrainImages = argslist[0];
            TestImages = argslist[0]+"-test";
            PredictOut = argslist[1];
//            const QStringList tempList = fileinput.split(".");

//            fileoutput.clear();
//            for(int index = 0;index < tempList.length() - 1;++index)
//            {
//                fileoutput += tempList[index];
//                fileoutput += ".";
//            }
////            fileoutput = fileoutput.left(fileoutput.length()-1)+"-out.";
////            fileoutput += tempList[tempList.length()-1];
//            fileoutput = fileoutput.left(fileoutput.length()-1)+"-out";
//            std::cout<<"the output filename are set default: "<<(fileoutput/*+"."+tempList[tempList.length()-1]*/).toStdString()<<std::endl;
        }
    }else{
        std::cout<<"you must give enough args"<<std::endl;
        exit( 1 );
    }
    QString typeValue("lbp");
    if(parser.isSet(textureType)) typeValue = parser.value(textureType);
    //qDebug()<<"the input filepath"<<fileinput;
    //qDebug()<<"the output filepath"<<fileoutput;
    if(typeValue=="lbp"){

        bool beOverWrite = parser.isSet(beOverwriteCache);
        KMakeLBP_SVMTable svmTbl(TrainImages,TestImages,PredictOut,beOverWrite);
        svmTbl.makeTable();
        QString exePath=QCoreApplication::applicationDirPath()+"/windows/";
        QStringList args;
        args.push_back(svmTbl.getTrainFile());
        args.push_back(svmTbl.getRootDir()+"model.bin");
        QProcess::execute(exePath+"svm-train.exe",args);
        args.clear();
        args.push_back(svmTbl.getTestFile());
        args.push_back(svmTbl.getRootDir() + "model.bin");
        args.push_back(PredictOut);
        QProcess::execute(exePath+"svm-predict.exe",args);

    }else{
        qDebug()<<"unknown feature type to use!";
    }
//    GDALDataset *piDataset = NULL;
//    GDALAllRegister();

//    piDataset = (GDALDataset *) GDALOpen( fileinput.toUtf8().constData(), GA_ReadOnly );

//    K_OPEN_ASSERT(piDataset,fileinput.toStdString());

//    KPicInfo::dataAttach(piDataset);
//    KPicInfo::getInstance()->build();

//    GDALDataset *poDataset = NULL;

//    if((poDataset = KImageCvt::img2gray(piDataset,poDataset,fileoutput)) == NULL) { std::cout<<"image convert failed\a"<<std::endl; exit( 1 ); }

//    // Color Reduce
//    //KImageCvt::colorReduce(poDataset,poDataset,128);

//    // split the image
//    //KSplitImage split(piDataset,"ss\\tt");
//    //split.split(8,8);

//    GDALClose(piDataset);
//    GDALClose(poDataset);

    std::cout<<"app run over...";
    std::cout.flush();
//    return app.exec();
    return 0;
}

