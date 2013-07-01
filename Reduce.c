#define FeatureDim 8
#include <math.h>


/************************************************************************/
/*			this is a  interface to compare two feature                 */
/* Parameters:															*/
/*				f1:     a feature										*/
/*				f2:		another feature									*/
/* Return Value:														*/
/*				the similarity of two feature						    */
/************************************************************************/
int FilterFrame_Sim(float* f1,float* f2)
{
	float sim = 0.;
	float weight[8];
	weight[0]=weight[2]=weight[4]=0.15;
	weight[1]=weight[3]=weight[5]=0.1;
	weight[6]=weight[7]=0.125;
	int i; 
	for (i=0; i<FeatureDim; i++)
	{
		sim += fabs(f1[i]-f2[i])*weight[i]*100/(f1[i]+0.000001);
	}
	return (sim<15);
}

/************************************************************************/
/*			this is a  interface to purify the  feature                 */
/* Parameters:															*/
/*				nLength:      the number of the feature in featureArray	*/
/*				featureArray: the buffer of original feature			*/
/* Return Value:														*/
/*				returns the number of saved feature in  featureArray    */
/************************************************************************/
int FeatureRefine(int nLength,float* featureArray)
{
	//------------------------------------------------------------------------------
	//								算法思想：
	//	遍历feature数组，首先留下第0个位置的特征，然后将第下一个位置的特征与留下的最
	//  后一个特征比较，如果不相似，留下此特征，否则继续考察下一特征。如果连续考察10
	//  个特征都没有留下，则留下第10个特征。留下的特征依次覆盖写入在feature数组前端
	//-------------------------------------------------------------------------------
	
	int nCounter = 0; //计数器，如果连续10个特征都没能留下一个，就留下一个
	int nSavePosition = 0;//最新留下特征在feature数组的覆盖写入位置

	//从第1个位置开始，第0个已经默认保存
	int nIndex;
	for (nIndex = 1;nIndex< nLength;nIndex++)
	{
		//保留不相似特征或者计数器到达阈值后的特征
		if(!FilterFrame_Sim(featureArray+(nSavePosition*FeatureDim),featureArray+(nIndex*FeatureDim))||++nCounter == 10)
		{
			nCounter = 0;//重置计数器
			nSavePosition ++; //计算留下的特征覆盖写入的位置
			//将留下特征覆盖写入在feature数组前端
			int nFeatureDim;
			for(nFeatureDim = 0;nFeatureDim<FeatureDim;nFeatureDim++)
			{
				featureArray[nSavePosition*FeatureDim+nFeatureDim] = featureArray[nIndex*FeatureDim+nFeatureDim];
			}
		}
	}
	return nSavePosition+1;//返回留下特征个数
}




