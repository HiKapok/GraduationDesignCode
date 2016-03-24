#include "kpyrimadmatch.h"

#include "kprogressbar.h"
#include "common.h"

#include <boost/math/special_functions/powm1.hpp>
#include <boost/math/special_functions/sqrt1pm1.hpp>

#include <QtGlobal>

#include <iostream>
#include <fstream>

/**
 * Sn=a1*(1-q^n)/(1-q)
 */

KPyrimadMatch::KPyrimadMatch(QString input, QString inputAnother, bool normarlized)
     :m_sInput(input),
      m_sInputAnother(inputAnother),
      m_beToNormarlized(normarlized)
{
    readPyrimad(m_sInput,m_vecPyrimad);
    readPyrimad(m_sInputAnother,m_vecPyrimadAnother);
    Q_ASSERT(m_vecPyrimad.size()==m_vecPyrimadAnother.size());
    m_iTotalLevel=m_vecPyrimad.size();
}

void KPyrimadMatch::readPyrimad(QString &file,vector<map<long,long> > & pyrimad)
{
    std::fstream fs(file.toUtf8().constData(),std::ios_base::binary|std::ios_base::in);
    long mapsize = 0;
    int levels = 0;
    long tempIndex = 0;
    long tempWeight = 0;

    fs.read(reinterpret_cast<char *>(&levels),sizeof(int));

    pyrimad.resize(levels);

    // read map
    for(int index = 0;index<levels;++index){
        fs.read(reinterpret_cast<char *>(&mapsize),sizeof(long));
        for(long mapIndex=0;mapIndex<mapsize;++mapIndex){
            fs.read(reinterpret_cast<char *>(&tempIndex),sizeof(long));
            fs.read(reinterpret_cast<char *>(&tempWeight),sizeof(long));
            pyrimad[index][tempIndex]=tempWeight;
        }
    }
    fs.close();

    //qDebug()<<file;
    // print to stdout
//    for(int index = 0;index<levels;++index){
//        qDebug()<<QString("level %1th total:%2").arg(index).arg(pyrimad[index].size());
//        for(map<long,long>::iterator it = pyrimad[index].begin();
//            it != pyrimad[index].end();++it){
//            QString temp("level %1th:%2-%3");
//            temp=QString(temp).arg(index);
//            temp=QString(temp).arg(it->first);
//            temp=QString(temp).arg(it->second);
//            qDebug()<<temp;
//        }
//    }
//    qDebug()<<"levels:"<<levels;
}

double KPyrimadMatch::doMatch()
{
    double similarity = 0.;
    long hisMatch = 0;
    long thisMatch = 0;
    for(int index = 0;index<m_iTotalLevel;++index){
        long binSize = boost::math::powm1(2,index)+1;
        double weight = 1./binSize;
        thisMatch = 0;
        map<long,long>::iterator itAnother = m_vecPyrimadAnother[index].begin();
        for(map<long,long>::iterator it = m_vecPyrimad[index].begin();it != m_vecPyrimad[index].end();++it){
            if((itAnother = m_vecPyrimadAnother[index].find(it->first)) != m_vecPyrimadAnother[index].end()){
                thisMatch += std::min(it->second,itAnother->second);
            }
        }
        similarity += weight*(thisMatch-hisMatch);
        hisMatch = thisMatch;
    }
    // get self similarity
    if(m_beToNormarlized){
        double selfSimi = 0.;
        double selfSimiAnother = 0.;

        for(map<long,long>::iterator it = m_vecPyrimad[0].begin();it != m_vecPyrimad[0].end();++it){
            selfSimi += it->second;
        }

        for(map<long,long>::iterator it = m_vecPyrimadAnother[0].begin();it != m_vecPyrimadAnother[0].end();++it){
            selfSimiAnother += it->second;
        }
        similarity /= (boost::math::sqrt1pm1(selfSimi*selfSimiAnother-1.) + 1.);
    }
    // similarity is between 0~1 after normarlized
    return similarity;
}

