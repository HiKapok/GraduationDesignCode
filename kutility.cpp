#include "kutility.h"

#include <QDir>
#include <QRegularExpression>
#include <QStringList>
#include <QStack>
#include <vector>
#include <QDebug>
#include <QCoreApplication>

#include <iostream>

using std::map;
using std::cout;

long KUtility::getRegionIndex(long temp)
{
    //static std::vector<long> vecIDBuff;
    static long count = 0;
    if(temp>0) count=temp;
    else return count++;
    return count;
}

QString KUtility::getDirRoot(QString filename)
{
    QString tempRet("");
    QRegularExpression re("[\\\\/]+");

    if(filename.contains('\\')||filename.contains('/')){
        QStringList tempList = filename.split(re);

        for(int pos = 0;pos<tempList.length()-1;++pos)
        {
            tempRet+=tempList[pos];
            tempRet+=QDir::separator();
        }
    }else{
        tempRet=QCoreApplication::applicationDirPath()+QDir::separator();
    }

    return tempRet;
}

void KUtility::buildInputList(map<QString,int>& list,QString RootDir)
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
            //qDebug()<<tempAbsDir+files[index];
        }
    }
}

bool KUtility::checkDirName(QString &RootDir)
{
    if(RootDir == ""){
        std::cout<<"KSVMController:an empty rootdir!"<<std::endl;
        return false;
    }
    QDir dir(RootDir);

    if(!dir.exists()){
        std::cout<<"KSVMController:the rootdir doesn't exist!"<<std::endl;
        return false;
    }
    RootDir = dir.absolutePath();

    return true;
}
