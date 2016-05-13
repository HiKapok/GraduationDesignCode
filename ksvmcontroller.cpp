#include "ksvmcontroller.h"

#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()

#include "kutility.h"
#include "ktamura.h"
#include "kglcm.h"
#include "kgabor.h"
#include "kprogressbar.h"
#include "kfeaturelbp.h"
#include "kimagecvt.h" //for image convert
#include "kpicinfo.h"

#include <QString>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QFile>

#include <iostream>
#include <fstream>
#include <utility>
#include <limits>

using std::pair;
using std::map;

KSVMController::KSVMController(QString parentTrainDir, QString parentTestDir, QString output,K_FeatureType type)
     :m_sOutFile(output),
      m_featureType(type)
{
    KUtility::buildInputList(m_vecTrainInput,parentTrainDir);
    KUtility::buildInputList(m_vecTestInput,parentTestDir);
    m_sOutRoot = KUtility::getDirRoot(output);

    GDALAllRegister();

}

void KSVMController::build()
{
    createTable(m_vecTrainInput,m_sOutRoot+"trainTbls.txt");
    createTable(m_vecTestInput,m_sOutRoot+"testTbls.txt");
}

void KSVMController::classProc()
{

    QProcess process;
    QString exePath=QCoreApplication::applicationDirPath()+"/external/tools/";
    QStringList args;
    std::cout<<"classification...\n\r";
    process.setWorkingDirectory(exePath);
    process.setStandardOutputFile("GraduationOutput.txt");
    args.push_back("easy.py");
    args.push_back(m_sOutRoot+"trainTbls.txt");
    args.push_back(m_sOutRoot+"testTbls.txt");
    process.start("python",args);
    process.waitForStarted();
    process.waitForFinished((std::numeric_limits<int>::max)());

    process.close();

    QFile file("GraduationOutput.txt");
    file.open(QFile::ReadOnly);
    std::cout<<file.readAll().constData();
    //QProcess::execute("python",args);
    //svm-scale -l 0 -u 1 -s range train > train.scale
    //svm-scale -r range test > test.scale
}

void KSVMController::createTable(map<QString, int> & lists, QString output)
{
    std::fstream fs(output.toUtf8().constData(),std::ios_base::out|std::ios_base::trunc);
    KPicInfo::beEcho=false;// close the echo char
    QRegularExpression re("[\\\\/]");

    KProgressBar progressBar("CreateTbl",lists.size(),80);
    K_PROGRESS_START(progressBar);
    for(map<QString, int>::iterator it = lists.begin();it != lists.end();++it){
        QString tempString("");

        if(m_featureType == Feature_Tamura){
            //qDebug()<<it->first;
            KTamura *feature = new KTamura(it->first);
            feature->build();
            tempString=feature->getSVMString();
            delete feature;
        }else{
            bool useImprovedLBP = false;
            if(m_featureType == Feature_ImprovedLBP) useImprovedLBP=true;
            if(m_featureType == Feature_LBP||m_featureType == Feature_ImprovedLBP){
                GDALDataset * piDataset = (GDALDataset *) GDALOpen((it->first).toUtf8().constData(), GA_ReadOnly );
                //GDALDataset * piDataset = (GDALDataset *) GDALOpen("D:\\tempimg\\1\\marais.jpg", GA_ReadOnly );

                K_OPEN_ASSERT(piDataset,(it->first).toStdString());

                QString tempOut = KUtility::getDirRoot(it->first)+"temp";
                QDir tempDir(tempOut);
                if(!tempDir.exists()) tempDir.mkpath(tempOut);
                tempOut+=QDir::separator();
                QString tempName=it->first.right(it->first.length()-it->first.lastIndexOf(re)-1);
                tempOut+=tempName.left(tempName.lastIndexOf("."));

                KPicInfo::dataAttach(piDataset,true);
                KPicInfo::getInstance()->build();

                GDALDataset *poDataset = NULL;

                if((poDataset = KImageCvt::img2gray(piDataset,poDataset,tempOut+"-gray")) == NULL) { std::cout<<"image convert failed\a"<<std::endl; exit( 1 ); }

                KPicInfo::dataAttach(poDataset,true);
                KPicInfo::getInstance()->build();

                // Calculate LBP Features
                GDALDataset *poLBPDataset = NULL;
                KFeatureLBP mLBPFeature16_2(poDataset,poLBPDataset,16,2,useImprovedLBP);
                poLBPDataset = mLBPFeature16_2.build(tempOut+"-lbp16_2");
                if(NULL != poLBPDataset){ if(!mLBPFeature16_2.run()) std::cout<<"KMakeSVMTable:calculate LBP16_2 Feature failed!"<<std::endl; }
                else std::cout<<"KSVMController:LBP16_2 build failed!"<<std::endl;

                poLBPDataset=NULL;
                KFeatureLBP mLBPFeature8_1(poDataset,poLBPDataset,8,1,useImprovedLBP);
                poLBPDataset = mLBPFeature8_1.build(tempOut+"-lbp8_1");
                if(NULL != poLBPDataset){ if(!mLBPFeature8_1.run()) std::cout<<"KMakeSVMTable:calculate LBP8_1 Feature failed!"<<std::endl; }
                else std::cout<<"KSVMController:LBP8_1 build failed!"<<std::endl;

                GDALClose(piDataset);
                GDALClose(poDataset);
                tempString=mLBPFeature8_1.getSVMString();
                //qDebug()<<"dddd:"<<tempString;
                if(useImprovedLBP)
                    tempString+=mLBPFeature16_2.getSVMString(11);
                else
                    tempString+=mLBPFeature16_2.getSVMString(257);
                //qDebug()<<"dddd2:"<<tempString;
            }else{
                if(m_featureType == Feature_Gabor){
                    KGabor *feature = new KGabor(it->first,7);
                    feature->build();
                    tempString=feature->getSVMString();
                    delete feature;
                }else{
                    if(m_featureType == Feature_GLCM){
                        KGLCM *feature = new KGLCM(it->first,32,32);
                        feature->build();
                        tempString=feature->getSVMString();
                        delete feature;
                    }else{
                        qDebug()<<"unknown feature...";
                        exit(1);
                    }
                }
            }
        }
        tempString = QString("%1").arg(it->second)+" "+tempString+"\n";
        fs<<tempString.toStdString();
        progressBar.autoUpdate();
    }
    fs.close();
    K_PROGRESS_END(progressBar);
    KPicInfo::beEcho=true;// trun on the echo char
}

