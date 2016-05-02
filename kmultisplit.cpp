#include "kmultisplit.h"

#include "common.h"
#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()

#include "kutility.h"
#include "kprogressbar.h"
#include <QString>

#include <boost/math/special_functions/powm1.hpp>
#include <algorithm>

KMultiSplit::~KMultiSplit()
{
    CPLFree(m_imgBuff);
    for(int bandIndex = 0;bandIndex<m_nBand;++bandIndex){
        CPLFree(m_orgImgBuff[bandIndex]);
    }
    CPLFree(m_orgImgBuff);
    CPLFree(labels);
}

KMultiSplit::KMultiSplit(QString input, QString output, QString labelOut, KMultiSplit::K_OutTypes type):
    m_sInput(input),
    m_sOutPut(output),
    m_sOutPutLabel(labelOut),
    m_tTypeOut(type)
{
    GDALAllRegister();
    GDALDataset * piDataset = (GDALDataset *) GDALOpen(input.toUtf8().constData(), GA_ReadOnly );
    K_OPEN_ASSERT(piDataset,input.toStdString());

    m_iXSize = piDataset->GetRasterXSize();
    m_iYSize = piDataset->GetRasterYSize();
    m_nBand = piDataset->GetRasterCount();
    GDALRasterBand * piBand = NULL;
    // allocate the memory
    m_orgImgBuff = (float **) CPLMalloc(sizeof(float *)*m_nBand);
    m_imgBuff=(float *) CPLMalloc(sizeof(float)*m_nBand*m_iXSize*m_iYSize);
    for(int index = 0;index<m_nBand;++index){
        m_orgImgBuff[index]=(float *) CPLMalloc(sizeof(float)*m_iXSize*m_iYSize);
    }
    labels = (long *) CPLMalloc(sizeof(long)*m_iXSize*m_iYSize);
    // read the image
    for(int index = 0; index < m_nBand; ++index)
    {
        piBand = piDataset->GetRasterBand(index + 1);

        piBand->RasterIO( GF_Read, 0, 0, m_iXSize, m_iYSize, m_orgImgBuff[index], m_iXSize, m_iYSize
                          , GDT_Float32, 0, 0 );
    }
    GDALClose(piDataset);
    // make full use of cache
    long orgIndex=0;
    for(long index = 0;index<m_nBand*m_iXSize*m_iYSize;index+=m_nBand,orgIndex++){
        for(long temp=0;temp<m_nBand;++temp){
            m_imgBuff[index+temp]=m_orgImgBuff[temp][orgIndex];
        }
    }
//    m_tempRegionVec.clear();
    memset(labels,-1,m_iXSize*m_iYSize);
//    // TODO:leave to be confirmed
    //    m_quickSplitThres=1.02;
}

void KMultiSplit::quickSplit(float splitThres)
{
    std::vector<KRegion> tempRegionVec;
    for(int iY = 0;iY<m_iYSize;++iY){
        for(int iX = 0;iX<m_iXSize;++iX){
            KSLE sle(iY,iX,iX);
            //long id = KUtility::getRegionIndex();
            long curID = iY*m_iXSize+iX;
            long leftID = -1;
            long upID = -1;
            std::vector<KRegion>::iterator leftIt = tempRegionVec.end();
            std::vector<KRegion>::iterator upIt = tempRegionVec.end();
            if(iY>0){
                upID = (iY-1)*m_iXSize+iX;
                KRegion tempReg(upID);
                upIt = std::find(tempRegionVec.begin(),tempRegionVec.end(),tempReg);
            }
            if(iX>0){
                leftID = iY*m_iXSize+iX-1;
                KRegion tempReg(leftID);
                leftIt = std::find(tempRegionVec.begin(),tempRegionVec.end(),tempReg);
            }
            KRegion reg(curID);
            reg.pushLine(sle);
            // left and up are the same region
            if(leftID == upID){
                if(-1 == leftID || getQuickColorDiff(*upIt,reg)>splitThres){
                    labels[iY*m_iXSize+iX]=curID;
                    tempRegionVec.push_back(reg);
                }else{
                    *upIt += reg;
                    labels[iY*m_iXSize+iX]=upID;
                }
            }else{
                float leftDiff = 0.;
                float upDiff = 0.;
                if(leftID != -1 && upID != -1){
                    leftDiff = getQuickColorDiff(reg,*leftIt);
                    upDiff = getQuickColorDiff(*upIt,reg);
                    if(leftDiff>splitThres&&upDiff>splitThres){
                        labels[iY*m_iXSize+iX]=curID;
                        tempRegionVec.push_back(reg);
                    }else{
                        if(leftDiff<=splitThres&&upDiff<=splitThres){
                            if(leftDiff>upDiff){
                                *upIt += reg;
                                labels[iY*m_iXSize+iX]=upID;
                            }else{
                                if(leftDiff<upDiff){
                                    *leftIt += reg;
                                    labels[iY*m_iXSize+iX]=leftID;
                                }else{
                                    if(getQuickColorDiff(*upIt,*leftIt)<=splitThres){
                                        *upIt += *leftIt;
                                        int nowIndex = iY*m_iXSize+iX;
                                        tempRegionVec.erase(leftIt);
                                        for(int tempIndex=0;tempIndex<nowIndex;++tempIndex){
                                            if(labels[tempIndex]==leftID){
                                                labels[tempIndex]=upID;
                                            }
                                        }
                                    }else{
                                        *upIt += reg;
                                        labels[iY*m_iXSize+iX]=upID;
                                    }
                                }
                            }
                        }else{
                            if(leftDiff<=splitThres){
                                *leftIt += reg;
                                labels[iY*m_iXSize+iX]=leftID;
                            }else{
                                *upIt += reg;
                                labels[iY*m_iXSize+iX]=upID;
                            }
                        }
                    }
                }else if(upID == -1){
                    leftDiff = getQuickColorDiff(reg,*leftIt);
                    if(leftDiff<=splitThres){
                        *leftIt += reg;
                        labels[iY*m_iXSize+iX]=leftID;
                    }else{
                        labels[iY*m_iXSize+iX]=curID;
                        tempRegionVec.push_back(reg);
                    }
                }else{
                    upDiff = getQuickColorDiff(*upIt,reg);
                    if(upDiff<=splitThres){
                        *upIt += reg;
                        labels[iY*m_iXSize+iX]=upID;
                    }else{
                        labels[iY*m_iXSize+iX]=curID;
                        tempRegionVec.push_back(reg);
                    }
                }
            }// end of not same region merge
        }
        // move some region to RAG
        for(std::vector<KRegion>::iterator it = tempRegionVec.begin();it != tempRegionVec.end();++it){
            if(it->getMaxLine()<iY && !(it->isSinglePixel())){
                // TODO:move to RAG

            }
        }
    }
    // merge single item
    for(std::vector<KRegion>::iterator it = tempRegionVec.begin();it != tempRegionVec.end();++it){
        if(it->isSinglePixel()){
            list<KSLE>::const_iterator its = (it->getLists()).begin();
            int lines = its->getLine();
            int cols = its->getStartCol();
            long leftID = lines*m_iXSize+cols-1;
            KRegion tempReg(leftID);
        }
    }
    // TODO:move to RAG

}

int KMultiSplit::getQuickColorMean(KRegion &reg, double *colorAvr)
{
    int pointSum=0;
    for(int index = 0;index<m_nBand;++index){ colorAvr[index]=0.; }

    list<KSLE> ltempList=reg.getLists();
    for(list<KSLE>::iterator it = ltempList.begin();it != ltempList.end();++it){
        int line = it->getLine();
        int start = it->getStartCol();
        int end = it->getEndCol();
        for(int band = 0;band<m_nBand;++band){
            for(int index = start;index<=end;++index){
                pointSum++;
                colorAvr[band]+=m_orgImgBuff[band][line*m_iXSize+index];
            }
        }
    }
    for(int band = 0;band<m_nBand;++band){
        colorAvr[band]/=pointSum;
    }
    return pointSum;
}

float KMultiSplit::getQuickColorDiff(KRegion &lhs, KRegion &rhs)
{
    int lPointSum = 0;
    int rPointSum = 0;
    double *lColorAvr=new double[m_nBand];
    double *rColorAvr=new double[m_nBand];

    lPointSum = getQuickColorMean(lhs,lColorAvr);
    rPointSum = getQuickColorMean(rhs,rColorAvr);

    double meanSum = 0.;
    for(int band=0;band<m_nBand;++band){
        double temp = lColorAvr[band]-rColorAvr[band];
        temp = temp*temp;
        meanSum+=temp;
    }
    delete [] lColorAvr;
    delete [] rColorAvr;
    long tempInt = (boost::math::powm1(meanSum*lPointSum*rPointSum/(lPointSum+rPointSum),0.5)+1.)*10000.;
    return tempInt/10000.;
}


