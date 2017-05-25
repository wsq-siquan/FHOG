


#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include<fstream>

using namespace std;
using namespace cv;

// #define HEIGHT 512
// #define WIDTH 512
// #define DEPTH 3

#define PI    CV_PI

#define EPS 0.000001

#define F_MAX FLT_MAX
#define F_MIN -FLT_MAX

// The number of elements in bin
// The number of sectors in gradient histogram building
#define NUM_SECTOR 9

// The number of levels in image resize procedure
// We need Lambda levels to resize image twice
#define LAMBDA 10

// Block size. Used in feature pyramid building procedure
#define SIDE_LENGTH 8

#define VAL_OF_TRUNCATE 0.2f 
#define LATENT_SVM_OK 0
#define LATENT_SVM_MEM_NULL 2
#define DISTANCE_TRANSFORM_OK 1
#define DISTANCE_TRANSFORM_GET_INTERSECTION_ERROR -1
#define DISTANCE_TRANSFORM_ERROR -2
#define DISTANCE_TRANSFORM_EQUAL_POINTS -3
#define LATENT_SVM_GET_FEATURE_PYRAMID_FAILED -4
#define LATENT_SVM_SEARCH_OBJECT_FAILED -5
#define LATENT_SVM_FAILED_SUPERPOSITION -6
#define FILTER_OUT_OF_BOUNDARIES -7
#define LATENT_SVM_TBB_SCHEDULE_CREATION_FAILED -8
#define LATENT_SVM_TBB_NUMTHREADS_NOT_CORRECT -9
#define FFT_OK 2
#define FFT_ERROR -10
#define LSVM_PARSER_FILE_NOT_FOUND -11

//
//#ifdef HAVE_TBB
//#include <tbb/tbb.h>
//#include "tbb/parallel_for.h"
//#include "tbb/blocked_range.h"
//#endif
//
//#ifndef max
//#define max(a,b)            (((a) > (b)) ? (a) : (b))
//#endif
//
//#ifndef min
//#define min(a,b)            (((a) < (b)) ? (a) : (b))
//#endif

typedef struct {
	int sizeX;
	int sizeY;
	int numFeatures;
	float *map;
} CvLSVMFeatureMapCaskade;


int getFeatureMaps(const IplImage * image, const int k, CvLSVMFeatureMapCaskade **map);
int normalizeAndTruncate(CvLSVMFeatureMapCaskade *map, const float alfa);
int PCAFeatureMaps(CvLSVMFeatureMapCaskade *map);

int allocFeatureMapObject(CvLSVMFeatureMapCaskade **obj, const int sizeX, const int sizeY,
	const int p);

int freeFeatureMapObject(CvLSVMFeatureMapCaskade **obj);




int main() {
	for (int mouse = 1; mouse < 500; mouse++) {
		std::stringstream ss;
		std::string str;
		ss << mouse;
		ss >> str;
		string fname = "F:/data/0059/test (" + str;
		fname += ").jpg";
		cout << fname << endl;

		//String fname= "F:/lena.jpg";
		//string fff = "F:/mo/mouse (1).jpg";

		Mat img = imread(fname);
		Mat imgDst;
		resize(img, imgDst, Size(32, 32));
		IplImage z_ipl = imgDst;
		CvLSVMFeatureMapCaskade *map;
		getFeatureMaps(&z_ipl, 8, &map);
		normalizeAndTruncate(map, 0.2f);
		PCAFeatureMaps(map);
		int size = map->sizeX * map->sizeY * map->numFeatures;
		cout << size << endl;
		
		ofstream outfile("F:/data/tout.txt", ofstream::app);
		//outfile << "2 ";
		for (int i = 0; i < size; i++) {
			outfile << i + 1 << ":" << map->map[i] << " ";
		}
		outfile << endl;
		outfile.close();
		//freeFeatureMapObject(&map);
		//outfile << "0 0 0 0" << endl;
	}
	return 6666;
}



int getFeatureMaps(const IplImage* image, const int k, CvLSVMFeatureMapCaskade **map)
{
	int sizeX, sizeY;
	int p, px, stringSize;
	int height, width, numChannels;
	int i, j, kk, c, ii, jj, d;
	float  * datadx, *datady;

	int   ch;
	float magnitude, x, y, tx, ty;

	IplImage * dx, *dy;
	int *nearest;
	float *w, a_x, b_x;

	float kernel[3] = { -1.f, 0.f, 1.f };
	CvMat kernel_dx = cvMat(1, 3, CV_32F, kernel);
	CvMat kernel_dy = cvMat(3, 1, CV_32F, kernel);

	float * r;
	int   * alfa;

	float boundary_x[NUM_SECTOR + 1];
	float boundary_y[NUM_SECTOR + 1];
	float max, dotProd;
	int   maxi;

	height = image->height;
	width = image->width;

	numChannels = image->nChannels;

	dx = cvCreateImage(cvSize(image->width, image->height),
		IPL_DEPTH_32F, 3);
	dy = cvCreateImage(cvSize(image->width, image->height),
		IPL_DEPTH_32F, 3);

	sizeX = width / k;
	sizeY = height / k;
	px = 3 * NUM_SECTOR;
	p = px;
	stringSize = sizeX * p;
	allocFeatureMapObject(map, sizeX, sizeY, p);

	cvFilter2D(image, dx, &kernel_dx, cvPoint(-1, 0));
	cvFilter2D(image, dy, &kernel_dy, cvPoint(0, -1));

	float arg_vector;
	for (i = 0; i <= NUM_SECTOR; i++)
	{
		arg_vector = ((float)i) * ((float)(PI) / (float)(NUM_SECTOR));
		boundary_x[i] = cosf(arg_vector);
		boundary_y[i] = sinf(arg_vector);
	}/*for(i = 0; i <= NUM_SECTOR; i++) */

	r = (float *)malloc(sizeof(float) * (width * height));
	alfa = (int   *)malloc(sizeof(int) * (width * height * 2));

	for (j = 1; j < height - 1; j++)
	{
		datadx = (float*)(dx->imageData + dx->widthStep * j);
		datady = (float*)(dy->imageData + dy->widthStep * j);
		for (i = 1; i < width - 1; i++)
		{
			c = 0;
			x = (datadx[i * numChannels + c]);
			y = (datady[i * numChannels + c]);

			r[j * width + i] = sqrtf(x * x + y * y);
			for (ch = 1; ch < numChannels; ch++)
			{
				tx = (datadx[i * numChannels + ch]);
				ty = (datady[i * numChannels + ch]);
				magnitude = sqrtf(tx * tx + ty * ty);
				if (magnitude > r[j * width + i])
				{
					r[j * width + i] = magnitude;
					c = ch;
					x = tx;
					y = ty;
				}
			}/*for(ch = 1; ch < numChannels; ch++)*/

			max = boundary_x[0] * x + boundary_y[0] * y;
			maxi = 0;
			for (kk = 0; kk < NUM_SECTOR; kk++)
			{
				dotProd = boundary_x[kk] * x + boundary_y[kk] * y;
				if (dotProd > max)
				{
					max = dotProd;
					maxi = kk;
				}
				else
				{
					if (-dotProd > max)
					{
						max = -dotProd;
						maxi = kk + NUM_SECTOR;
					}
				}
			}
			alfa[j * width * 2 + i * 2] = maxi % NUM_SECTOR;
			alfa[j * width * 2 + i * 2 + 1] = maxi;
		}/*for(i = 0; i < width; i++)*/
	}/*for(j = 0; j < height; j++)*/

	nearest = (int  *)malloc(sizeof(int) *  k);
	w = (float*)malloc(sizeof(float) * (k * 2));

	for (i = 0; i < k / 2; i++)
	{
		nearest[i] = -1;
	}/*for(i = 0; i < k / 2; i++)*/
	for (i = k / 2; i < k; i++)
	{
		nearest[i] = 1;
	}/*for(i = k / 2; i < k; i++)*/

	for (j = 0; j < k / 2; j++)
	{
		b_x = k / 2 + j + 0.5f;
		a_x = k / 2 - j - 0.5f;
		w[j * 2] = 1.0f / a_x * ((a_x * b_x) / (a_x + b_x));
		w[j * 2 + 1] = 1.0f / b_x * ((a_x * b_x) / (a_x + b_x));
	}/*for(j = 0; j < k / 2; j++)*/
	for (j = k / 2; j < k; j++)
	{
		a_x = j - k / 2 + 0.5f;
		b_x = -j + k / 2 - 0.5f + k;
		w[j * 2] = 1.0f / a_x * ((a_x * b_x) / (a_x + b_x));
		w[j * 2 + 1] = 1.0f / b_x * ((a_x * b_x) / (a_x + b_x));
	}/*for(j = k / 2; j < k; j++)*/

	for (i = 0; i < sizeY; i++)
	{
		for (j = 0; j < sizeX; j++)
		{
			for (ii = 0; ii < k; ii++)
			{
				for (jj = 0; jj < k; jj++)
				{
					if ((i * k + ii > 0) &&
						(i * k + ii < height - 1) &&
						(j * k + jj > 0) &&
						(j * k + jj < width - 1))
					{
						d = (k * i + ii) * width + (j * k + jj);
						(*map)->map[i * stringSize + j * (*map)->numFeatures + alfa[d * 2]] +=
							r[d] * w[ii * 2] * w[jj * 2];
						(*map)->map[i * stringSize + j * (*map)->numFeatures + alfa[d * 2 + 1] + NUM_SECTOR] +=
							r[d] * w[ii * 2] * w[jj * 2];
						if ((i + nearest[ii] >= 0) &&
							(i + nearest[ii] <= sizeY - 1))
						{
							(*map)->map[(i + nearest[ii]) * stringSize + j * (*map)->numFeatures + alfa[d * 2]] +=
								r[d] * w[ii * 2 + 1] * w[jj * 2];
							(*map)->map[(i + nearest[ii]) * stringSize + j * (*map)->numFeatures + alfa[d * 2 + 1] + NUM_SECTOR] +=
								r[d] * w[ii * 2 + 1] * w[jj * 2];
						}
						if ((j + nearest[jj] >= 0) &&
							(j + nearest[jj] <= sizeX - 1))
						{
							(*map)->map[i * stringSize + (j + nearest[jj]) * (*map)->numFeatures + alfa[d * 2]] +=
								r[d] * w[ii * 2] * w[jj * 2 + 1];
							(*map)->map[i * stringSize + (j + nearest[jj]) * (*map)->numFeatures + alfa[d * 2 + 1] + NUM_SECTOR] +=
								r[d] * w[ii * 2] * w[jj * 2 + 1];
						}
						if ((i + nearest[ii] >= 0) &&
							(i + nearest[ii] <= sizeY - 1) &&
							(j + nearest[jj] >= 0) &&
							(j + nearest[jj] <= sizeX - 1))
						{
							(*map)->map[(i + nearest[ii]) * stringSize + (j + nearest[jj]) * (*map)->numFeatures + alfa[d * 2]] +=
								r[d] * w[ii * 2 + 1] * w[jj * 2 + 1];
							(*map)->map[(i + nearest[ii]) * stringSize + (j + nearest[jj]) * (*map)->numFeatures + alfa[d * 2 + 1] + NUM_SECTOR] +=
								r[d] * w[ii * 2 + 1] * w[jj * 2 + 1];
						}
					}
				}/*for(jj = 0; jj < k; jj++)*/
			}/*for(ii = 0; ii < k; ii++)*/
		}/*for(j = 1; j < sizeX - 1; j++)*/
	}/*for(i = 1; i < sizeY - 1; i++)*/

	cvReleaseImage(&dx);
	cvReleaseImage(&dy);


	free(w);
	free(nearest);

	free(r);
	free(alfa);

	return LATENT_SVM_OK;
}

/*
// Feature map Normalization and Truncation
//
// API
// int normalizeAndTruncate(featureMap *map, const float alfa);
// INPUT
// map               - feature map
// alfa              - truncation threshold
// OUTPUT
// map               - truncated and normalized feature map
// RESULT
// Error status
*/
int normalizeAndTruncate(CvLSVMFeatureMapCaskade *map, const float alfa)
{
	int i, j, ii;
	int sizeX, sizeY, p, pos, pp, xp, pos1, pos2;
	float * partOfNorm; // norm of C(i, j)
	float * newData;
	float   valOfNorm;

	sizeX = map->sizeX;
	sizeY = map->sizeY;
	partOfNorm = (float *)malloc(sizeof(float) * (sizeX * sizeY));

	p = NUM_SECTOR;
	xp = NUM_SECTOR * 3;
	pp = NUM_SECTOR * 12;

	for (i = 0; i < sizeX * sizeY; i++)
	{
		valOfNorm = 0.0f;
		pos = i * map->numFeatures;
		for (j = 0; j < p; j++)
		{
			valOfNorm += map->map[pos + j] * map->map[pos + j];
		}/*for(j = 0; j < p; j++)*/
		partOfNorm[i] = valOfNorm;
	}/*for(i = 0; i < sizeX * sizeY; i++)*/

	sizeX -= 2;
	sizeY -= 2;

	newData = (float *)malloc(sizeof(float) * (sizeX * sizeY * pp));
	//normalization
	for (i = 1; i <= sizeY; i++)
	{
		for (j = 1; j <= sizeX; j++)
		{
			valOfNorm = sqrtf(
				partOfNorm[(i)*(sizeX + 2) + (j)] +
				partOfNorm[(i)*(sizeX + 2) + (j + 1)] +
				partOfNorm[(i + 1)*(sizeX + 2) + (j)] +
				partOfNorm[(i + 1)*(sizeX + 2) + (j + 1)]) + FLT_EPSILON;
			pos1 = (i)* (sizeX + 2) * xp + (j)* xp;
			pos2 = (i - 1) * (sizeX)* pp + (j - 1) * pp;
			for (ii = 0; ii < p; ii++)
			{
				newData[pos2 + ii] = map->map[pos1 + ii] / valOfNorm;
			}/*for(ii = 0; ii < p; ii++)*/
			for (ii = 0; ii < 2 * p; ii++)
			{
				newData[pos2 + ii + p * 4] = map->map[pos1 + ii + p] / valOfNorm;
			}/*for(ii = 0; ii < 2 * p; ii++)*/
			valOfNorm = sqrtf(
				partOfNorm[(i)*(sizeX + 2) + (j)] +
				partOfNorm[(i)*(sizeX + 2) + (j + 1)] +
				partOfNorm[(i - 1)*(sizeX + 2) + (j)] +
				partOfNorm[(i - 1)*(sizeX + 2) + (j + 1)]) + FLT_EPSILON;
			for (ii = 0; ii < p; ii++)
			{
				newData[pos2 + ii + p] = map->map[pos1 + ii] / valOfNorm;
			}/*for(ii = 0; ii < p; ii++)*/
			for (ii = 0; ii < 2 * p; ii++)
			{
				newData[pos2 + ii + p * 6] = map->map[pos1 + ii + p] / valOfNorm;
			}/*for(ii = 0; ii < 2 * p; ii++)*/
			valOfNorm = sqrtf(
				partOfNorm[(i)*(sizeX + 2) + (j)] +
				partOfNorm[(i)*(sizeX + 2) + (j - 1)] +
				partOfNorm[(i + 1)*(sizeX + 2) + (j)] +
				partOfNorm[(i + 1)*(sizeX + 2) + (j - 1)]) + FLT_EPSILON;
			for (ii = 0; ii < p; ii++)
			{
				newData[pos2 + ii + p * 2] = map->map[pos1 + ii] / valOfNorm;
			}/*for(ii = 0; ii < p; ii++)*/
			for (ii = 0; ii < 2 * p; ii++)
			{
				newData[pos2 + ii + p * 8] = map->map[pos1 + ii + p] / valOfNorm;
			}/*for(ii = 0; ii < 2 * p; ii++)*/
			valOfNorm = sqrtf(
				partOfNorm[(i)*(sizeX + 2) + (j)] +
				partOfNorm[(i)*(sizeX + 2) + (j - 1)] +
				partOfNorm[(i - 1)*(sizeX + 2) + (j)] +
				partOfNorm[(i - 1)*(sizeX + 2) + (j - 1)]) + FLT_EPSILON;
			for (ii = 0; ii < p; ii++)
			{
				newData[pos2 + ii + p * 3] = map->map[pos1 + ii] / valOfNorm;
			}/*for(ii = 0; ii < p; ii++)*/
			for (ii = 0; ii < 2 * p; ii++)
			{
				newData[pos2 + ii + p * 10] = map->map[pos1 + ii + p] / valOfNorm;
			}/*for(ii = 0; ii < 2 * p; ii++)*/
		}/*for(j = 1; j <= sizeX; j++)*/
	}/*for(i = 1; i <= sizeY; i++)*/
	 //truncation
	for (i = 0; i < sizeX * sizeY * pp; i++)
	{
		if (newData[i] > alfa) newData[i] = alfa;
	}/*for(i = 0; i < sizeX * sizeY * pp; i++)*/
	 //swop data

	map->numFeatures = pp;
	map->sizeX = sizeX;
	map->sizeY = sizeY;

	free(map->map);
	free(partOfNorm);

	map->map = newData;

	return LATENT_SVM_OK;
}
/*
// Feature map reduction
// In each cell we reduce dimension of the feature vector
// according to original paper special procedure
//
// API
// int PCAFeatureMaps(featureMap *map)
// INPUT
// map               - feature map
// OUTPUT
// map               - feature map
// RESULT
// Error status
*/
int PCAFeatureMaps(CvLSVMFeatureMapCaskade *map)
{
	int i, j, ii, jj, k;
	int sizeX, sizeY, p, pp, xp, yp, pos1, pos2;
	float * newData;
	float val;
	float nx, ny;

	sizeX = map->sizeX;
	sizeY = map->sizeY;
	p = map->numFeatures;
	pp = NUM_SECTOR * 3 + 4;
	yp = 4;
	xp = NUM_SECTOR;

	nx = 1.0f / sqrtf((float)(xp * 2));
	ny = 1.0f / sqrtf((float)(yp));

	newData = (float *)malloc(sizeof(float) * (sizeX * sizeY * pp));

	for (i = 0; i < sizeY; i++)
	{
		for (j = 0; j < sizeX; j++)
		{
			pos1 = ((i)*sizeX + j)*p;
			pos2 = ((i)*sizeX + j)*pp;
			k = 0;
			for (jj = 0; jj < xp * 2; jj++)
			{
				val = 0;
				for (ii = 0; ii < yp; ii++)
				{
					val += map->map[pos1 + yp * xp + ii * xp * 2 + jj];
				}/*for(ii = 0; ii < yp; ii++)*/
				newData[pos2 + k] = val * ny;
				k++;
			}/*for(jj = 0; jj < xp * 2; jj++)*/
			for (jj = 0; jj < xp; jj++)
			{
				val = 0;
				for (ii = 0; ii < yp; ii++)
				{
					val += map->map[pos1 + ii * xp + jj];
				}/*for(ii = 0; ii < yp; ii++)*/
				newData[pos2 + k] = val * ny;
				k++;
			}/*for(jj = 0; jj < xp; jj++)*/
			for (ii = 0; ii < yp; ii++)
			{
				val = 0;
				for (jj = 0; jj < 2 * xp; jj++)
				{
					val += map->map[pos1 + yp * xp + ii * xp * 2 + jj];
				}/*for(jj = 0; jj < xp; jj++)*/
				newData[pos2 + k] = val * nx;
				k++;
			} /*for(ii = 0; ii < yp; ii++)*/
		}/*for(j = 0; j < sizeX; j++)*/
	}/*for(i = 0; i < sizeY; i++)*/
	 //swop data

	map->numFeatures = pp;

	free(map->map);

	map->map = newData;

	return LATENT_SVM_OK;
}


//modified from "lsvmc_routine.cpp"

int allocFeatureMapObject(CvLSVMFeatureMapCaskade **obj, const int sizeX,
	const int sizeY, const int numFeatures)
{
	int i;
	(*obj) = (CvLSVMFeatureMapCaskade *)malloc(sizeof(CvLSVMFeatureMapCaskade));
	(*obj)->sizeX = sizeX;
	(*obj)->sizeY = sizeY;
	(*obj)->numFeatures = numFeatures;
	(*obj)->map = (float *)malloc(sizeof(float) *
		(sizeX * sizeY  * numFeatures));
	for (i = 0; i < sizeX * sizeY * numFeatures; i++)
	{
		(*obj)->map[i] = 0.0f;
	}
	return LATENT_SVM_OK;
}

int freeFeatureMapObject(CvLSVMFeatureMapCaskade **obj)
{
	if (*obj == NULL) return LATENT_SVM_MEM_NULL;
	free((*obj)->map);
	free(*obj);
	(*obj) = NULL;
	return LATENT_SVM_OK;
}

	


//	Mat image = imread("F:/lena.jpg", 0);
//
//    Mat tmp_image;
//	Mat image_resized;
//    image.convertTo(tmp_image, CV_32FC1);
//    cout<<image.rows<<" "<<image.cols<<endl;
//	//resize(tmp_image, image_resized, Size(32, 32));
//    Mat fhog_feature = fhog::fhog(tmp_image);
//    cout<<fhog_feature.rows<<" "<<fhog_feature.cols<<endl;
//	
//
//	ofstream outfile;
//	outfile.open("F:/out.txt");
//
//    vector<Mat> feats;
//    split(fhog_feature, feats);
//	//cout << feats.size() << endl;
//	//cout << feats[0].at<>(0, 0) << endl;
//    for (size_t i = 0; i < feats.size(); i++) {
//		//if(feats[i] != NULL)
//        //cout<<feats[i]<<endl;
//		outfile <<"********" <<i << endl;
//		//cout << feats[i].rows << " " << feats[i].cols << endl;
//		for (int r = 0; r < feats[i].rows; r++) {
//			for (int c = 0; c < feats[i].cols; c++) {
//				outfile << feats[i].at<double>(r, c) << " ";
//			}
//			outfile << endl;
//		}
//		//cout << feats[i].at<double>(0, 0) << endl;
//        
//		imshow("feat", feats[i]);
//        waitKey();
//		
//    }
//	outfile.close();
//
//
//
//
//
//    return 0;
//}