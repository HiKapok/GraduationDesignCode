#include "kcalpmk.h"

#include "kprogressbar.h"
#include "common.h"

#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/special_functions/powm1.hpp>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <QFile>

#include <iostream>
#include <fstream>
#include <limits>

vector<map<vector<int>,long> > KCalPMK::sIndex_map;
vector<long> KCalPMK::sNowIndex;
QString KCalPMK::sMapIndexFile="";

// static function
bool KCalPMK::readMapIndex(QString filename)
{
    sMapIndexFile = filename;
    sNowIndex.clear();
    sIndex_map.clear();
    QFile file(sMapIndexFile);
    if(!sMapIndexFile.isEmpty() && file.exists()){
        std::fstream fs(sMapIndexFile.toUtf8().constData(),std::ios_base::binary|std::ios_base::in);
        long tempLong = 0;
        long tempCount = 0;
        int tempInt = 0;
        /* read nowIndex */
        fs.read(reinterpret_cast<char *>(&tempCount),sizeof(long));
        for(long index = 0;index<tempCount;++index){
            fs.read(reinterpret_cast<char *>(&tempLong),sizeof(long));
            sNowIndex.push_back(tempLong);
        }
        /* read map vector */
        fs.read(reinterpret_cast<char *>(&tempCount),sizeof(long));
        sIndex_map.resize(tempCount);
        for(long index = 0;index<tempCount;++index){
            long mapLength = 0;
            map<vector<int>,long> tempMaps;
            fs.read(reinterpret_cast<char *>(&mapLength),sizeof(long));
            /* read each map */
            for(long mapCount = 0;mapCount<mapLength;++mapCount){
                long vecLength = 0;
                vector<int> tempVectors;
                fs.read(reinterpret_cast<char *>(&vecLength),sizeof(long));
                for(long vecCount = 0;vecCount<vecLength;++vecCount){
                    fs.read(reinterpret_cast<char *>(&tempInt),sizeof(int));
                    tempVectors.push_back(tempInt);
                }
                fs.read(reinterpret_cast<char *>(&tempLong),sizeof(long));
                tempMaps[tempVectors]=tempLong;
            }
            sIndex_map[index]=tempMaps;
        }

        fs.close();

        // print to stdout for test
//        for(unsigned long index = 0;index<sNowIndex.size();++index){
//            qDebug()<<QString("NowIndex %1th:%2").arg(index).arg(sNowIndex[index]);
//        }
//        for(unsigned vecIndex = 0;vecIndex<sIndex_map.size();++vecIndex){
//            for(map<vector<int>,long>::iterator it=sIndex_map[vecIndex].begin();it!=sIndex_map[vecIndex].end();++it){
//                QString temp=QString("%1:[").arg(vecIndex);
//                for(unsigned long index = 0;index<(it->first).size();++index){
//                    temp+=QString("%1,").arg((it->first)[index]);
//                }
//                temp+="]:";
//                temp+=QString("%1").arg(it->second);
//                qDebug()<<temp;
//            }
//        }
        return true;
    }
    return false;
}
// static function
void KCalPMK::saveMapIndex()
{
//    sNowIndex.push_back(45);
//    sNowIndex.push_back(425);
//    sNowIndex.push_back(445);
//    sNowIndex.push_back(15);
//    vector<int> a;
//    map<vector<int>,long> m;
//    a.push_back(1);
//    a.push_back(2);
//    m[a]=2323;
//    a.push_back(2);
//    m[a]=22;
//    a.push_back(56);
//    m[a]=13;
//    sIndex_map.push_back(m);
//    a.push_back(1111);
//    m[a]=111;
//    sIndex_map.push_back(m);
    if(!sMapIndexFile.isEmpty()){
        std::fstream fs(sMapIndexFile.toUtf8().constData(),std::ios_base::binary|std::ios_base::out|std::ios_base::trunc);
        long tempLong = 0;
        int tempInt = 0;
        /* write nowIndex */
        // this is the length
        tempLong = sNowIndex.size();
        fs.write(reinterpret_cast<char *>(&tempLong),sizeof(long));
        // data here
        for(unsigned long index = 0;index<sNowIndex.size();++index){
            tempLong = sNowIndex[index];
            fs.write(reinterpret_cast<char *>(&tempLong),sizeof(long));
        }
        /* map follows */
        // this is the length
        tempLong = sIndex_map.size();
        fs.write(reinterpret_cast<char *>(&tempLong),sizeof(long));
        // data here
        for(unsigned long index = 0;index<sIndex_map.size();++index){
            // this is the length of each map
            tempLong = sIndex_map[index].size();
            fs.write(reinterpret_cast<char *>(&tempLong),sizeof(long));
            // data here
            for(map<vector<int>,long>::iterator it=sIndex_map[index].begin();it!=sIndex_map[index].end();++it){
                tempLong = (it->first).size();
                fs.write(reinterpret_cast<char *>(&tempLong),sizeof(long));
                for(unsigned int vecIndex = 0;vecIndex<(it->first).size();++vecIndex){
                    tempInt = (it->first)[vecIndex];
                    fs.write(reinterpret_cast<char *>(&tempInt),sizeof(int));
                }
                tempLong = it->second;
                fs.write(reinterpret_cast<char *>(&tempLong),sizeof(long));
            }
        }
        fs.close();
    }
}
// make beNewInstance true if you want to ignore any mapfile to read but write only, or map will be readed from and stored into the specified file
KCalPMK::KCalPMK(QString input, QString output,bool beNewInstance,double scale)
    :m_sInput(input),
    m_sOutput(output),
    m_dScale(scale)
{
    m_PointPair.clear();

    readPrimaryHist();
    prepare();
    if(beNewInstance){
        sIndex_map.clear();
        sNowIndex.clear();
        sIndex_map.resize(m_iMaxLevel);
        sNowIndex.resize(m_iMaxLevel,0);
    }
    m_pyramid.resize(m_iMaxLevel);
    buildPyramid();
}

void KCalPMK::savePyramid_unittest()
{
    vector<map<long,long> > tempPyramid;
    std::fstream fs(m_sOutput.toUtf8().constData(),std::ios_base::binary|std::ios_base::in);
    long mapsize = 0;
    int levels = 0;
    long tempIndex = 0;
    long tempWeight = 0;

    fs.read(reinterpret_cast<char *>(&levels),sizeof(int));

    tempPyramid.resize(levels);

    // read map
    for(int index = 0;index<levels;++index){
        fs.read(reinterpret_cast<char *>(&mapsize),sizeof(long));
        for(long mapIndex=0;mapIndex<mapsize;++mapIndex){
            fs.read(reinterpret_cast<char *>(&tempIndex),sizeof(long));
            fs.read(reinterpret_cast<char *>(&tempWeight),sizeof(long));
            tempPyramid[index][tempIndex]=tempWeight;
        }
    }
    fs.close();

    // print to stdout
    for(int index = 0;index<levels;++index){
        qDebug()<<QString("level %1th total:%2").arg(index).arg(tempPyramid[index].size());
        for(map<long,long>::iterator it = tempPyramid[index].begin();
            it != tempPyramid[index].end();++it){
            QString temp("level %1th:%2-%3");
            temp=QString(temp).arg(index);
            temp=QString(temp).arg(it->first);
            temp=QString(temp).arg(it->second);
            qDebug()<<temp;
        }
    }
    qDebug()<<"levels:"<<levels;
}

void KCalPMK::savePtramid()
{
    std::fstream fs(m_sOutput.toUtf8().constData(),std::ios_base::binary|std::ios_base::out|std::ios_base::trunc);

    long tempLong = 0;

    fs.write(reinterpret_cast<char *>(&m_iMaxLevel),sizeof(int));

    //qDebug()<<"levels:"<<m_iMaxLevel;
    for(int index = 0;index<m_iMaxLevel;++index){
        tempLong = m_pyramid[index].size();
        fs.write(reinterpret_cast<char *>(&tempLong),sizeof(long));
        for(map<long,long>::iterator it = m_pyramid[index].begin();
            it != m_pyramid[index].end();++it){
            tempLong = it->first;
            fs.write(reinterpret_cast<char *>(&tempLong),sizeof(long));
            tempLong = it->second;
            fs.write(reinterpret_cast<char *>(&tempLong),sizeof(long));
        }
    }

    fs.close();
}

void KCalPMK::readPrimaryHist()
{
    std::fstream fs(m_sInput.toUtf8().constData(),std::ios_base::binary|std::ios_base::in);
    long tempLong = 0;
    int tempInt = 0;
    fs.read(reinterpret_cast<char *>(&m_lPointPair),sizeof(long));
    //qDebug()<<"nums:"<<m_lPointPair;

    fs.read(reinterpret_cast<char *>(&m_iDims),sizeof(int));
    //qDebug()<<"files:"<<m_iDims;

    for(long index = 0;index<m_lPointPair;++index){
        vector<int> tempVec;
        for(int dimIndex=0;dimIndex<m_iDims;++dimIndex){
            fs.read(reinterpret_cast<char *>(&tempInt),sizeof(int));
            tempVec.push_back(tempInt*m_dScale);
        }
        fs.read(reinterpret_cast<char *>(&tempLong),sizeof(long));
        m_PointPair[tempVec]=tempLong;
    }

    fs.close();
}

void KCalPMK::buildPyramid()
{
    for(int index = 0;index<m_iMaxLevel;++index){
        for(map<vector<int>,long>::iterator it = m_PointPair.begin();it!=m_PointPair.end();++it){
            vector<int> tempVec;
            long binPos=0;
            long binSize = boost::math::powm1(2,index)+1;
            for(int dimIndex=0;dimIndex<m_iDims;++dimIndex){
                int temp = it->first[dimIndex];
                int binIndex = ((temp - m_dMin)- m_transTable[index][dimIndex])*1./ binSize;
                tempVec.push_back(binIndex);
            }
            if(sIndex_map[index].find(tempVec) == sIndex_map[index].end()){
                // a new position_pair in this level are found
                binPos = sNowIndex[index]++;
                // insert this new pair
                sIndex_map[index][tempVec]=binPos;
                m_pyramid[index][binPos]=0;
            }else{
                binPos = sIndex_map[index][tempVec];
            }
            m_pyramid[index][binPos]+=it->second;
        }
    }
}

void KCalPMK::prepare()
{
    calMinMax();
    m_dDiameter = m_dMax-m_dMin+1;
    //qDebug()<<m_dDiameter;
    m_iMaxLevel = boost::math::round(boost::math::log1p(m_dDiameter-1)/boost::math::log1p(1))+1;
    //qDebug()<<m_iMaxLevel;
    m_transTable.resize(m_iMaxLevel);

    for(int index=0;index<m_iMaxLevel;++index){
        long binSize = boost::math::powm1(2,index)+1;
        boost::random::mt19937 gen(std::time(0));
        boost::random::uniform_int_distribution<> dist(0, binSize);
        m_transTable[index].resize(m_iDims,0);
        for(int dimIndex = 0;dimIndex<m_iDims;++dimIndex){
            //m_transTable[index][dimIndex] = dist(gen);
            // this trans don't work well
            m_transTable[index][dimIndex] = 0;
            //qDebug()<<QString("level:%1").arg(index)<<m_transTable[index][dimIndex];
        }
    }
}

void KCalPMK::calMinMax()
{
    m_dMin = (std::numeric_limits<int>::max)();
    m_dMax = (std::numeric_limits<int>::min)();

    for(map<vector<int>,long>::iterator it = m_PointPair.begin();it!=m_PointPair.end();++it){
        for(int nDim=0;nDim<m_iDims;++nDim){
            int temp = it->first[nDim];
            if(m_dMax<temp){
                m_dMax=temp;
            }
            if(m_dMin>temp){
                m_dMin=temp;
            }
        }
    }
}
