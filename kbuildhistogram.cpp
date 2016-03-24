#include "kbuildhistogram.h"

#include "kprogressbar.h"
#include "common.h"

#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

KBuildHistogram::KBuildHistogram(QString output)
    :m_sOutput(output)
{
    m_vFileInput.clear();
    m_PointPair.clear();
}

void KBuildHistogram::build()
{
    vector<GByte *> p_vecData;
    int nXSize = 0;
    int nYSize = 0;

    for(unsigned int index = 0;index<m_vFileInput.size();++index){
        GDALDataset * piDataset = (GDALDataset *) GDALOpen( m_vFileInput[index].toUtf8().constData(), GA_ReadOnly );
        K_OPEN_ASSERT(piDataset,m_vFileInput[index].toStdString());
        nXSize = piDataset->GetRasterXSize();
        nYSize = piDataset->GetRasterYSize();
        GDALRasterBand * piBand = piDataset->GetRasterBand(1);
        GByte *pafData = (GByte *) CPLMalloc(sizeof(GByte)*nXSize*nYSize);
        piBand->RasterIO( GF_Read, 0, 0, nXSize, nYSize, pafData, nXSize, nYSize, GDT_Byte, 0, 0 );
        p_vecData.push_back(pafData);
        GDALClose(piDataset);
    }

    KProgressBar progressBar("Building Primary Histogram",nYSize*nXSize,80);
    K_PROGRESS_START(progressBar);
    for(int nY=0;nY<nYSize;++nY){
        for(int nX=0;nX<nXSize;++nX){
            vector<int> tempVec;
            for(unsigned int index = 0;index<m_vFileInput.size();++index){
                tempVec.push_back(p_vecData[index][nY*nXSize+nX]);
            }
//            if(m_PointPair.find(tempVec) != m_PointPair.end())
//                m_PointPair[tempVec]++;
//            else m_PointPair[tempVec]=1;
            m_PointPair[tempVec]++;
            progressBar.autoUpdate();
        }
    }

    K_PROGRESS_END(progressBar);
    for(unsigned int index = 0;index<p_vecData.size();++index){
        CPLFree(p_vecData[index]);
    }

    //qDebug()<<nXSize*nYSize;
}

void KBuildHistogram::save()
{
    std::fstream fs(m_sOutput.toUtf8().constData(),std::ios_base::binary|std::ios_base::out|std::ios_base::trunc);
    //std::ostringstream temp;
    long tempLong = 0;
    int tempInt = 0;
    tempLong=m_PointPair.size();
    fs.write(reinterpret_cast<char *>(&tempLong),sizeof(long));

    tempInt=m_vFileInput.size();
    fs.write(reinterpret_cast<char *>(&tempInt),sizeof(int));
//    temp<<static_cast<long>(m_PointPair.size());
//    temp<<static_cast<int>(m_vFileInput.size());

    for(map<vector<int>,long>::iterator it = m_PointPair.begin();
        it != m_PointPair.end();++it){
        for(unsigned int index=0;index<m_vFileInput.size();++index){
            //temp<<static_cast<long>(it->first[index])<<"-";
            tempInt=it->first[index];
            fs.write(reinterpret_cast<char *>(&tempInt),sizeof(int));
        }
        tempLong=it->second;

        fs.write(reinterpret_cast<char *>(&tempLong),sizeof(long));
        //temp<<static_cast<int>(it->second)<<";";
    }

//    qDebug()<<"the files size:"<<m_vFileInput.size();
//    qDebug()<<"the map size:"<<m_PointPair.size();
//    qDebug()<<"the stringbuff:"<<temp.str().data();
//    fs.write(temp.str().data(),temp.str().length());
//    fs<<temp.str();
    fs.close();
}

void KBuildHistogram::save_unittest()
{
    std::fstream fs(m_sOutput.toUtf8().constData(),std::ios_base::binary|std::ios_base::in);
    long allPointsPair = 0;
    int nFiles = 0;
    long tempLong = 0;
    int tempInt = 0;
    fs.read(reinterpret_cast<char *>(&allPointsPair),sizeof(long));
    qDebug()<<"nums:"<<allPointsPair;

    fs.read(reinterpret_cast<char *>(&nFiles),sizeof(int));
    qDebug()<<"files:"<<nFiles;

    for(long index = 0;index<allPointsPair;++index){
        QString temp("%1th:%2");
        for(int fileIndex=1;fileIndex<nFiles;++fileIndex){
            temp+=QString("-%%1").arg(fileIndex+2);
        }
        temp=QString(temp).arg(index);
        for(int fileIndex=0;fileIndex<nFiles;++fileIndex){
            fs.read(reinterpret_cast<char *>(&tempInt),sizeof(int));
            temp=QString(temp).arg(tempInt);
        }

        fs.read(reinterpret_cast<char *>(&tempLong),sizeof(long));
        qDebug()<<temp<<"total:"<<tempLong;
    }

    fs.close();
}

