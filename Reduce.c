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
	//								�㷨˼�룺
	//	����feature���飬�������µ�0��λ�õ�������Ȼ�󽫵���һ��λ�õ����������µ���
	//  ��һ�������Ƚϣ���������ƣ����´��������������������һ�����������������10
	//  ��������û�����£������µ�10�����������µ��������θ���д����feature����ǰ��
	//-------------------------------------------------------------------------------
	
	int nCounter = 0; //���������������10��������û������һ����������һ��
	int nSavePosition = 0;//��������������feature����ĸ���д��λ��

	//�ӵ�1��λ�ÿ�ʼ����0���Ѿ�Ĭ�ϱ���
	int nIndex;
	for (nIndex = 1;nIndex< nLength;nIndex++)
	{
		//�����������������߼�����������ֵ�������
		if(!FilterFrame_Sim(featureArray+(nSavePosition*FeatureDim),featureArray+(nIndex*FeatureDim))||++nCounter == 10)
		{
			nCounter = 0;//���ü�����
			nSavePosition ++; //�������µ���������д���λ��
			//��������������д����feature����ǰ��
			int nFeatureDim;
			for(nFeatureDim = 0;nFeatureDim<FeatureDim;nFeatureDim++)
			{
				featureArray[nSavePosition*FeatureDim+nFeatureDim] = featureArray[nIndex*FeatureDim+nFeatureDim];
			}
		}
	}
	return nSavePosition+1;//����������������
}




