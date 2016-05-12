#include "ktamura.h"

#include "kprogressbar.h"
#include "common.h"
#include "kutility.h"
#include "kpicinfo.h"
#include "kimagecvt.h" //for image convert

#include <boost/math/special_functions/powm1.hpp>
#include <boost/math/constants/constants.hpp>

#include <vector>
#include <map>
#include <utility>
#include <limits>
#include <cmath>

#include <QRegularExpression>
#include <QCoreApplication>
#include <QDir>

using std::vector;
using std::map;
using std::make_pair;

KTamura::KTamura(QString input)
    :m_iMaxExtend(32)
{
    QRegularExpression re("[\\\\/]");

    GDALDataset * piDataset = (GDALDataset *) GDALOpen(input.toUtf8().constData(), GA_ReadOnly );

    K_OPEN_ASSERT(piDataset,input.toStdString());

    m_sOutputRoot = getDirRoot(input);

    QString tempName=input.right(input.length()-input.lastIndexOf(re)-1);
    m_sFileNoExtName=tempName.left(tempName.lastIndexOf("."));

    GDALDataset *poDataset = NULL;

    if((poDataset = KImageCvt::img2gray(piDataset,poDataset,m_sOutputRoot+"temp"+QDir::separator()+m_sFileNoExtName+"-gray")) == NULL) { std::cout<<"image convert failed\a"<<std::endl; exit(1); }

    m_lXSize = piDataset->GetRasterXSize();
    m_lYSize = piDataset->GetRasterYSize();
    m_fOrgArray = (float *) CPLMalloc(sizeof(float)*m_lXSize*m_lYSize);
    m_fExtArray = (float *) CPLMalloc(sizeof(float)*(m_lXSize+2*m_iMaxExtend)*(m_lYSize+2*m_iMaxExtend));

    // gray only
    GDALRasterBand * poBand = poDataset->GetRasterBand(1);
    poBand->RasterIO( GF_Read, 0, 0, m_lXSize, m_lYSize, m_fOrgArray, m_lXSize, m_lYSize, GDT_Float32, 0, 0 );

    KUtility::reflectExtend<float>(m_fOrgArray,m_fExtArray,m_lXSize,m_lYSize,m_iMaxExtend);

    GDALClose(piDataset);
    GDALClose(poDataset);
}

KTamura::~KTamura()
{
    CPLFree(m_fOrgArray);
    CPLFree(m_fExtArray);
}

void KTamura::build()
{

    m_dCoarseness = calCoarseness();
    m_dContrast = calContrast();
    m_dDirectionality = calDirectionality();
}

double KTamura::calCoarseness()
{
    map<int,std::pair<double,double> > mapAvrGray;
    double coarseness = 0.;
    float * tempOut = new float[m_lXSize*m_lYSize];

    unsigned long int lineLength = m_lXSize+2*m_iMaxExtend;
    KProgressBar progressBar("CalCoarseness",m_lXSize*m_lYSize,80);
    K_PROGRESS_START(progressBar);
    long int orgY = m_iMaxExtend;
    for(unsigned long int desY = 0;orgY<m_lYSize+m_iMaxExtend;++orgY,++desY){
        long int orgX = m_iMaxExtend;
        for(unsigned long int desX = 0;orgX<m_lXSize+m_iMaxExtend;++orgX,++desX){
            mapAvrGray.clear();
            double hDiff = abs(m_fExtArray[orgY*lineLength+orgX-1]-m_fExtArray[orgY*lineLength+orgX+1]);
            double vDiff = abs(m_fExtArray[(orgY-1)*lineLength+orgX]-m_fExtArray[(orgY+1)*lineLength+orgX]);
            mapAvrGray[0] = make_pair(hDiff,vDiff);

            for(int k = 1;k < 6;k++){
                long binSize = boost::math::powm1(2,k)+1;
                hDiff = abs(getAverageGray(m_fExtArray,lineLength,orgX-binSize/2,orgY,binSize)-getAverageGray(m_fExtArray,lineLength,orgX+binSize/2,orgY,binSize));
                vDiff = abs(getAverageGray(m_fExtArray,lineLength,orgX,orgY-binSize/2,binSize)-getAverageGray(m_fExtArray,lineLength,orgX,orgY+binSize/2,binSize));
                mapAvrGray[k] = make_pair(hDiff,vDiff);
            }

            double eMax = (std::numeric_limits<double>::min)();
            int kMax=-1;
            // find the max E
            for(map<int,std::pair<double,double> >::iterator it = mapAvrGray.begin();
                it != mapAvrGray.end(); ++it){
                double tempMax = std::max(it->second.first,it->second.second);
                if(eMax<tempMax){ kMax=it->first; eMax=tempMax; }
            }
            // find optimal E & k
            int tempKMax=kMax;
            for(map<int,std::pair<double,double> >::iterator it = mapAvrGray.begin();
                it != mapAvrGray.end(); ++it){
                if(it->first>kMax){
                    double tempMax = std::max(it->second.first,it->second.second);
                    if(tempMax>0.9*eMax){
                        if(tempKMax<it->first) tempKMax = it->first;
                    }
                }
            }
            kMax=tempKMax;
            tempOut[desY*m_lXSize+desX] = boost::math::powm1(2,kMax)+1;
            progressBar.autoUpdate();
        }
    }
    K_PROGRESS_END(progressBar);

    for(long iY = 0;iY<m_lYSize;++iY){
        for(long iX = 0;iX<m_lXSize;++iX){
            coarseness += tempOut[iY*m_lXSize+iX];
        }
    }

    coarseness = coarseness / (m_lXSize*m_lYSize);

    delete [] tempOut;
    return coarseness;
}

double KTamura::calContrast()
{
    double alpha4 = 0.;
    double miu4 = 0.;
    double delta2 = 0.;
    double delta = 0.;
    double avr = 0.;
    double contrast = 0.;
    /* alpha4 = Sum((Yi - Yavr)^4)/((N-1)*delta^4) */
    // get Average
    for(long int iY = 0;iY<m_lYSize;++iY){
        for(long int iX = 0;iX<m_lXSize;++iX){
            avr+=m_fOrgArray[iY*m_lXSize+iX];
        }
    }
    avr /= (m_lYSize*m_lXSize);
    // get Variance and mean fourth
    for(long int iY = 0;iY<m_lYSize;++iY){
        for(long int iX = 0;iX<m_lXSize;++iX){
            double tempValue = m_fOrgArray[iY*m_lXSize+iX];
            double temp = 0.;
            temp = abs(tempValue-avr);
            delta2 += temp * temp;
            miu4 += delta2 * delta2;
        }
    }
    delta2 /= (m_lYSize*m_lXSize);
    delta = boost::math::powm1(delta2,0.5)+1;
    miu4 /= (m_lYSize*m_lXSize - 1);
    alpha4 = miu4 / (delta2*delta2);
    contrast = delta/(boost::math::powm1(alpha4,0.25)+1);
    return contrast;
}

double KTamura::calDirectionality()
{
    const int quantizationLevel = 16;
    const int thresT = 12;
    double directionality = 0.;
    float * tempDeltaG = new float[m_lXSize*m_lYSize];
    float * tempTheta = new float[m_lXSize*m_lYSize];
    long * histogramD = new long[quantizationLevel];
    long imageLength = m_lXSize*m_lYSize;
    unsigned long int lineLength = m_lXSize+2*m_iMaxExtend;
    //memset(histogramD,0,quantizationLevel);
    for(int index = 0;index < quantizationLevel;++index){
        histogramD[index]=0;
    }
    long int orgY = m_iMaxExtend;
    for(unsigned long int desY = 0;orgY<m_lYSize+m_iMaxExtend;++orgY,++desY){
        long int orgX = m_iMaxExtend;
        for(unsigned long int desX = 0;orgX<m_lXSize+m_iMaxExtend;++orgX,++desX){
            float tempDeltaH=0.;
            float tempDeltaV=0.;
            tempDeltaH = m_fExtArray[(orgY-1)*lineLength+orgX+1]+m_fExtArray[orgY*lineLength+orgX+1]+m_fExtArray[(orgY+1)*lineLength+orgX+1]-
                         m_fExtArray[(orgY-1)*lineLength+orgX-1]-m_fExtArray[orgY*lineLength+orgX-1]-m_fExtArray[(orgY+1)*lineLength+orgX-1];
            tempDeltaV = m_fExtArray[(orgY-1)*lineLength+orgX-1]+m_fExtArray[(orgY-1)*lineLength+orgX]+m_fExtArray[(orgY-1)*lineLength+orgX+1]-
                         m_fExtArray[(orgY+1)*lineLength+orgX-1]-m_fExtArray[(orgY+1)*lineLength+orgX]-m_fExtArray[(orgY+1)*lineLength+orgX+1];
            tempDeltaG[desY*m_lXSize+desX] = (abs(tempDeltaV) + abs(tempDeltaH))/2;
            if(tempDeltaH<0.0001&&tempDeltaH>0.) tempDeltaH=0.0001;
            if(tempDeltaH>-0.0001&&tempDeltaH<0.) tempDeltaH=-0.0001;
            if(tempDeltaH==0.) tempDeltaH+=0.00001;
            tempTheta[desY*m_lXSize+desX] = atan(tempDeltaV/tempDeltaH)+boost::math::constants::pi<float>()/2;
            //if(tempTheta[desY*m_lXSize+desX]<0) tempTheta[desY*m_lXSize+desX]+=boost::math::constants::pi<float>();
            //qDebug()<<"DeltaG"<<tempDeltaG[desY*m_lXSize+desX]<<"DeltaH:"<<tempDeltaH<<"DeltaV:"<<tempDeltaV<<"tempTheta:"<<tempTheta[desY*m_lXSize+desX];
        }
    }
    // get HistogramD
    float sumHistogramD = 0.;
    for(int index = 0;index < quantizationLevel;++index){
        //qDebug()<<"start:"<<histogramD[index];
        float thetaLow = (2*index)*boost::math::constants::pi<float>()/(2*quantizationLevel);
        float thetaHigh = (2*index+2)*boost::math::constants::pi<float>()/(2*quantizationLevel);
        //qDebug()<<"thetaLow"<<thetaLow<<"thetaHigh"<<thetaHigh;

        for(long imageIndex = 0;imageIndex < imageLength;++imageIndex){
            //qDebug()<<tempTheta[imageIndex];
            if(tempDeltaG[imageIndex]>thresT){
                if(tempTheta[imageIndex]>=thetaLow&&tempTheta[imageIndex]<thetaHigh){
                    histogramD[index]+=1;
                }
            }
        }
        //qDebug()<<"a:"<<a;
        //qDebug()<<"histogramD:"<<histogramD[index];
        sumHistogramD += histogramD[index];
    }
//    qDebug()<<sumHistogramD;
//    for(int index = 0;index < quantizationLevel;++index){
//        //qDebug()<<"1:"<<histogramD[index];
//        histogramD[index]=histogramD[index];
//        //qDebug()<<"2:"<<histogramD[index];
//    }
//    QString temp("");
//    for(int index = 0;index < quantizationLevel;++index){
//        temp+=QString("%1:%2 ").arg(index).arg(histogramD[index]);
//    }
//    qDebug()<<temp;
    // get Directionality
    // find all the valley and it's position
    vector<std::pair<int,float> > tempValleyArray;
    vector<std::pair<int,float> > valleyArray;
    vector<std::pair<int,float> > finalValley;
    int firstPeak = getNextValleyPeak(histogramD,0,quantizationLevel,true);
    int lastPos = firstPeak;
    tempValleyArray.push_back(make_pair(firstPeak,histogramD[firstPeak]));
    int tempPeakValley = getNextValleyPeak(histogramD,lastPos,quantizationLevel);
    while((lastPos>=firstPeak && tempPeakValley>lastPos) || tempPeakValley < firstPeak){
        tempValleyArray.push_back(make_pair(tempPeakValley, histogramD[tempPeakValley]));
        lastPos = tempPeakValley;
        tempPeakValley = getNextValleyPeak(histogramD, lastPos, quantizationLevel);
    }
    //qDebug()<<"tempValleyArray"<<tempValleyArray.size();
    //for(unsigned int index = 0;index < tempValleyArray.size();index++){
    //    qDebug()<<tempValleyArray[index].second;
    //}
    float maxPeakRate = (std::numeric_limits<float>::min)();
    int maxRatePos = -1;
    // remove the peak and valley which peak/valley<2.0
    for(unsigned int index = 0;index < tempValleyArray.size();index+=2){
        float tempRate = tempValleyArray[index].second/tempValleyArray[index+1].second;
        if(tempRate>maxPeakRate){ maxPeakRate = tempRate; maxRatePos = index; }
        if(tempRate>2.){
            if(0==index) valleyArray.push_back(tempValleyArray[tempValleyArray.size()-1]);
            else valleyArray.push_back(tempValleyArray[index-1]);
            valleyArray.push_back(tempValleyArray[index]);
            valleyArray.push_back(tempValleyArray[index+1]);
        }
    }
    if(0 == valleyArray.size() && maxPeakRate>1.){
        if(0==maxRatePos) valleyArray.push_back(tempValleyArray[tempValleyArray.size()-1]);
        else valleyArray.push_back(tempValleyArray[maxRatePos-1]);
        valleyArray.push_back(tempValleyArray[maxRatePos]);
        valleyArray.push_back(tempValleyArray[maxRatePos+1]);
    }
    // get the mian peak and the secondmain peak
    float maxPeak = (std::numeric_limits<float>::min)();
    int maxPos = -1;
    float secondMaxPeak = (std::numeric_limits<float>::min)();
    int secondMaxPos = -1;
    //qDebug()<<"valleySize:"<<valleyArray.size();
    for(unsigned int index = 1;index < valleyArray.size();index+=3){
        if(valleyArray[index].second>maxPeak){ maxPeak = valleyArray[index].second; maxPos=index; }
    }
    for(unsigned int index = 1;index < valleyArray.size();index+=3){
        if(static_cast<int>(index)!=maxPos && valleyArray[index].second>secondMaxPeak){ secondMaxPeak = valleyArray[index].second; secondMaxPos=index; }
    }
    //qDebug()<<"max:"<<maxPos;
    if(-1 == maxPos){
        delete [] tempDeltaG;
        delete [] tempTheta;
        delete [] histogramD;
        return 0.;
    }
    //for(unsigned int index = 0;index < tempValleyArray.size();index++)
    //        qDebug()<<index<<":"<<tempValleyArray[index].first;
    //for(unsigned int index = 0;index < valleyArray.size();index++)
    //    qDebug()<<index<<":"<<valleyArray[index].first;
    finalValley.push_back(valleyArray[maxPos-1]);
    finalValley.push_back(valleyArray[maxPos]);
    finalValley.push_back(valleyArray[maxPos+1]);
    if(-1 != secondMaxPos){
        if(maxPeak/secondMaxPeak<5.){
            finalValley.push_back(valleyArray[secondMaxPos-1]);
            finalValley.push_back(valleyArray[secondMaxPos]);
            finalValley.push_back(valleyArray[secondMaxPos+1]);
        }
    }

    int nPeaks = finalValley.size()/3;
//    vector<long> doubleHistogramD;
//    for(int index = 0;index<2*quantizationLevel;++index){
//        doubleHistogramD.push_back(histogramD[index%quantizationLevel]);
//    }
    for(unsigned int index = 1;index < finalValley.size();index+=3){
        vector<int> realOrder;
        int start=0;
        int peak=0;
        int end=0;
        realOrder.push_back(finalValley[index-1].first);
        realOrder.push_back(finalValley[index-1].first+quantizationLevel);
        realOrder.push_back(finalValley[index].first);
        realOrder.push_back(finalValley[index].first+quantizationLevel);
        realOrder.push_back(finalValley[index+1].first);
        realOrder.push_back(finalValley[index+1].first+quantizationLevel);

        std::sort(realOrder.begin(),realOrder.end());
        //for(unsigned int index = 0;index < realOrder.size();index++)
        //    qDebug()<<realOrder[index];
        vector<int>::iterator it=realOrder.begin();
        if((it=std::find(realOrder.begin(),realOrder.end(),finalValley[index].first))!=realOrder.begin()){
            it--;
            start = *it++;
            peak = *it++;
            end = *it++;
        }else{
//            if(realOrder[0]==realOrder[1]){
//                start = realOrder[0];
//                peak = realOrder[1];
//                end = realOrder[2];
//            }else{
            it=std::find(realOrder.begin(),realOrder.end(),finalValley[index].first+quantizationLevel);
            it--;
            start = *it++;
            peak = *it++;
            end = *it++;
//            }
        }
        double tempDir = 0.;
        for(int position = start;position<end+1;++position){
            double temp=abs(peak-position);
            tempDir += temp*temp*histogramD[position%quantizationLevel];
        }

        directionality+=tempDir;
    }
    // calculate sharpness of the peak
//    int nPeaks = finalValley.size()/3;
//    for(unsigned int index = 1;index < finalValley.size();index+=3){
//        double tempDir = 0.;
//        // first valley after peak
//        if(finalValley[index-1].first>finalValley[index].first){
//            for(int position = finalValley[index-1].first;position<quantizationLevel;++position){
//                double temp=quantizationLevel-position+finalValley[index].first;
//                tempDir += temp*temp*histogramD[position];
//            }
//            for(int position = 0;position<finalValley[index+1].first;++position){
//                double temp=position-finalValley[index].first;
//                tempDir += temp*temp*histogramD[position];
//            }
//            directionality+=tempDir;
//            continue;
//        }
//        // second valley before peak
//        if(finalValley[index+1].first<finalValley[index].first){
//            for(int position = finalValley[index-1].first;position<quantizationLevel;++position){
//                double temp=position-finalValley[index].first;
//                tempDir += temp*temp*histogramD[position];
//            }
//            for(int position = 0;position<finalValley[index+1].first;++position){
//                double temp=quantizationLevel+position-finalValley[index].first;
//                tempDir += temp*temp*histogramD[position];
//            }
//            directionality+=tempDir;
//            continue;
//        }
//        for(int position = finalValley[index-1].first;position<finalValley[index+1].first;++position){
//            double temp=position-finalValley[index].first;
//            tempDir += temp*temp*histogramD[position];
//        }
//        directionality+=tempDir;
//    }

    delete [] tempDeltaG;
    delete [] tempTheta;
    delete [] histogramD;
    //qDebug()<<nPeaks*directionality*1./sumHistogramD;
    return nPeaks*directionality*1./sumHistogramD;//*(boost::math::powm1(10,23)+1);
}

int KTamura::getNextValleyPeak(long *histogram, int start, int length,bool bePeak)
{
    static bool bePeakValley = false;
    if(bePeak) bePeakValley = false;
    bePeakValley = !bePeakValley;
    start %= length;
    int tempPos = start;
    for (int index = start; index < start + length; ++index){
        if (bePeakValley){
            if (histogram[index%length]<histogram[(index + 1) % length]){
                for (; index < start + length; ++index){
                    if (histogram[index%length] >= histogram[(index + 1) % length]) break;
                }
                tempPos = index;// %length;
                if (histogram[index%length] == histogram[(index + 1) % length]){
                    for (; index < start + length; ++index){
                        if (histogram[index%length]>histogram[(index + 1) % length]) break;
                    }
                }
                tempPos = ((tempPos + index) / 2) % length;
                break;
            }
        }
        else{
            if (histogram[index%length]>histogram[(index + 1) % length]){
                for (; index < start + length; ++index){
                    if (histogram[index%length] <= histogram[(index + 1) % length]) break;
                }
                tempPos = index;//%length;
                if (histogram[index%length] == histogram[(index + 1) % length]){
                    for (; index < start + length; ++index){
                        if (histogram[index%length]<histogram[(index + 1) % length]) break;
                    }
                }
                tempPos = ((tempPos + index) / 2) % length;
                break;
            }
        }
    }
    return tempPos;
}

double KTamura::getAverageGray(float * image, unsigned long lineLength, unsigned long lX, unsigned long lY, long size)
{
    double sumGray = 0.;
    for(unsigned long nY = lY-size/2; nY < lY+size/2; ++nY){
        for(unsigned long nX = lX-size/2; nX < lX+size/2; ++nX){
            sumGray += image[lineLength*nY+nX];
        }
    }
    sumGray = sumGray/(size*size);
    return sumGray;
}

QString KTamura::getDirRoot(QString filename)
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

QString KTamura::getSVMString(int start)
{
    QString temp("");
    for(int index= start;index<start+3;++index){
        temp+=QString("%1:%%2 ").arg(index).arg(index-start+1);
    }
    return QString(temp).arg(m_dCoarseness).arg(m_dContrast).arg(m_dDirectionality);
}
