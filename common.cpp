#include "common.h"

// the two datasets are considered to be the same when their files list are common
bool K_CheckDataSetEqu(GDALDataset *set1, GDALDataset *set2)
{
    if(NULL == set1 && NULL == set2) return true;
    if(NULL == set1 || NULL == set2) return false;

    char ** ilist =set1->GetFileList();
    char ** olist =set2->GetFileList();
    int  length = 0;
    bool ret = false;

    if((length = CSLCount (ilist)) == CSLCount (olist))
    {
        int index = 0;
        for(;index < length;++index)
        {
            if(0 != strcmp(ilist[index],olist[index]))
            {
                break;
            }
        }
        if(length == index)
        {
            ret = true;
        }
    }

    CSLDestroy (ilist);
    CSLDestroy (olist);

    return ret;
}

void calHistogram( GDALRasterBand *poBand, GUIntBig *panHistogram )
{
    GByte *pabyData;
    int imageWidth, imageHeight;
    imageWidth = poBand->GetXSize();
    imageHeight = poBand->GetYSize();

    memset( panHistogram, 0, sizeof(GUIntBig) * 256 );
    CPLAssert( poBand->GetRasterDataType() == GDT_Byte );
    pabyData = (GByte *) CPLMalloc(imageWidth * imageHeight);

    poBand->RasterIO(GF_Read
                     ,0,0
                     ,imageWidth
                     ,imageHeight
                     ,pabyData
                     ,imageWidth
                     ,imageHeight
                     ,GDT_Byte
                     ,0,0);

    for(GIntBig index = 0;index < imageWidth * imageHeight;++index)
    {
        panHistogram[pabyData[index]] += 1;
    }

    CPLFree(pabyData);
}

// a likely more efficient traverse function
void calFastHistogram( GDALRasterBand *poBand, GUIntBig *panHistogram )
{
    int nXBlocks, nYBlocks, nXBlockSize, nYBlockSize;
    int iXBlock, iYBlock;
    GByte *pabyData;

    memset( panHistogram, 0, sizeof(GUIntBig) * 256 );
    CPLAssert( poBand->GetRasterDataType() == GDT_Byte );
    poBand->GetBlockSize( &nXBlockSize, &nYBlockSize );

    // get the upper integer
    nXBlocks = (poBand->GetXSize() + nXBlockSize - 1) / nXBlockSize;
    nYBlocks = (poBand->GetYSize() + nYBlockSize - 1) / nYBlockSize;

    pabyData = (GByte *) CPLMalloc(nXBlockSize * nYBlockSize);
    for( iYBlock = 0; iYBlock < nYBlocks; iYBlock++ )
    {
        for( iXBlock = 0; iXBlock < nXBlocks; iXBlock++ )
        {
            int nXValid, nYValid;
            poBand->ReadBlock( iXBlock, iYBlock, pabyData );   // Compute the portion of the block that is valid
            // for partial edge blocks.
            // get the real blockXWidth
            if( (iXBlock+1) * nXBlockSize > poBand->GetXSize() )
                nXValid = poBand->GetXSize() - iXBlock * nXBlockSize;
            else
                nXValid = nXBlockSize;
            // get the real blockYWidth
            if( (iYBlock+1) * nYBlockSize > poBand->GetYSize() )
                nYValid = poBand->GetYSize() - iYBlock * nYBlockSize;
            else
                nYValid = nYBlockSize; // Collect the histogram counts.
            for( int iY = 0; iY < nYValid; iY++ )
            {
                for( int iX = 0; iX < nXValid; iX++ )
                {
                    panHistogram[pabyData[iX + iY * nXBlockSize]] += 1;
                }
            }
        }
    }
    CPLFree(pabyData);
}
