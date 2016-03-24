#include "kmakesvmtable.h"

#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()

#include "kprogressbar.h"
#include "common.h"
#include "kpicinfo.h"
#include "kimagecvt.h" //for image convert
#include "kfeaturelbp.h"
#include "kbuildhistogram.h"
#include "kcalpmk.h"
#include "kpyrimadmatch.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <QDebug>
#include <QtGlobal>
#include <QStack>
#include <QStringList>
#include <QRegularExpression>
#include <QCoreApplication>

#include <iostream>
#include <fstream>

KMakeSVMTable::KMakeSVMTable(map<QString,int> inputList, QString output, bool beTrain)
      :m_vecInput(inputList),
       m_sOutput(output),
       m_beTraining(beTrain)
{
    m_sOutRoot = getDirRoot(m_sOutput);
    QString mapFileOutPath = m_sOutRoot;
    if(m_beTraining) mapFileOutPath += "trainFileIndexMap.txt";
    else mapFileOutPath += "testFileIndexMap.txt";

    std::fstream fs(mapFileOutPath.toUtf8().constData(),std::ios_base::out|std::ios_base::trunc);

    for(map<QString,int>::iterator it = inputList.begin();it!=inputList.end();++it){
        QString tempString("");
        tempString = QString("label:%1\tfile:%2\n").arg(it->second,-6).arg(it->first);
        fs<<tempString.toStdString();
    }
    fs.close();
    buildAllPyramid();
}

KMakeSVMTable::KMakeSVMTable(QString parentDir, QString output, bool beTrain)
{
    map<QString,int> inputList;
    buildInputList(inputList,parentDir);
    KMakeSVMTable(inputList,output,beTrain);
}

void KMakeSVMTable::makeTable()
{
    std::fstream fs(m_sOutput.toUtf8().constData(),std::ios_base::out|std::ios_base::trunc);

    KProgressBar progressBar("BuildSimilarityTbl",m_vecPyramid.size()*m_vecPyramid.size(),80);
    K_PROGRESS_START(progressBar);

    long index=1;
    long innerIndex=1;
    for(map<QString,int>::iterator it = m_vecPyramid.begin();it != m_vecPyramid.end();++it,++index){
        innerIndex=1;
        QString tempLine=QString("%1 0:%2").arg(it->second).arg(index);
        for(unsigned long tempIndex = 0;tempIndex<m_vecPyramid.size();++tempIndex){
            tempLine+=QString(" %1:%%2").arg(tempIndex+1).arg(tempIndex+1);
        }
        for(map<QString,int>::iterator itAnother = m_vecPyramid.begin();itAnother != m_vecPyramid.end();++itAnother,++innerIndex){
            // do match
            KPyrimadMatch match(it->first,itAnother->first);
            double similarity = match.doMatch();
            tempLine=tempLine.arg(similarity);
            progressBar.autoUpdate();
        }
        fs<<tempLine.toStdString()<<"\n";
    }
    K_PROGRESS_END(progressBar);
}

QString KMakeSVMTable::getDirRoot(QString filename)
{
    QString tempRet("");
    QRegularExpression re("[\\/\\\\//]");

    if(filename.contains('\\')||filename.contains('/')){
        QStringList tempList = filename.split(re);
        for(int pos = 0;pos<tempList.length()-1;++pos)
        {
            tempRet+=tempList[pos];
            tempRet+="/";
        }
    }else{
        tempRet=QCoreApplication::applicationDirPath()+"/";
    }
    return tempRet;
}

void KMakeSVMTable::buildInputList(map<QString,int>& list,QString RootDir)
{
    QStack<QString> DirList;
    QRegularExpression re("[\\/\\\\//]");
    // ensure a directory
    if(!checkDirName(RootDir)){ exit(1); }

    DirList.push(RootDir);

    while(!DirList.empty()){
        QString tempDir = DirList.pop();

        QDir temp(tempDir);
        QString tempAbsDir = temp.absolutePath() + QDir::separator();
        QStringList dirlist = temp.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

        if(!dirlist.empty()){
            for(int index = 0;index<dirlist.length();++index){
                if(!dirlist[index].contains("temp"))
                    DirList.push(tempAbsDir + dirlist[index]);
            }
        }
        if(RootDir==tempDir) continue;
        QStringList tempList = tempAbsDir.split(re);
        QStringList files = temp.entryList(QDir::Files);
        int label = tempList[tempList.length()-2].toInt();
        for(int index =0;index<files.length();++index){
            list[tempAbsDir+files[index]]=label;
        }
    }
}

void KMakeSVMTable::buildAllPyramid()
{
    std::streambuf *backup;
    std::ifstream fin;
    fin.open("data.in");
    backup = std::cin.rdbuf();   // back up cin's streambuf
    std::cin.rdbuf(fin.rdbuf()); // assign file's streambuf to cin
    // ... cin will read from file
    std::cin.rdbuf(backup);     // restore cin's original streambuf

    static bool firstInstance = true;
    GDALAllRegister();

    QRegularExpression re("[\\\\/]");
    KProgressBar progressBar("BuildAllPyramid",m_vecInput.size(),80);
    K_PROGRESS_START(progressBar);

    for(map<QString,int>::iterator it = m_vecInput.begin();it != m_vecInput.end();++it){

        GDALDataset * piDataset = (GDALDataset *) GDALOpen((it->first).toUtf8().constData(), GA_ReadOnly );
        //GDALDataset * piDataset = (GDALDataset *) GDALOpen("D:\\tempimg\\1\\marais.jpg", GA_ReadOnly );

        K_OPEN_ASSERT(piDataset,(it->first).toStdString());

        QString tempOut = getDirRoot(it->first)+"temp";
        QDir tempDir(tempOut);
        if(!tempDir.exists()) tempDir.mkpath(tempOut);
        tempOut+=QDir::separator();
        QString tempName=it->first.right(it->first.length()-it->first.lastIndexOf(re)-1);
        tempOut+=tempName.left(tempName.lastIndexOf("."));
        // no need for extend filename
        //QString tempNameExt= tempName.right(tempName.length()-tempName.lastIndexOf("."));

        KPicInfo::dataAttach(piDataset,true);
        KPicInfo::getInstance()->build();

        GDALDataset *poDataset = NULL;

        if((poDataset = KImageCvt::img2gray(piDataset,poDataset,tempOut+"-gray")) == NULL) { std::cout<<"image convert failed\a"<<std::endl; exit( 1 ); }

        // Calculate LBP Features
        GDALDataset *poLBPDataset = NULL;
        KFeatureLBP mLBPFeature8_1(poDataset,poLBPDataset,8,1);
        poLBPDataset = mLBPFeature8_1.build(tempOut+"-lbp8_1");
        if(NULL != poLBPDataset){ if(!mLBPFeature8_1.run()) std::cout<<"KMakeSVMTable:calculate LBP8_1 Feature failed!"<<std::endl; }
        else std::cout<<"KMakeSVMTable:LBP8_1 build failed!"<<std::endl;

        poLBPDataset=NULL;
        KFeatureLBP mLBPFeature16_2(poDataset,poLBPDataset,16,2);
        poLBPDataset = mLBPFeature16_2.build(tempOut+"-lbp16_2");
        if(NULL != poLBPDataset){ if(!mLBPFeature16_2.run()) std::cout<<"KMakeSVMTable:calculate LBP16_2 Feature failed!"<<std::endl; }
        else std::cout<<"KMakeSVMTable:LBP16_2 build failed!"<<std::endl;

        GDALClose(piDataset);
        GDALClose(poDataset);

        // make the primary histogram
        KBuildHistogram primaryHist(tempOut+"-histogram.bin");
        primaryHist.addFile(tempOut+"-lbp8_1"+mLBPFeature8_1.getRealExtName());
        primaryHist.addFile(tempOut+"-lbp16_2"+mLBPFeature16_2.getRealExtName());
        primaryHist.build();
        primaryHist.save();

        // build the pyrimad
        QString pyrimadName=tempOut+"-primary.bin";
        KCalPMK pmk(tempOut+"-histogram.bin",pyrimadName,firstInstance);
        pmk.savePtramid();

        firstInstance = false;

        m_vecPyramid[pyrimadName]=it->second;

        progressBar.autoUpdate();

    }
    K_PROGRESS_END(progressBar);
}

bool KMakeSVMTable::checkDirName(QString &RootDir)
{
    if(RootDir == ""){
        std::cout<<"KMakeSVMTable:an empty rootdir!"<<std::endl;
        return false;
    }
    QDir dir(RootDir);

    if(!dir.exists()){
        std::cout<<"KMakeSVMTable:the rootdir doesn't exist!"<<std::endl;
        return false;
    }
    RootDir = dir.absolutePath();

    return true;
}
