#include "kmultisplit.h"

#include "common.h"

//#define TIXML_USE_STL

#include "cpl_conv.h" // for CPLMalloc()
#include "tinyxml.h"

#include "kutility.h"
#include "kprogressbar.h"
#include "kwaitbar.h"
#include <QString>
#include <QDebug>

#include <boost/math/special_functions/powm1.hpp>
#include <cmath>

#include <iterator>
#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

KMultiSplit::~KMultiSplit()
{
    CPLFree(m_imgBuff);
    for(int bandIndex = 0;bandIndex<m_nBand;++bandIndex){
        CPLFree(m_orgImgBuff[bandIndex]);
    }
    CPLFree(m_orgImgBuff);
    CPLFree(labels);
    CPLFree(m_adfMinMax);
}

KMultiSplit::KMultiSplit(QString input, QString output, QString labelOut, KMultiSplit::K_OutTypes type):
    m_sInput(input),
    m_sOutPut(output),
    m_sOutPutLabel(labelOut),
    m_tTypeOut(type)
{
    m_maxScale = (std::numeric_limits<float>::max)()/10.;
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

    // make full use of cache
    long orgIndex=0;
    for(long index = 0;index<m_nBand*m_iXSize*m_iYSize;index+=m_nBand,orgIndex++){
        for(long temp=0;temp<m_nBand;++temp){
            m_imgBuff[index+temp]=m_orgImgBuff[temp][orgIndex];
        }
    }
//    m_tempRegionVec.clear();
    memset(labels,-1,m_iXSize*m_iYSize*sizeof(long));
//    // TODO:leave to be confirmed
    //    m_quickSplitThres=1.02;
    m_sOutPut = m_sOutPut.left(m_sOutPut.lastIndexOf('.'));
    m_sOutPutLabel = m_sOutPutLabel.left(m_sOutPutLabel.lastIndexOf('.'));

    m_extName = m_sInput.right(m_sInput.length()-m_sInput.lastIndexOf('.'));
    const char *pszFormat = piDataset->GetDriverName();
    m_poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if( m_poDriver == NULL )
    {
        std::cout<<"KMultiSplit:GetGDALDriverManager failed!"<<std::endl;
        exit(1);
    }
    if(!CSLFetchBoolean( m_poDriver->GetMetadata(), GDAL_DCAP_CREATE, FALSE ) )
    {
        m_poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
        m_extName = ".tif";
    }

    m_adfMinMax = (float *) CPLMalloc(sizeof(float)*2*m_nBand);
    GDALRasterBand * tempBand = NULL;
    int tempSuccess;
    for(int index = 0;index<m_nBand;++index){
        tempBand = piDataset->GetRasterBand( index+1 );
        m_adfMinMax[2*index] = tempBand->GetMinimum( &tempSuccess );
        m_adfMinMax[2*index+1] = tempBand->GetMaximum( &tempSuccess );
    }
    GDALClose(piDataset);
}

void KMultiSplit::quickSplit(float splitThres)
{
    std::unordered_map<long,KRegion> tempRegioMap;
    KProgressBar progressBar("QuickSplit",m_iYSize*m_iXSize,80);
    K_PROGRESS_START(progressBar);

    for(int iY = 0;iY<m_iYSize;++iY){
        for(int iX = 0;iX<m_iXSize;++iX){
            KSLE sle(iY,iX,iX);
            //long id = KUtility::getRegionIndex();
            long curID = iY*m_iXSize+iX;
            long leftID = -1;
            long upID = -1;
            std::unordered_map<long,KRegion>::iterator leftIt = tempRegioMap.end();
            std::unordered_map<long,KRegion>::iterator upIt = tempRegioMap.end();
            if(iY>0){
                upID = labels[(iY-1)*m_iXSize+iX];
                upIt = tempRegioMap.find(upID);
            }
            if(iX>0){
                leftID = labels[curID-1];
                leftIt = tempRegioMap.find(leftID);
            }

            KRegion reg(curID);
            reg.pushLine(sle);
            // left and up are the same region

            if(leftID == upID){
                if(-1 == leftID || getQuickColorDiff(upIt->second,reg)>splitThres){
                    labels[curID]=curID;
                    tempRegioMap.insert(std::make_pair(curID,reg));
                }else{
                    upIt->second += reg;
                    labels[curID]=upID;
                }
            }else{
                float leftDiff = 0.;
                float upDiff = 0.;
                if(leftID != -1 && upID != -1){
                    leftDiff = getQuickColorDiff(reg,leftIt->second);
                    upDiff = getQuickColorDiff(upIt->second,reg);
                    if(leftDiff>splitThres&&upDiff>splitThres){
                        labels[curID]=curID;
                        tempRegioMap.insert(std::make_pair(curID,reg));
                    }else{
                        if(leftDiff<=splitThres&&upDiff<=splitThres){
                            if(leftDiff>upDiff){
                                upIt->second += reg;
                                labels[curID]=upID;
                            }else{
                                if(leftDiff<upDiff){
                                    leftIt->second += reg;
                                    labels[curID]=leftID;
                                }else{
                                    if(getQuickColorDiff(upIt->second,leftIt->second)<=splitThres){
                                        upIt->second += leftIt->second;
                                        upIt->second += reg;
                                        labels[curID]=upID;
                                        tempRegioMap.erase(leftIt);
                                        for(int tempIndex=0;tempIndex<curID;++tempIndex){
                                            if(labels[tempIndex]==leftID){
                                                labels[tempIndex]=upID;
                                            }
                                        }
                                    }else{
                                        upIt->second += reg;
                                        labels[curID]=upID;
                                    }
                                }
                            }
                        }else{
                            if(leftDiff<=splitThres){
                                leftIt->second += reg;
                                labels[curID]=leftID;
                            }else{
                                upIt->second += reg;
                                labels[curID]=upID;
                            }
                        }
                    }
                }else if(upID == -1){
                    //qDebug()<<leftID<<upID;
                    leftDiff = getQuickColorDiff(reg,leftIt->second);
                    if(leftDiff<=splitThres){
                        leftIt->second += reg;
                        labels[curID]=leftID;
                    }else{
                        labels[curID]=curID;
                        tempRegioMap.insert(std::make_pair(curID,reg));
                    }
                }else{
                    upDiff = getQuickColorDiff(upIt->second,reg);
                    if(upDiff<=splitThres){
                        upIt->second += reg;
                        labels[curID]=upID;
                    }else{
                        labels[curID]=curID;
                        tempRegioMap.insert(std::make_pair(curID,reg));
                    }
                }
            }// end of not same region merge
//            if(-1==labels[iY*m_iXSize+iX]){
//                qDebug()<<"up"<<upID;
//                qDebug()<<"left"<<leftID;
//                qDebug()<<"cur"<<curID;
//            }
            progressBar.autoUpdate();
        }

        // new: move some finished region to another vector
        // old: move some region to RAG
        for(std::unordered_map<long,KRegion>::iterator it = tempRegioMap.begin();it != tempRegioMap.end();){
            if((it->second).getMaxLine()<iY && !((it->second).isSinglePixel())){
                // just copy to avoid invalid iterator
                m_mapRegion.insert(*it);
                it = tempRegioMap.erase(it);
            }else{
                ++it;
            }
        }
        // delete items in the source vector
        //for(std::vector<KRegion>::iterator it = m_vecRegion.begin();it != m_vecRegion.end();++it){
        //    tempRegionVec.erase(std::find(tempRegionVec.begin(),tempRegionVec.end(),*it));
        //}
    }

    K_PROGRESS_END(progressBar);
    // move all region to the finish vector
    for(std::unordered_map<long,KRegion>::iterator it = tempRegioMap.begin();it != tempRegioMap.end();++it){
        m_mapRegion.insert(*it);
    }
    // merge single item
    KWaitBar waitBar("MergeSingle",4,20);
    K_WAITBAR_START(waitBar);
    for(std::unordered_map<long,KRegion>::iterator it = m_mapRegion.begin();it != m_mapRegion.end();){
        if((it->second).isSinglePixel()){
            list<KSLE>::const_iterator its = ((it->second).getLists()).begin();
            int lines = its->getLine();
            int cols = its->getStartCol();
            long leftID = -1;
            long rightID = -1;
            long upID = -1;
            long downID = -1;
            float leftDiff = (std::numeric_limits<float>::max)();
            float rightDiff = (std::numeric_limits<float>::max)();
            float upDiff = (std::numeric_limits<float>::max)();
            float downDiff = (std::numeric_limits<float>::max)();
            std::unordered_map<long,KRegion>::iterator minDiffIt = m_mapRegion.begin();
            float minDiff = (std::numeric_limits<float>::max)();
            if(cols>0){
                leftID = labels[lines*m_iXSize+cols-1];
            }
            if(cols<m_iXSize-1){
                rightID = labels[lines*m_iXSize+cols+1];
            }
            if(lines>0){
                upID = labels[(lines-1)*m_iXSize+cols];
            }
            if(lines<m_iYSize-1){
                downID = labels[(lines+1)*m_iXSize+cols];
            }
            if(leftID != -1){
                std::unordered_map<long,KRegion>::iterator itTemp = m_mapRegion.find(leftID);
                leftDiff=getQuickColorDiff(itTemp->second,it->second);
                if(minDiff>leftDiff){
                    minDiff=leftDiff;
                    minDiffIt=itTemp;
                }
            }
            if(rightID != -1){
                std::unordered_map<long,KRegion>::iterator itTemp = m_mapRegion.find(rightID);
                rightDiff=getQuickColorDiff(itTemp->second,it->second);
                if(minDiff>rightDiff){
                    minDiff=rightDiff;
                    minDiffIt=itTemp;
                }
            }
            if(upID != -1){
                std::unordered_map<long,KRegion>::iterator itTemp = m_mapRegion.find(upID);
                upDiff=getQuickColorDiff(itTemp->second,it->second);
                if(minDiff>upDiff){
                    minDiff=upDiff;
                    minDiffIt=itTemp;
                }
            }
            if(downID != -1){
                std::unordered_map<long,KRegion>::iterator itTemp = m_mapRegion.find(downID);
                downDiff=getQuickColorDiff(itTemp->second,it->second);
                if(minDiff>downDiff){
                    minDiff=downDiff;
                    minDiffIt=itTemp;
                }
            }
            (minDiffIt->second)+=(it->second);
            labels[lines*m_iXSize+cols]=(minDiffIt->second).getID();
            it = m_mapRegion.erase(it);
        }else{
            ++it;
        }
        waitBar.update();
    }
    K_WAITBAR_END(waitBar);
    // TODO:move to RAG
    buildRAG();
    saveSplit("0");
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
    double tempSum = meanSum*lPointSum*rPointSum/(lPointSum+rPointSum);
    if(tempSum==0.) return 0.;
    //qDebug()<<lPointSum<<rPointSum<<meanSum;
    //qDebug()<<"tempSum"<<tempSum;
    long tempInt = (boost::math::powm1(tempSum,0.5)+1.)*10000.;
    return tempInt/10000.;
}

void KMultiSplit::saveSplit(QString level)
{
    if(OutPic == m_tTypeOut){
        float **tempImgBuff;
        tempImgBuff = (float **) CPLMalloc(sizeof(float *)*m_nBand);
        for(int index = 0;index<m_nBand;++index){
            tempImgBuff[index]=(float *) CPLMalloc(sizeof(float)*m_iXSize*m_iYSize);
        }
        QString tempName = m_sOutPut+"-"+level+m_extName;
        for(int index = 0;index<m_nBand;++index){
            memcpy(tempImgBuff[index],m_orgImgBuff[index],sizeof(float)*m_iXSize*m_iYSize);
        }
        for(int iY = 0;iY<m_iYSize;++iY){
            for(int iX = 0;iX<m_iXSize;++iX){
                int tempIndex = iY*m_iXSize+iX;
                long curID = labels[tempIndex];
                long upID = curID;
                long leftID = curID;
                if(iX>0){
                    leftID = labels[tempIndex-1];
                }
                if(iY>0){
                    upID = labels[(iY-1)*m_iXSize+iX];
                }
                if(upID!=curID || curID!=leftID){
                    if(m_nBand==1){
                        tempImgBuff[0][tempIndex]=0.;
                    }else{
                        tempImgBuff[0][tempIndex]=0.;
                        tempImgBuff[1][tempIndex]=255.;
                        tempImgBuff[2][tempIndex]=0.;
                    }
                }else{
                    if(m_nBand==1){
                        tempImgBuff[0][tempIndex]=tempImgBuff[0][tempIndex]*255./(m_adfMinMax[1]-m_adfMinMax[0]);
                    }else{
                        tempImgBuff[0][tempIndex]=tempImgBuff[0][tempIndex]*255./(m_adfMinMax[1]-m_adfMinMax[0]);
                        tempImgBuff[1][tempIndex]=tempImgBuff[1][tempIndex]*255./(m_adfMinMax[3]-m_adfMinMax[2]);
                        tempImgBuff[2][tempIndex]=tempImgBuff[2][tempIndex]*255./(m_adfMinMax[5]-m_adfMinMax[4]);
                    }
                }
            }
        }
        GDALDataset * poutDataset = m_poDriver->Create(tempName.toUtf8().data(),m_iXSize,m_iYSize,3,GDT_Byte,0);
        for(int bandIndex = 0;bandIndex<m_nBand;++bandIndex){
            GDALRasterBand * poutBand = poutDataset->GetRasterBand(bandIndex+1);
            poutBand->RasterIO( GF_Write, 0, 0, m_iXSize,m_iYSize, tempImgBuff[bandIndex], m_iXSize,m_iYSize, GDT_Float32, 0, 0 );
            poutBand->FlushCache();
            poutDataset->FlushCache();
            CPLFree(tempImgBuff[bandIndex]);
        }
        GDALClose(poutDataset);
        CPLFree(tempImgBuff);


    }else{
        QString tempName = m_sOutPut+"-"+level+".xml";
        TiXmlDocument doc;

        TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "", "" );
        doc.LinkEndChild(decl);

        TiXmlElement * root = new TiXmlElement( "RegionList" );
        doc.LinkEndChild(root);

        TiXmlComment * comment = new TiXmlComment();
        QString temp = " The RegionList of file: "+m_sInput+" ";
        //qDebug()<<temp;
        comment->SetValue( temp.toUtf8().constData() );

        root->LinkEndChild(comment);

        TiXmlElement * regionDescription = new TiXmlElement("RegionDescription");
        root->LinkEndChild(regionDescription);
        int totalNum = boost::num_vertices(m_RAG);
        regionDescription->SetAttribute("total",totalNum);

        TiXmlElement * region;

        vertexIterator vi, vi_end;
        indexMap tempIndexMap = boost::get(boost::vertex_index_t(), m_RAG);
        boost::tie(vi,vi_end) = boost::vertices(m_RAG);
        std::vector<std::tuple<int,int>> tempVec;
        for(;vi!=vi_end;++vi){
            int idTemp = tempIndexMap[*vi];
            float height=0.;
            float width=0.;
            std::unordered_map<long,KRegion>::iterator it = m_mapRegion.find(static_cast<long>(idTemp));
            region = new TiXmlElement("Region");
            regionDescription->LinkEndChild(region);
            region->SetAttribute("name", "default");
            region->SetAttribute("id", idTemp);
            std::tie(height,width)=getMinArea(it->second,tempVec);
            region->SetDoubleAttribute("height", height);
            region->SetDoubleAttribute("width", width); // floating point attrib
        }

//drawSquare(tempVec);
        doc.SaveFile(tempName.toUtf8().constData());
    }
    // save label array
    float * tempLabel=(float *) CPLMalloc(sizeof(float)*m_iXSize*m_iYSize);
    for(int index = 0;index<m_iXSize*m_iYSize;++index){
        tempLabel[index] = labels[index];
    }
    GDALDriver *tempDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    QString labelName = m_sOutPutLabel+"-"+level+m_extName;
    GDALDataset * poDataset = tempDriver->Create(labelName.toUtf8().data(),m_iXSize,m_iYSize,1,GDT_Int32,0);
    GDALRasterBand * poBand = poDataset->GetRasterBand(1);
    poBand->RasterIO( GF_Write, 0, 0, m_iXSize,m_iYSize, tempLabel, m_iXSize,m_iYSize, GDT_Float32, 0, 0 );
    poBand->FlushCache();
    poDataset->FlushCache();
    GDALClose(poDataset);
    CPLFree(tempLabel);
}

void KMultiSplit::buildRAG()
{
    int * tempValid=(int *) CPLMalloc(sizeof(int)*m_iXSize*m_iYSize);
    memset(tempValid,1,m_iXSize*m_iYSize*sizeof(int));
    KProgressBar progressBar("buildRAG",m_mapRegion.size()*2,80);
    K_PROGRESS_START(progressBar);
    for(std::unordered_map<long,KRegion>::iterator it = m_mapRegion.begin();it != m_mapRegion.end();++it){
        std::set<long> tempVecID;
        long curID = (it->second).getID();
        const list<KSLE> tempList = (it->second).getLists();
        // each region
        for(list<KSLE>::const_iterator itSLE = tempList.begin();itSLE != tempList.end();++itSLE){
            int line = itSLE->getLine();
            int start = itSLE->getStartCol();
            int end = itSLE->getEndCol();
            // handle each SLE
            long tempID = -1;
            int tempIndex = -1;
            if(start!=0){
                tempIndex = line*m_iXSize+start-1;
                if(tempValid[tempIndex] && labels[tempIndex] != curID){
                    tempValid[line*m_iXSize+start]=0;
                    tempID = labels[tempIndex];
                    tempVecID.insert(tempID);
                }
            }
            if(end!=m_iXSize-1){
                tempIndex = line*m_iXSize+end+1;
                if(tempValid[tempIndex] && labels[tempIndex] != curID){
                    tempValid[line*m_iXSize+end]=0;
                    tempID = labels[tempIndex];
                    tempVecID.insert(tempID);
                }
            }
            for(int pixel = start+1;pixel<end;++pixel){
                long downID = -1;
                long upID = -1;
                int tempPos = -1;

                if(line==0){
                    if(line!=m_iYSize-1){
                        tempPos = (line+1)*m_iXSize+pixel;
                        if(tempValid[tempPos] && labels[tempPos] != curID) downID = labels[tempPos];
                    }
                }else{
                    if(line==m_iYSize-1){
                        tempPos = (line-1)*m_iXSize+pixel;
                        if(tempValid[tempPos] && labels[tempPos] != curID) upID = labels[tempPos];
                    }else{
                        tempPos = (line+1)*m_iXSize+pixel;
                        if(tempValid[tempPos] && labels[tempPos] != curID) downID = labels[tempPos];
                        tempPos = (line-1)*m_iXSize+pixel;
                        if(tempValid[tempPos] && labels[tempPos] != curID) upID = labels[tempPos];
                    }
                }
                if(-1 != upID || -1 != downID){
                    tempValid[line*m_iXSize+pixel]=0;
                    if(-1 != upID) tempVecID.insert(upID);
                    if(-1 != downID) tempVecID.insert(downID);
                }
            }
        }
        progressBar.autoUpdate();
        // link this region
        RAGraph::vertex_descriptor v1 = getVertex((it->second).getID());
        for(std::set<long>::iterator itSet = tempVecID.begin();itSet != tempVecID.end();++itSet){
            std::unordered_map<long,KRegion>::iterator itReg = m_mapRegion.find(*itSet);
            //qDebug()<<*itSet;
            RAGraph::vertex_descriptor v2 = getVertex(*itSet);
            //indexMap tempIndexMap = boost::get(boost::vertex_index_t(), m_RAG);
            //qDebug()<<tempIndexMap[v1]<<tempIndexMap[v2];
            boost::add_edge(v1, v2, getRegionDiff(it->second,itReg->second), m_RAG);
        }
        progressBar.autoUpdate();
    }
    K_PROGRESS_END(progressBar);
    CPLFree(tempValid);
}

void KMultiSplit::mergeIn(long &des, long &src)
{
    std::unordered_map<long,KRegion>::iterator itDes = m_mapRegion.find(des);
    std::unordered_map<long,KRegion>::iterator itSrc = m_mapRegion.find(src);
    assert(itDes != m_mapRegion.end() && itSrc != m_mapRegion.end() && itSrc != m_mapRegion.end());

    vertexIterator vi, vi_end;
    RAGraph::vertex_descriptor v_des,v_src;
    indexMap tempIndexMap = boost::get(boost::vertex_index_t(), m_RAG);
    boost::tie(vi,vi_end) = boost::vertices(m_RAG);
    for(;vi!=vi_end;++vi){
        if(tempIndexMap[*vi]==src){
            // src point find first
            v_src = *vi;
            for(;vi!=vi_end;++vi){
                if(tempIndexMap[*vi]==des){
                    v_des = *vi;
                    break;
                }
            }
            break;
        }else if(tempIndexMap[*vi]==des){
            v_des = *vi;
            for(;vi!=vi_end;++vi){
                if(tempIndexMap[*vi]==src){
                    v_src = *vi;
                    break;
                }
            }
            break;
        }
    }
    // there's at least one point failed to locate
    if(vi==vi_end){
        return;
    }
    RAGraph::adjacency_iterator vit, vend;
    std::tie(vit, vend) = boost::adjacent_vertices(v_src, m_RAG);
    // change the link graph
    for(;vit != vend;++vit){
        boost::add_edge(v_des, *vit, 0, m_RAG);
    }
    boost::clear_vertex(v_src, m_RAG);
    boost::remove_vertex(v_src, m_RAG);
    // merge the real region
    (itDes->second) += (itSrc->second);
    m_mapRegion.erase(itSrc);
    // change region label
    for(int index = 0;index<m_iXSize*m_iYSize;++index){
        if(labels[index] == src){
            labels[index] = des;
        }
    }
    // change edge's weight
    tempIndexMap = boost::get(boost::vertex_index_t(), m_RAG);
    weightMap tempWeightMap = boost::get(boost::edge_weight_t(),m_RAG);
    RAGraph::out_edge_iterator eit, eend;
    std::tie(eit, eend) = boost::out_edges(v_des, m_RAG);
    for(;eit != eend;++eit){
        RAGraph::vertex_descriptor v_target,v_source;
        v_target = boost::target(*eit, m_RAG);
        v_source = boost::source(*eit, m_RAG);
        std::unordered_map<long,KRegion>::iterator itTarget = m_mapRegion.find(tempIndexMap[v_target]);
        std::unordered_map<long,KRegion>::iterator itSource = m_mapRegion.find(tempIndexMap[v_source]);
        tempWeightMap[*eit]=getRegionDiff(itTarget->second,itSource->second);
    }
}

void KMultiSplit::mergeIn(RAGraph::edge_descriptor & edge)
{
    RAGraph::vertex_descriptor v_des,v_src;
    //std::vector<RAGraph::vertex_descriptor> tempVec;

    v_des = boost::target(edge, m_RAG);
    v_src = boost::source(edge, m_RAG);

    indexMap tempIndexMap = boost::get(boost::vertex_index_t(), m_RAG);

    long des = tempIndexMap[v_des];
    long src = tempIndexMap[v_src];

    std::unordered_map<long,KRegion>::iterator itDes = m_mapRegion.find(des);
    std::unordered_map<long,KRegion>::iterator itSrc = m_mapRegion.find(src);
    assert(itDes != m_mapRegion.end() && itSrc != m_mapRegion.end());

    RAGraph::adjacency_iterator vit, vend;
    //RAGraph::adjacency_iterator vdit, vdend;
    std::tie(vit, vend) = boost::adjacent_vertices(v_src, m_RAG);
    //std::tie(vdit, vdend) = boost::adjacent_vertices(v_des, m_RAG);
    // change the link graph
//    for(;vit != vend;++vit){
//         qDebug()<<tempIndexMap[v_des]<<tempIndexMap[*vit];
//    }
    for(;vit != vend;++vit){
        // default 0
        //qDebug()<<tempIndexMap[v_des]<<tempIndexMap[*vit];
        //if(std::find(vdit,vdend,*vit)==vdend)
        //qDebug()<<v_des<<*vit;
        //tempVec.push_back(*vit);
//        if(10076==tempIndexMap[v_des])
//        {
//            qDebug()<<tempIndexMap[v_des]<<tempIndexMap[*vit];
//            //if(9895==tempIndexMap[*vit]){
//                std::tie(vdit, vdend) = boost::adjacent_vertices(*vit, m_RAG);
//                for(;vdit!=vdend;++vdit)qDebug()<<tempIndexMap[*vdit];
//           // }
//        }
        boost::add_edge(v_des, *vit, 0., m_RAG);
    }
    //for(RAGraph::vertex_descriptor & vertexs:tempVec)boost::add_edge(v_des, vertexs, 0., m_RAG);

    boost::clear_vertex(v_src, m_RAG);
    boost::remove_vertex(v_src, m_RAG);
    // merge the real region
    (itDes->second) += (itSrc->second);
    m_mapRegion.erase(itSrc);
    // change region label
    for(int index = 0;index<m_iXSize*m_iYSize;++index){
        if(labels[index] == src){
            labels[index] = des;
        }
    }
    // change edge's weight
    tempIndexMap = boost::get(boost::vertex_index_t(), m_RAG);
    weightMap tempWeightMap = boost::get(boost::edge_weight_t(),m_RAG);
    RAGraph::out_edge_iterator eit, eend;
    //qDebug()<<boost::num_edges(m_RAG);
    std::tie(eit, eend) = boost::out_edges(v_des, m_RAG);
    for(;eit != eend;++eit){
        RAGraph::vertex_descriptor v_target,v_source;
        v_target = boost::target(*eit, m_RAG);
        v_source = boost::source(*eit, m_RAG);
        std::unordered_map<long,KRegion>::iterator itTarget = m_mapRegion.find(tempIndexMap[v_target]);
        std::unordered_map<long,KRegion>::iterator itSource = m_mapRegion.find(tempIndexMap[v_source]);
        tempWeightMap[*eit]=getRegionDiff(itTarget->second,itSource->second);
    }
}

void KMultiSplit::runMultiSplit(float startScale, float endScale, float alpha, bool beClear, RAGraph::edges_size_type minAreas)
{
    static int level = 1;
    if(beClear) level = 1;

    float preScale = 0.;
    float curScale = 0.;
    float minDiff = 0.;

    m_maxScale = endScale;
    KWaitBar waitBar("MultiSplit",4,10);
    K_WAITBAR_START(waitBar);
    while(true){
        if(boost::num_edges(m_RAG)<minAreas) break;
        std::tie(minDiff,curScale)=getGraphScale();
        //qDebug()<<"min"<<minDiff;
        //qDebug()<<"curScale"<<curScale;
        if(curScale>startScale) break;
        // merge
        mergeCurScale(minDiff);
        waitBar.update();
    }
    preScale = curScale;
    saveSplit(QString("%1").arg(level++));
    waitBar.update();
    while(true){
        if(boost::num_edges(m_RAG)<minAreas){ saveSplit(QString("%1").arg(level++)); break; }
        if(curScale<alpha*preScale){
            std::tie(minDiff,curScale)=getGraphScale();
            //qDebug()<<"min"<<minDiff;
            //qDebug()<<"curScale"<<curScale;
            // merge
            mergeCurScale(minDiff);
            waitBar.update();
        }else{
            preScale = curScale;
            saveSplit(QString("%1").arg(level++));
            waitBar.update();
            if(curScale>endScale){ break; }
        }
        if(curScale>endScale){ saveSplit(QString("%1").arg(level++)); break; }
    }
    K_WAITBAR_END(waitBar);
}

void KMultiSplit::testXMLOutput()
{
//    <?xml version="1.0" ?>
//    <RegionList>
//        <!-- RegionList of file: C://test.jpg -->
//        <RegionDescription total="10">
//            <Region name="nothing" id="1" height="5" width="123.456000" />
//        </RegionDescription>
//    </RegionList>
    TiXmlDocument doc;

    TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "", "" );
    doc.LinkEndChild(decl);

    TiXmlElement * root = new TiXmlElement( "RegionList" );
    doc.LinkEndChild(root);

    TiXmlComment * comment = new TiXmlComment();
    QString temp = " The RegionList of file: "+m_sInput+" ";
    //qDebug()<<temp;
    comment->SetValue( temp.toUtf8().constData() );

    root->LinkEndChild(comment);

    TiXmlElement * regionDescription = new TiXmlElement("RegionDescription");
    root->LinkEndChild(regionDescription);
    regionDescription->SetAttribute("total",10);

    TiXmlElement * region;
    region = new TiXmlElement("Region");
    regionDescription->LinkEndChild(region);
    region->SetAttribute("name", "nothing");
    region->SetAttribute("id", 1);
    region->SetAttribute("height", 5);
    region->SetDoubleAttribute("width", 123.456); // floating point attrib

    doc.SaveFile("C://test.xml");
}

void KMultiSplit::testXMLInput()
{
//    <?xml version="1.0" ?>
//    <RegionList>
//        <!-- RegionList of file: C://test.jpg -->
//        <RegionDescription total="10">
//            <Region name="nothing" id="1" height="5" weight="123.456000" />
//        </RegionDescription>
//    </RegionList>
    TiXmlDocument doc("C://test.xml");
    if (!doc.LoadFile(TIXML_ENCODING_UTF8)) return;

    TiXmlHandle hRoot(&doc);

    int totalRegions = 0;
    double tempDouble = 0.;
    int id=0;
    int height = 0;
    int width = 0;

    TiXmlElement* pRegionNode=hRoot.FirstChild("RegionList").FirstChild("RegionDescription").FirstChild().Element();
    hRoot.FirstChild("RegionList").FirstChild("RegionDescription").Element()->QueryIntAttribute("total",&totalRegions);
    qDebug()<<"totalRegions"<<totalRegions;

    for( ; pRegionNode; pRegionNode=pRegionNode->NextSiblingElement())
    {
        const char *pName=pRegionNode->Attribute("name");
        if (pName) qDebug()<<pName;

        pRegionNode->QueryIntAttribute("id", &id);
        pRegionNode->QueryIntAttribute("height", &height);
        pRegionNode->QueryDoubleAttribute("width",&tempDouble);

        qDebug()<<"id"<<id<<"height"<<height<<"width"<<width<<"tempDouble"<<tempDouble;
    }

}

void KMultiSplit::mergeCurScale(float minDiff)
{
    std::vector<RAGraph::edge_descriptor> edges;
    std::vector<RAGraph::vertex_descriptor> points;
    RAGraph::edge_iterator eit, eend;
    std::tie(eit, eend) = boost::edges(m_RAG);
    // get all points needed to be merged
    weightMap tempWeightMap = boost::get(boost::edge_weight_t(),m_RAG);
    for(;eit != eend;++eit){
        float temp = tempWeightMap[*eit];
        RAGraph::vertex_descriptor v_target,v_source;
        v_target = boost::target(*eit, m_RAG);
        v_source = boost::source(*eit, m_RAG);
        if(isFloatEqual(minDiff,temp)){
            if(std::find(points.begin(),points.end(),v_target) == points.end()
                    &&std::find(points.begin(),points.end(),v_source) == points.end()){
                points.push_back(v_target);
                points.push_back(v_source);
                edges.push_back(*eit);
            }
        }
    }
    // merge
    for(RAGraph::edge_descriptor &it : edges) mergeIn(it);
}

int KMultiSplit::getColorVariance(KRegion &reg, float *colorVar)
{
    int pointSum=0;
    float *colorAvr = new float[3];
    for(int index = 0;index<m_nBand;++index){ colorVar[index]=0.; colorAvr[index]=0.; }

    list<KSLE> ltempList=reg.getLists();
    for(list<KSLE>::iterator it = ltempList.begin();it != ltempList.end();++it){
        int line = it->getLine();
        int start = it->getStartCol();
        int end = it->getEndCol();
        for(int band = 0;band<m_nBand;++band){
            for(int index = start;index<=end;++index){
                int tempIndex = line*m_iXSize+index;
                pointSum++;
                colorAvr[band]+=m_orgImgBuff[band][tempIndex];
                colorVar[band]+=m_orgImgBuff[band][tempIndex]*m_orgImgBuff[band][tempIndex];
            }
        }
    }
    for(int band = 0;band<m_nBand;++band){
        double tempAbs = (std::numeric_limits<double>::max)();
        if(pointSum) tempAbs = std::fabs(colorVar[band]-colorAvr[band]*colorAvr[band])/pointSum;
        if(tempAbs==0.){ colorVar[band]=0.; continue; }
        //qDebug()<<"tempAbs"<<tempAbs;
        colorVar[band] = boost::math::powm1(tempAbs,0.5)+1.;
    }
    delete [] colorAvr;
    return pointSum;
}
// height, width ; left-top, left-bottom, right-top, right-bottom---original
std::tuple<float, float> KMultiSplit::getMinArea(KRegion &reg,std::vector<std::tuple<int,int>>& cornerVec)
{
    std::vector<std::tuple<int,int>> vecPoint;
    float minArea = (std::numeric_limits<float>::max)();
    std::tuple<float, float, float, float, float, float> pointAtMin;
    float degreeAtMin=0.;
    const float degreeStep = M_PI/20.;
    list<KSLE> tempList = reg.getLists();
    cornerVec.clear();
    if(tempList.size()==1){
        KSLE sle = *(tempList.begin());
        int line=sle.getLine()+1;
        int start=sle.getStartCol()+1;
        int end = sle.getEndCol()+1;
        cornerVec.push_back(std::make_tuple(start,line));
        cornerVec.push_back(std::make_tuple(start,line));
        cornerVec.push_back(std::make_tuple(end,line));
        cornerVec.push_back(std::make_tuple(end,line));
        //qDebug()<<"test...";
        return std::make_tuple(1,end-start+1);
    }
    for(KSLE sle : tempList){
        int line=sle.getLine()+1;
        int start=sle.getStartCol()+1;
        int end = sle.getEndCol()+1;
        vecPoint.push_back(std::make_tuple(line,start));
        vecPoint.push_back(std::make_tuple(line,end));
    }
    if(vecPoint.size()==0){ return std::make_tuple(0.,0.); }
    for(float theta=0.;theta<M_PI/2.;theta+=degreeStep){
        std::tuple<float, float, float, float, float, float> tempTuple;
        tempTuple=getRotateArea(vecPoint,theta);
        float area = get<0>(tempTuple)*get<1>(tempTuple);
        if(area<minArea){
            minArea = area;
            pointAtMin = tempTuple;
            degreeAtMin = theta;
        }
    }
    std::vector<std::tuple<float,float>> vecRotatedPoint;
    vecRotatedPoint.push_back(std::make_tuple(get<4>(pointAtMin),get<2>(pointAtMin)));
    vecRotatedPoint.push_back(std::make_tuple(get<5>(pointAtMin),get<2>(pointAtMin)));
    vecRotatedPoint.push_back(std::make_tuple(get<5>(pointAtMin),get<3>(pointAtMin)));
    vecRotatedPoint.push_back(std::make_tuple(get<4>(pointAtMin),get<3>(pointAtMin)));
    for(auto _point:vecRotatedPoint){
        float nowTheta = 0.;
        if(get<0>(_point)<=0.) nowTheta=std::atan(get<0>(_point)*-1./get<1>(_point))+M_PI/2.;
        else nowTheta=std::atan(get<1>(_point)/get<0>(_point));
        //qDebug()<<nowTheta;
        //if(nowTheta<0) nowTheta+=M_PI;
        float orgTheta = nowTheta - degreeAtMin;
        float length = get<1>(_point)/std::sin(nowTheta);
        float tempY = length * std::cos(orgTheta) - 1.;
        if(tempY<0.) tempY = 0.;
        if(tempY>m_iYSize-1) tempY=m_iYSize-1;
        float tempX = length * std::sin(orgTheta) - 1.;
        if(tempX<0.) tempX = 0.;
        if(tempX>m_iXSize-1) tempX=m_iXSize-1;

        cornerVec.push_back(std::make_tuple(static_cast<int>(tempY),static_cast<int>(tempX)));
    }
//    if(get<0>(pointAtMin)<0.001){
//        qDebug()<<"degreeAtMin"<<degreeAtMin;
//    }
//qDebug()<<get<0>(pointAtMin)<<get<1>(pointAtMin);
    return std::make_tuple(get<0>(pointAtMin),get<1>(pointAtMin));
}
// height, width, left, right, top, bottom---rotated
std::tuple<float, float, float, float, float, float> KMultiSplit::getRotateArea(std::vector<std::tuple<int,int>> &points, float degree)
{
    std::vector<std::tuple<float,float>> tempVecPoint;

    float left = (std::numeric_limits<float>::max)();
    float top = (std::numeric_limits<float>::max)();
    float right = (std::numeric_limits<float>::min)();
    float bottom = (std::numeric_limits<float>::min)();
    for(auto _point : points){
        float orgTheta =std::atan(get<1>(_point)*1./get<0>(_point));
        float nowTheta = orgTheta + degree;
        float length = get<1>(_point)/std::sin(orgTheta);
        tempVecPoint.push_back(std::make_tuple(length*std::cos(nowTheta),length*std::sin(nowTheta)));
    }
    //qDebug()<<tempVecPoint.size();
    for(auto _point:tempVecPoint){
        float temp = get<1>(_point);
        if(temp>right){
            right = temp;
        }
        if(temp<left){
            left = temp;
        }
    }
    for(auto _point:tempVecPoint){
        float temp = get<0>(_point);
        if(temp>bottom){
            bottom = temp;
        }
        if(temp<top){
            top = temp;
        }
    }
    //if(bottom==top) bottom=top+1.;
    //if(right==right) right=left+1.;
    //qDebug()<<"bottom"<<bottom<<"top"<<top<<"left"<<left<<"right"<<right;
    return std::make_tuple(bottom-top+1.,right-left+1.,left,right,top,bottom);
}

KMultiSplit::RAGraph::vertex_descriptor KMultiSplit::getVertex(long id)
{
    vertexIterator vi, vi_end;
    RAGraph::vertex_descriptor v;
    indexMap tempIndexMap = boost::get(boost::vertex_index_t(), m_RAG);
    boost::tie(vi,vi_end) = boost::vertices(m_RAG);
    for(;vi!=vi_end;++vi){
        if(tempIndexMap[*vi]==id){ break; }
    }
    if(vi==vi_end){
        v = add_vertex(m_RAG);
        tempIndexMap[v]=id;
    }else{ v = *vi; }
    return v;
}

float KMultiSplit::getRegionDiff(KRegion &lhs, KRegion &rhs)
{
    float colorDiff = 0.;
    float shapeDiff = 0.;
    float weightedDiff = 0.;

    int lPointSum = 0;
    int rPointSum = 0;
    int totalPointSum = 0;
    float *lColorVar=new float[m_nBand];
    float *rColorVar=new float[m_nBand];
    float *totalColorVar=new float[m_nBand];

    std::vector<std::tuple<int,int>> tempVecCorner;
    float totalHeight=0.;
    float totalWidth = 0.;
    float lHeight = 0.;
    float lWidth = 0.;
    float rHeight = 0.;
    float rWidth = 0.;

    KRegion tempReg = lhs;
    tempReg += rhs;

    lPointSum = getColorVariance(lhs,lColorVar);
    //qDebug()<<"lPointSum"<<lPointSum;
    rPointSum = getColorVariance(rhs,rColorVar);
    //qDebug()<<"rPointSum"<<rPointSum;
    totalPointSum = getColorVariance(tempReg,totalColorVar);
    float varSum = 0.;
    //qDebug()<<"totalPointSum"<<totalPointSum;
    for(int band=0;band<m_nBand;++band){
        varSum +=totalPointSum*totalColorVar[band]-lPointSum*lColorVar[band]-rPointSum*rColorVar[band];
    }
    //qDebug()<<"colorDiff"<<varSum;
    colorDiff = std::fabs(varSum);

    std::tie(lHeight,lWidth)=getMinArea(lhs,tempVecCorner);
    std::tie(rHeight,rWidth)=getMinArea(rhs,tempVecCorner);
    std::tie(totalHeight,totalWidth)=getMinArea(tempReg,tempVecCorner);
//    if(totalWidth<0.001)qDebug()<<totalWidth;
//    if(lWidth<0.001)qDebug()<<lWidth;
//    if(rWidth<0.001)qDebug()<<rWidth;
    shapeDiff = std::fabs(totalPointSum*totalHeight/totalWidth-lPointSum*lHeight/lWidth-rPointSum*rHeight/rWidth);
//    qDebug()<<"shapeDiff"<<shapeDiff;
    weightedDiff = 0.9*colorDiff + 0.1 * shapeDiff;

    delete [] lColorVar;
    delete [] rColorVar;
    delete [] totalColorVar;

    return weightedDiff;
}

std::tuple<float, float> KMultiSplit::getGraphScale()
{
    float avrDiff = 0.;
    float min_diff = (std::numeric_limits<float>::max)();
    int edge_num = 0;
    RAGraph::edge_iterator eit, eend;
    std::tie(eit, eend) = boost::edges(m_RAG);
    weightMap tempWeightMap = boost::get(boost::edge_weight_t(),m_RAG);
    for(;eit != eend;++eit){
        float temp = tempWeightMap[*eit];
        //if(temp>100000.)qDebug()<<temp;
        if(min_diff>temp) min_diff = temp;
        avrDiff += temp;
        edge_num++;
    }
//qDebug()<<"avrDiff"<<avrDiff;
//qDebug()<<"edge_num"<<edge_num;
    if(edge_num) avrDiff /= edge_num;
    else return std::make_tuple(min_diff,m_maxScale*2.);

    if(avrDiff==0.) return std::make_tuple(min_diff,0.);

    return std::make_tuple(min_diff,boost::math::powm1(avrDiff,0.5)+1.);
}

void KMultiSplit::testMinArea()
{

}

void KMultiSplit::drawSquare(std::vector<std::tuple<int,int> > &src, int band)
{
    static int count = 0;
    GDALDataset *poSrcDS = (GDALDataset *)GDALOpen(m_sInput.toUtf8().constData(), GA_ReadOnly );
    GDALDataset *dataSet;
    //const char *pszFormat = poSrcDS->GetDriverName();
    GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    QString tempOut = m_sOutPut + QString("-square%1").arg(count++) + m_extName;
    dataSet = poDriver->CreateCopy(tempOut.toUtf8().constData(), poSrcDS, FALSE, NULL, NULL, NULL );
    /* Once we're done, close properly the dataset */
    dataSet->FlushCache();
    GDALClose((GDALDatasetH)poSrcDS);

    int width = (penWidth-1)/2;
    std::vector<std::pair<int,int>> temp;
    m_nBand = std::min(m_nBand,3);
    float *pafData = (float *) CPLMalloc(sizeof(float)*m_iXSize*m_iYSize);
    GDALRasterBand * piBand = NULL;
    for(int index = 0;index<m_nBand;++index){
        piBand = dataSet->GetRasterBand(index + 1);
        piBand->RasterIO( GF_Read, 0, 0, m_iXSize, m_iYSize, pafData, m_iXSize, m_iYSize, GDT_Float32, 0, 0 );
        temp.clear();
        for(unsigned int vecIndex = 0;vecIndex<src.size();){
            ++vecIndex;
            temp.push_back(std::make_pair(std::get<1>(src[vecIndex-1]),std::get<0>(src[vecIndex-1])));
            if(vecIndex%4==0){
                temp.push_back(temp[0]);
                for(unsigned int num = 0;num<temp.size()-1;++num){
                    // process each square
                    long cnt = 0;
                    if(std::abs(temp[num].first-temp[num+1].first)<std::abs(temp[num].second-temp[num+1].second)){
                        double dxy = 1.0 * (temp[num+1].first-temp[num].first )/(temp[num+1].second-temp[num].second);
                        if(temp[num].second<temp[num+1].second){
                            for(long pos = temp[num].second;pos<temp[num+1].second;++pos,++cnt) {
                                long tempX = static_cast<long>(temp[num].first + dxy*cnt);
                                if(pos<0||pos>m_iYSize-1||tempX<0||tempX>m_iXSize-1) continue;

                                for(int tempPen = std::max(tempX-width,static_cast<long>(0));tempPen<std::min(tempX+width,static_cast<long>(m_iXSize));++tempPen)
                                {
                                    if(index==band) pafData[pos*m_iXSize+tempPen]=(std::numeric_limits<float>::max)();
                                    else pafData[pos*m_iXSize+tempPen]=0;
                                }
                            }
                        }else{
                            for(long pos = temp[num+1].second;pos<temp[num].second;++pos,++cnt) {
                                long tempX = static_cast<long>(temp[num+1].first + dxy*cnt);
                                if(pos<0||pos>m_iYSize-1||tempX<0||tempX>m_iXSize-1) continue;
                                for(int tempPen = std::max(tempX-width,static_cast<long>(0));tempPen<std::min(tempX+width,static_cast<long>(m_iXSize));++tempPen)
                                {
                                    if(index==band) pafData[pos*m_iXSize+tempPen]=(std::numeric_limits<float>::max)();
                                    else pafData[pos*m_iXSize+tempPen]=0;
                                }
                            }
                        }
                    }else{
                        double dyx = 1.0 * (temp[num+1].second-temp[num].second )/(temp[num+1].first-temp[num].first);
                        if(temp[num].first<temp[num+1].first){
                            for(long pos = temp[num].first;pos<temp[num+1].first;++pos,++cnt) {
                                long tempY = static_cast<long>(temp[num].second + dyx*cnt);
                                if(pos<0||pos>m_iXSize-1||tempY<0||tempY>m_iYSize-1) continue;
                                //pafData[tempY*XSize+pos]=0;
                                for(int tempPen = std::max(tempY-width,static_cast<long>(0));tempPen<std::min(tempY+width,static_cast<long>(m_iYSize));++tempPen)
                                {
                                    if(index==band) pafData[tempPen*m_iXSize+pos]=(std::numeric_limits<float>::max)();
                                    else pafData[tempPen*m_iXSize+pos]=0;
                                }
                            }
                        }else{
                            for(long pos = temp[num+1].first;pos<temp[num].first;++pos,++cnt) {
                                long tempY = static_cast<long>(temp[num+1].second + dyx*cnt);
                                if(pos<0||pos>m_iXSize-1||tempY<0||tempY>m_iYSize-1) continue;
                                for(int tempPen = std::max(tempY-width,static_cast<long>(0));tempPen<std::min(tempY+width,static_cast<long>(m_iYSize));++tempPen)
                                {
                                    if(index==band) pafData[tempPen*m_iXSize+pos]=(std::numeric_limits<float>::max)();
                                    else pafData[tempPen*m_iXSize+pos]=0;
                                }
                            }
                        }
                    }
                }
                temp.clear();
            }
        }

        piBand->RasterIO( GF_Write, 0, 0, m_iXSize, m_iYSize, pafData, m_iXSize, m_iYSize, GDT_Float32, 0, 0 );
        piBand->FlushCache();
    }
    dataSet->FlushCache();
    if( dataSet != NULL )
        GDALClose((GDALDatasetH)dataSet);
    CPLFree(pafData);
}
/*
 * RAGraph::out_edge_iterator eit, eend;
    std::tie(eit, eend) = boost::out_edges(v1, m_RAG);
    for(;eit!=eend;++eit){
        std::cout<<wei[*eit];
        std::cout << name[boost::target(*eit, m_RAG)];
        std::cout << name[boost::source(*eit, m_RAG)];
    }
graph::adjacency_iterator vit, vend;
  std::tie(vit, vend) = boost::adjacent_vertices(topLeft, g);
  std::copy(vit, vend,
    std::ostream_iterator<graph::vertex_descriptor>{std::cout, "\n"});

  graph::out_edge_iterator eit, eend;
  std::tie(eit, eend) = boost::out_edges(topLeft, g);
  std::for_each(eit, eend,
    [&g](graph::edge_descriptor it)
      { std::cout << boost::target(it, g) << '\n'; });
*/

