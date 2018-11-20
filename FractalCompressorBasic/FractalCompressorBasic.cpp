// FractalCompressorBasic.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <Windows.h>
#include <math.h>

typedef struct HEADEROFFCOMFILE //todo. correct the size
{
	int16_t blueDomainCount;
	int16_t redDomainCount;
	int16_t greenDomainCount;
};

typedef struct COMPRESSEDBLOCKCODE //todo. correct the size
{
	int16_t xoffset;
	int16_t yoffset;
	int16_t xdoffset;
	int16_t ydoffset;
	byte transformType;
	int16_t blockSize;
	float brightnessDifference;
	float contrastCoefficient;
};


byte* somebytes;
int valoffset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

//first draft. needs work. also need to be able to select filetype.
int LoadPixels(const char* fname, byte*** reftopixels, BITMAPFILEHEADER* fheader, BITMAPINFOHEADER* iheader)
{
	std::ifstream file(fname, std::ios::binary);
	if (!file)
	{
		std::cout << "can't open file " << fname << "\n";
		return 1;
	}
	file.read((char*)fheader, sizeof(BITMAPFILEHEADER));
	file.read((char*)iheader, sizeof(BITMAPINFOHEADER));
	if (fheader->bfType != 0x4D42)
	{
		std::cout << "file " << fname << "is not a bmp file\n";
		return 2;
	}
	somebytes = (byte*)malloc(fheader->bfOffBits - valoffset);
	file.read((char*)somebytes, fheader->bfOffBits - valoffset);
	*reftopixels = (byte**)malloc(sizeof(byte*) * iheader->biHeight);//чет фигня какаято
	for (int i = 0; i < iheader->biHeight; i++)
	{
		(*reftopixels)[i] = (byte*)malloc(sizeof(byte)* iheader->biSizeImage/ iheader->biHeight);
		file.read((char*)((*reftopixels)[i]), (iheader->biSizeImage / iheader->biHeight));
	}
	return 0;
}

void SavePixels(const char* fname, byte** pixels, BITMAPFILEHEADER* fheader, BITMAPINFOHEADER* iheader)
{
	std::ofstream file(fname, std::ios::binary);
	file.write((char*)fheader, sizeof(BITMAPFILEHEADER));
	file.write((char*)iheader, sizeof(BITMAPINFOHEADER));
	file.write((char*)somebytes, fheader->bfOffBits - valoffset);
	int padsize = (4 - (iheader->biWidth * 3) % 4);
	if (padsize == 4) padsize = 0;
	byte padding = 0;
	for (int i = 0; i < iheader->biHeight; i++)
	{
		file.write((char*)(pixels[i]), (iheader->biWidth * 3));
		for (int j = 0; j < padsize; j++)
			file.write((char*)&padding, sizeof(byte));
	}
}

byte** rotate90(byte** block, int n)
{
	byte** newblock = (byte**)malloc(sizeof(byte*) * n);
	for (int i = 0; i < n; i++) 
	{
		newblock[i] = (byte*)malloc(sizeof(byte) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[n - j - 1][i];
		}
	}
	return newblock;
}

byte** rotate180(byte** block, int n)
{
	byte** newblock = (byte**)malloc(sizeof(byte*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (byte*)malloc(sizeof(byte) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[n - i - 1][n - j - 1];
		}
	}
	return newblock;
}

byte** rotate270(byte** block, int n)
{
	byte** newblock = (byte**)malloc(sizeof(byte*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (byte*)malloc(sizeof(byte) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[j][n - i - 1];
		}
	}
	return newblock;
}

byte** flipHorizontal(byte** block, int n)
{
	byte** newblock = (byte**)malloc(sizeof(byte*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (byte*)malloc(sizeof(byte) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[i][n - j - 1];
		}
	}
	return newblock;
}

byte** flipVertical(byte** block, int n)
{
	byte** newblock = (byte**)malloc(sizeof(byte*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (byte*)malloc(sizeof(byte) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[n - i - 1][j];
		}
	}
	return newblock;
}

byte** flipAlongMainDiagonal(byte** block, int n)
{
	byte** newblock = (byte**)malloc(sizeof(byte*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (byte*)malloc(sizeof(byte) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[j][i];
		}
	}
	return newblock;
}

byte** flipAlongSubDiagonal(byte** block, int n)
{
	byte** newblock = (byte**)malloc(sizeof(byte*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (byte*)malloc(sizeof(byte) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[n - j - 1][n - i - 1];
		}
	}
	return newblock;
}

byte** downsize(byte** pixels, int xoffset, int yoffset, int n)
{
	int m = n / 2;
	byte** newblock = (byte**)malloc(sizeof(byte*) * m);
	for (int i = 0; i < m; i++)
	{
		newblock[i] = (byte*)malloc(sizeof(byte) * m);
		for (int j = 0; j < m; j++)
		{
			newblock[i][j] = (pixels[yoffset + 2 * i][xoffset + 2 * j] + pixels[yoffset + 2 * i][xoffset + 2 * j + 1] +
				pixels[yoffset + 2 * i + 1][xoffset + 2 * j] + pixels[yoffset + 2 * i + 1][xoffset + 2 * j + 1]) / 4;
		}
	}
	return newblock;	
}

float** frotate90(float** block, int n)
{
	float** newblock = (float**)malloc(sizeof(float*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (float*)malloc(sizeof(float) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[n - j - 1][i];
		}
	}
	return newblock;
}

float** frotate180(float** block, int n)
{
	float** newblock = (float**)malloc(sizeof(float*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (float*)malloc(sizeof(float) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[n - i - 1][n - j - 1];
		}
	}
	return newblock;
}

float** frotate270(float** block, int n)
{
	float** newblock = (float**)malloc(sizeof(float*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (float*)malloc(sizeof(float) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[j][n - i - 1];
		}
	}
	return newblock;
}

float** fflipHorizontal(float** block, int n)
{
	float** newblock = (float**)malloc(sizeof(float*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (float*)malloc(sizeof(float) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[i][n - j - 1];
		}
	}
	return newblock;
}

float** fflipVertical(float** block, int n)
{
	float** newblock = (float**)malloc(sizeof(float*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (float*)malloc(sizeof(float) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[n - i - 1][j];
		}
	}
	return newblock;
}

float** fflipAlongMainDiagonal(float** block, int n)
{
	float** newblock = (float**)malloc(sizeof(float*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (float*)malloc(sizeof(float) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[j][i];
		}
	}
	return newblock;
}

float** fflipAlongSubDiagonal(float** block, int n)
{
	float** newblock = (float**)malloc(sizeof(float*) * n);
	for (int i = 0; i < n; i++)
	{
		newblock[i] = (float*)malloc(sizeof(float) * n);
		for (int j = 0; j < n; j++)
		{
			newblock[i][j] = block[n - j - 1][n - i - 1];
		}
	}
	return newblock;
}

float** fdownsize(float** pixels, int xoffset, int yoffset, int n)
{
	int m = n / 2;
	float** newblock = (float**)malloc(sizeof(float*) * m);
	for (int i = 0; i < m; i++)
	{
		newblock[i] = (float*)malloc(sizeof(float) * m);
		for (int j = 0; j < m; j++)
		{
			newblock[i][j] = (pixels[yoffset + 2 * i][xoffset + 2 * j] + pixels[yoffset + 2 * i][xoffset + 2 * j + 1] +
				pixels[yoffset + 2 * i + 1][xoffset + 2 * j] + pixels[yoffset + 2 * i + 1][xoffset + 2 * j + 1]) / 4;
		}
	}
	return newblock;
}


void compareAndUpdate(double* minDifference, double difference, int* ki, int k, int* li, int l, byte* affineTransform, byte caffineTransform)
{
	if (difference < *minDifference)
	{
		*minDifference = difference;
		*ki = k;
		*li = l;
		*affineTransform = caffineTransform;
	}
}

void free2Dimensions(byte** ptr, int n)
{
	for (int i = 0; i < n; i++)
	{
		free(ptr[i]);
	}
	free(ptr);
}

void free2fDimensions(float** ptr, int n)
{
	for (int i = 0; i < n; i++)
	{
		free(ptr[i]);
	}
	free(ptr);
}

void calcCoeffs(byte** block, byte**  pixels, int offsetX, int offsetY, int n, float* brightDiffValue, float* contrastCoefficient)
{
	int pval = 0;
	int dval = 0;
	float a = 0;
	float b = 0;
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			pval += pixels[offsetY + i][offsetX + j];
			dval += block[i][j];
		}
	}
	float daverage = ((float)dval) / (n*n);
	float paverage = ((float)pval) / (n*n);
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			a += (block[i][j] - daverage)*(pixels[offsetY + i][offsetX + j] - paverage);
			b += (block[i][j] - paverage)*(block[i][j] - daverage);
		}
	}
	*contrastCoefficient = a / b;
	*brightDiffValue = (paverage - (a / b)*daverage);
}

double difference(byte** block, byte**  pixels, int offsetX, int offsetY, int n)
{
	double difference = 0;
	float brightDiffValue = 0;
	float contrastCoefficient = 0;
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			difference += pow(block[i][j] - pixels[offsetY + i][offsetX + j], 2);
		}
	}
	return difference;
}




int calculateDomainSize()
{
	return 0;//todo this thing later
}

void colorChannelSeparator(byte** pixels, byte** blue, byte** green, byte** red, int width, int height)
{
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			blue[i][j] = pixels[i][j * 3];
			green[i][j] = pixels[i][j * 3 + 1];
			red[i][j] = pixels[i][j * 3 + 2];
		}
	}
}

void colorChannelCombinator(byte** pixels, byte** blue, byte** green, byte** red, int width, int height)
{
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pixels[i][j * 3] = blue[i][j];
			pixels[i][j * 3 + 1] = green[i][j];
			pixels[i][j * 3 + 2] = red[i][j];
		}
	}
}

void fractalCompressionStep3(byte** pixels, int offsetx, int offsety, int blocksize, int* blockamount, COMPRESSEDBLOCKCODE** blockCodes, int sizex, int sizey, int qualifer)
{
	int offsetxOfMin = 0;
	int offsetyOfMin = 0;
	double minDifference = MAXINT;
	float brightnessDifference = 0;
	byte affineTransform = 0;
	float contrastCoefficient = 0;
	for (int offsetxl = 0; offsetxl + 2 * blocksize <= sizex; offsetxl += blocksize)
	{
		for (int offsetyl = 0; offsetyl + 2 * blocksize <= sizey; offsetyl += blocksize)
		{
			byte*** affineTransfs = (byte***)malloc(8 * sizeof(byte**));
			affineTransfs[0] = downsize(pixels, offsetxl, offsetyl, blocksize * 2);
			affineTransfs[1] = rotate90(affineTransfs[0], blocksize);
			affineTransfs[2] = rotate180(affineTransfs[0], blocksize);
			affineTransfs[3] = rotate270(affineTransfs[0], blocksize);
			affineTransfs[4] = flipHorizontal(affineTransfs[0], blocksize);
			affineTransfs[5] = flipVertical(affineTransfs[0], blocksize);
			affineTransfs[6] = flipAlongMainDiagonal(affineTransfs[0], blocksize);
			affineTransfs[7] = flipAlongSubDiagonal(affineTransfs[0], blocksize);
			double cdifference;
			for (int i = 0; i < 8; i++)
			{
				cdifference = difference(affineTransfs[i], pixels, offsetx, offsety, blocksize);
				compareAndUpdate(&minDifference, cdifference, &offsetxOfMin, offsetxl, &offsetyOfMin, offsetyl, &affineTransform, i);
			}
			for (int i = 0; i < 8; i++)
			{
				free2Dimensions(affineTransfs[i], blocksize);
			}
			free(affineTransfs);
		}
	}
	minDifference /= (blocksize * blocksize);
	if (minDifference < qualifer || blocksize <= 4)
	{
		byte** downblock = downsize(pixels, offsetxOfMin, offsetyOfMin, blocksize * 2);
		byte** trblock = nullptr;
		switch (affineTransform)
		{
		case 1:trblock = rotate90(downblock, blocksize); break;
		case 2:trblock = rotate180(downblock, blocksize); break;
		case 3:trblock = rotate270(downblock, blocksize); break;
		case 4:trblock = flipHorizontal(downblock, blocksize); break;
		case 5:trblock = flipVertical(downblock, blocksize); break;
		case 6:trblock = flipAlongMainDiagonal(downblock, blocksize); break;
		case 7:trblock = flipAlongSubDiagonal(downblock, blocksize); break;
		}
		if (affineTransform == 0)
		{
			calcCoeffs(downblock, pixels, offsetx, offsety, blocksize, &brightnessDifference, &contrastCoefficient);
			free2Dimensions(downblock, blocksize);
		}
		else
		{
			calcCoeffs(trblock, pixels, offsetx, offsety, blocksize, &brightnessDifference, &contrastCoefficient);
			free2Dimensions(downblock, blocksize);
			free2Dimensions(trblock, blocksize);
		}
		//in the future use initial size and check for overflow, then reallocate
		//causes a brpnt error -> blockCodes = (COMPRESSEDBLOCKCODE**)realloc(blockCodes, ((*blockamount) + 1) * sizeof(COMPRESSEDBLOCKCODE*));
		blockCodes[*blockamount] = (COMPRESSEDBLOCKCODE*)malloc(sizeof(COMPRESSEDBLOCKCODE));
		blockCodes[*blockamount]->blockSize = blocksize;
		blockCodes[*blockamount]->brightnessDifference = brightnessDifference;
		blockCodes[*blockamount]->contrastCoefficient = contrastCoefficient;
		blockCodes[*blockamount]->transformType = affineTransform;
		blockCodes[*blockamount]->xoffset = offsetx;
		blockCodes[*blockamount]->yoffset = offsety;
		blockCodes[*blockamount]->xdoffset = offsetxOfMin;
		blockCodes[*blockamount]->ydoffset = offsetyOfMin;
		(*blockamount)++;
	}
	else
	{
		fractalCompressionStep3(pixels, offsetx, offsety, blocksize / 2, blockamount, blockCodes, sizex, sizey, qualifer);
		fractalCompressionStep3(pixels, offsetx + blocksize / 2, offsety, blocksize / 2, blockamount, blockCodes, sizex, sizey, qualifer);
		fractalCompressionStep3(pixels, offsetx, offsety + blocksize / 2, blocksize / 2, blockamount, blockCodes, sizex, sizey, qualifer);
		fractalCompressionStep3(pixels, offsetx + blocksize / 2, offsety + blocksize / 2, blocksize / 2, blockamount, blockCodes, sizex, sizey, qualifer);
	}
}

//this encodes domains as range which is wrong
//void fractalCompressionStep2(byte** pixels, int offsetx, int offsety, int blocksize, int* blockamount, COMPRESSEDBLOCKCODE** blockCodes, float brightnessCompressionCoef, int sizex, int sizey, int qualifer)
//{
//	byte*** affineTransfs = (byte***)malloc(8 * sizeof(byte**));
//	affineTransfs[0] = downsize(pixels, offsetx, offsety, blocksize);
//	affineTransfs[1] = rotate90(affineTransfs[0], blocksize / 2);
//	affineTransfs[2] = rotate180(affineTransfs[0], blocksize / 2);
//	affineTransfs[3] = rotate270(affineTransfs[0], blocksize / 2);
//	affineTransfs[4] = flipHorizontal(affineTransfs[0], blocksize / 2);
//	affineTransfs[5] = flipVertical(affineTransfs[0], blocksize / 2);
//	affineTransfs[6] = flipAlongMainDiagonal(affineTransfs[0], blocksize / 2);
//	affineTransfs[7] = flipAlongSubDiagonal(affineTransfs[0], blocksize / 2);
//	int offsetxOfMin = 0;
//	int offsetyOfMin = 0;
//	int minDifference = MAXINT;
//	int brightnessDifference = 0;
//	byte affineTransform = 0;
//	for (int offsetxl = 0; offsetxl < sizex; offsetxl += blocksize / 2)
//	{
//		for (int offsetyl = 0; offsetyl < sizey; offsetyl += blocksize / 2)
//		{
//			int cdifference;
//			int cbrightnessDiffValue;
//			for (int i = 0; i < 8; i++)
//			{
//				cdifference = difference(affineTransfs[i], pixels, offsetxl, offsetyl, blocksize / 2, &cbrightnessDiffValue, brightnessCompressionCoef);
//				compareAndUpdate(&minDifference, cdifference, &offsetxOfMin, offsetxl, &offsetyOfMin, offsetyl, &affineTransform, i, &brightnessDifference, cbrightnessDiffValue);
//			}
//		}
//	}
//	for (int i = 0; i < 8; i++)
//	{
//		free2Dimensions(affineTransfs[i], blocksize / 2);
//	}
//	free(affineTransfs);
//	minDifference /= (blocksize * blocksize / 4);
//	if (minDifference < qualifer || blocksize < 8)
//	{
//		//in the future use initial size and check for overflow, then reallocate
//		blockCodes = (COMPRESSEDBLOCKCODE**)realloc(blockCodes, (*blockamount + 1) * sizeof(COMPRESSEDBLOCKCODE*));
//		blockCodes[*blockamount] = (COMPRESSEDBLOCKCODE*)malloc(sizeof(COMPRESSEDBLOCKCODE));
//		blockCodes[*blockamount]->blockSize = blocksize;
//		blockCodes[*blockamount]->brightnessDifference = brightnessDifference;
//		blockCodes[*blockamount]->transformType = affineTransform;
//		blockCodes[*blockamount]->xoffset = offsetx;
//		blockCodes[*blockamount]->yoffset = offsety;
//		*blockamount++;
//	}
//	else
//	{
//		
//	}
//}
////todo FIxMEM LEAKS//not done due to being wrong
//void fractalCompressionStep1(byte** pixels, unsigned int domainBlockSize, unsigned int blocksX, unsigned int blocksY, COMPRESSEDBLOCKCODE** blockCodes, byte** dictonaryBase, float brightnessCompressionCoef)
//{
//	unsigned int dictionaryBlockSize = domainBlockSize / 2;
//	for(int i = 0; i < blocksX; i++)
//	{
//		for (int j = 0; j < blocksY; j++)
//		{
//			byte** domainDown = downsize(pixels, i * domainBlockSize, j * domainBlockSize, domainBlockSize);
//			int ki = 0;
//			int li = 0;
//			int minDifference = MAXINT;
//			int brightnessDifference = 0;
//			byte affineTransform = 0;
//			for (int k = 0; k < blocksX * 2; k++)
//			{
//				for (int l = 0; l < blocksY * 2; l++)
//				{
//					int cdifference;
//					int cbrightnessDiffValue = 0;
//					cdifference = difference(domainDown, pixels, k * dictionaryBlockSize, l * dictionaryBlockSize, dictionaryBlockSize, &cbrightnessDiffValue, brightnessCompressionCoef);
//					compareAndUpdate(&minDifference, cdifference, &ki, k, &li, l, &affineTransform, 0, &brightnessDifference, cbrightnessDiffValue);
//
//					byte** domainTransformed = rotate90(domainDown, dictionaryBlockSize);
//					cdifference = difference(domainTransformed, pixels, k * dictionaryBlockSize, l * dictionaryBlockSize, dictionaryBlockSize, &cbrightnessDiffValue, brightnessCompressionCoef);
//					compareAndUpdate(&minDifference, cdifference, &ki, k, &li, l, &affineTransform, 1, &brightnessDifference, cbrightnessDiffValue);
//
//					domainTransformed = rotate180(domainDown, dictionaryBlockSize);
//					cdifference = difference(domainTransformed, pixels, k * dictionaryBlockSize, l * dictionaryBlockSize, dictionaryBlockSize, &cbrightnessDiffValue, brightnessCompressionCoef);
//					compareAndUpdate(&minDifference, cdifference, &ki, k, &li, l, &affineTransform, 2, &brightnessDifference, cbrightnessDiffValue);
//
//					domainTransformed = rotate270(domainDown, dictionaryBlockSize);
//					cdifference = difference(domainTransformed, pixels, k * dictionaryBlockSize, l * dictionaryBlockSize, dictionaryBlockSize, &cbrightnessDiffValue, brightnessCompressionCoef);
//					compareAndUpdate(&minDifference, cdifference, &ki, k, &li, l, &affineTransform, 3, &brightnessDifference, cbrightnessDiffValue);
//
//					domainTransformed = flipHorizontal(domainDown, dictionaryBlockSize);
//					cdifference = difference(domainTransformed, pixels, k * dictionaryBlockSize, l * dictionaryBlockSize, dictionaryBlockSize, &cbrightnessDiffValue, brightnessCompressionCoef);
//					compareAndUpdate(&minDifference, cdifference, &ki, k, &li, l, &affineTransform, 4, &brightnessDifference, cbrightnessDiffValue);
//
//					domainTransformed = flipVertical(domainDown, dictionaryBlockSize);
//					cdifference = difference(domainTransformed, pixels, k * dictionaryBlockSize, l * dictionaryBlockSize, dictionaryBlockSize, &cbrightnessDiffValue, brightnessCompressionCoef);
//					compareAndUpdate(&minDifference, cdifference, &ki, k, &li, l, &affineTransform, 5, &brightnessDifference, cbrightnessDiffValue);
//
//					domainTransformed = flipAlongMainDiagonal(domainDown, dictionaryBlockSize);
//					cdifference = difference(domainTransformed, pixels, k * dictionaryBlockSize, l * dictionaryBlockSize, dictionaryBlockSize, &cbrightnessDiffValue, brightnessCompressionCoef);
//					compareAndUpdate(&minDifference, cdifference, &ki, k, &li, l, &affineTransform, 6, &brightnessDifference, cbrightnessDiffValue);
//
//					domainTransformed = flipAlongSubDiagonal(domainDown, dictionaryBlockSize);
//					cdifference = difference(domainTransformed, pixels, k * dictionaryBlockSize, l * dictionaryBlockSize, dictionaryBlockSize, &cbrightnessDiffValue, brightnessCompressionCoef);
//					compareAndUpdate(&minDifference, cdifference, &ki, k, &li, l, &affineTransform, 7, &brightnessDifference, cbrightnessDiffValue);
//				}
//			}
//			/*dictonaryBase[i * blocksX + j][0] = ki;
//			dictonaryBase[i * blocksX + j][1] = li;*/
//			blockCodes[i * blocksX + j]->transformType = affineTransform;
//			blockCodes[i * blocksX + j]->xoffset = li * blocksX;
//			blockCodes[i * blocksX + j]->yoffset = ki * blocksY;
//			blockCodes[i * blocksX + j]->brightnessDifference = brightnessDifference;
//		}
//	}
//}

//int buildDictionary(byte**** dictonary, byte** dictionaryBase, int n, int dictonaryBlockSize) 
//{
//	int size = n;
//	preparing array
//	for (int i = 0; i < n - 1; i++)
//	{
//		for (int j = i + 1; j < n; j++)
//		{
//			if ((dictionaryBase[j][0] == dictionaryBase[i][0]) &&
//				(dictionaryBase[j][1] == dictionaryBase[i][1]))
//			{
//				dictionaryBase[j][0] = -1;
//				dictionaryBase[j][1] = i - (n - size);//every ecountered skip decreases id
//				size--;//for every skip decrease dictonary size
//			}
//		}
//	}
//	building dictionary
//	*dictonary = (byte***)malloc(sizeof(byte***) * size);
//	int k = 0;
//	for (int i = 0; i < n; i++) 
//	{
//		if (dictionaryBase[i][0] != -1)
//		{
//
//		}
//	}
//}


void SaveCompressed(const char* fname, BITMAPFILEHEADER* fheader, BITMAPINFOHEADER* iheader, HEADEROFFCOMFILE* cheader, COMPRESSEDBLOCKCODE** blueCode, COMPRESSEDBLOCKCODE** redCode, COMPRESSEDBLOCKCODE** greenCode)
{
	std::ofstream file(fname, std::ios::binary);
	file.write((char*)fheader, sizeof(BITMAPFILEHEADER));
	file.write((char*)iheader, sizeof(BITMAPINFOHEADER));
	file.write((char*)cheader, sizeof(HEADEROFFCOMFILE));

	for(int i = 0; i < cheader->blueDomainCount; i++)
		file.write((char*)(blueCode[i]), sizeof(COMPRESSEDBLOCKCODE));

	for (int i = 0; i < cheader->greenDomainCount; i++)
		file.write((char*)(greenCode[i]), sizeof(COMPRESSEDBLOCKCODE));

	for (int i = 0; i < cheader->redDomainCount; i++)
		file.write((char*)(redCode[i]), sizeof(COMPRESSEDBLOCKCODE));
}

int powerOf2Before(int number)
{
	int twoInPower = 1;
	while (number - twoInPower >= number / 2)
		twoInPower *= 2;
	return twoInPower;
}

int LoadCompressed(const char* fname, BITMAPFILEHEADER* fheader, BITMAPINFOHEADER* iheader, HEADEROFFCOMFILE* cheader, COMPRESSEDBLOCKCODE*** blueCode, COMPRESSEDBLOCKCODE*** redCode, COMPRESSEDBLOCKCODE*** greenCode)
{
	std::ifstream file(fname, std::ios::binary);
	if (!file)
	{
		std::cout << "can't open file " << fname << "\n";
		return 1;
	}
	file.read((char*)fheader, sizeof(BITMAPFILEHEADER));
	file.read((char*)iheader, sizeof(BITMAPINFOHEADER));
	file.read((char*)cheader, sizeof(HEADEROFFCOMFILE));
	if (fheader->bfType != 0x4D42)
	{
		std::cout << "file " << fname << "is not a bmp file\n";
		return 2;
	}
	*blueCode = (COMPRESSEDBLOCKCODE**)malloc(sizeof(COMPRESSEDBLOCKCODE*) * cheader->blueDomainCount);//чет фигня какаято
	*redCode = (COMPRESSEDBLOCKCODE**)malloc(sizeof(COMPRESSEDBLOCKCODE*) * cheader->redDomainCount);//чет фигня какаято
	*greenCode = (COMPRESSEDBLOCKCODE**)malloc(sizeof(COMPRESSEDBLOCKCODE*) * cheader->greenDomainCount);//чет фигня какаято
	for (int i = 0; i < cheader->blueDomainCount; i++)
	{
		(*blueCode)[i] = (COMPRESSEDBLOCKCODE*)malloc(sizeof(COMPRESSEDBLOCKCODE));
		file.read((char*)(*blueCode)[i], sizeof(COMPRESSEDBLOCKCODE));
	}
	for (int i = 0; i < cheader->greenDomainCount; i++)
	{
		(*greenCode)[i] = (COMPRESSEDBLOCKCODE*)malloc(sizeof(COMPRESSEDBLOCKCODE));
		file.read((char*)(*greenCode)[i], sizeof(COMPRESSEDBLOCKCODE));
	}
	for (int i = 0; i < cheader->redDomainCount; i++)
	{
		(*redCode)[i] = (COMPRESSEDBLOCKCODE*)malloc(sizeof(COMPRESSEDBLOCKCODE));
		file.read((char*)(*redCode)[i], sizeof(COMPRESSEDBLOCKCODE));
	}
	return 0;
}

void copyPixelSquare(float** from, float** to, int offsetxf, int offsetyf, int offsetxt, int offsetyt, int n, float brightnessCompr, int diff)
{
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
		{
			to[i + offsetyt][j + offsetxt] = from[i + offsetyf][j + offsetxf] * brightnessCompr + diff;
			if (from[i + offsetyf][j + offsetxf] * brightnessCompr + diff > 255)
				to[i + offsetyt][j + offsetxt] = 255;
			if (from[i + offsetyf][j + offsetxf] * brightnessCompr + diff < 0)
				to[i + offsetyt][j + offsetxt] = 0;
		}
}

void pixelfromfloat(byte** pixels, float** fpixels, int sizex, int sizey)
{
	for (int i = 0; i < sizey; i++)
		for (int j = 0; j < sizex; j++)
		{
			pixels[i][j] = fpixels[i][j];
		}
}

void fractalDecompressionStep3(byte** pixels, COMPRESSEDBLOCKCODE** blockCodes, int sizex, int sizey, int blockCount)
{
	float** iterPixels = (float**)malloc(sizeof(float*) * sizey);
	for (int i = 0; i < sizex; i++)
	{
		iterPixels[i] = (float*)calloc(sizex ,sizeof(float));
	}
	float** tPixels = (float**)malloc(sizeof(float*) * sizey);
	for (int i = 0; i < sizex; i++)
	{
		tPixels[i] = (float*)calloc(sizex, sizeof(float));
	}
	for (int i = 0; i < sizex; i++)
		for (int j = 0; j < sizey / 2; j++)
			tPixels[i][j] = 255;
	for (int iteration = 0; iteration < 100; iteration++)
	{
		for (int i = 0; i < blockCount; i++)
		{
			for (int j = 0; j < sizey; j++)
				for (int k = 0; k < sizex; k++)
					iterPixels[j][k] = tPixels[j][k];
			COMPRESSEDBLOCKCODE* cblockCode = blockCodes[i];
			float** affineTransformed = nullptr;
			float** downSized = fdownsize(iterPixels, cblockCode->xdoffset, cblockCode->ydoffset, cblockCode->blockSize * 2);
			switch (cblockCode->transformType)//refactor this stupidity
			{
			case 0:copyPixelSquare(downSized, tPixels, 0, 0, cblockCode->xoffset, cblockCode->yoffset, cblockCode->blockSize, cblockCode->contrastCoefficient, cblockCode->brightnessDifference);  break;
			case 1:affineTransformed = frotate90(downSized, cblockCode->blockSize);
				copyPixelSquare(affineTransformed, tPixels, 0, 0, cblockCode->xoffset, cblockCode->yoffset, cblockCode->blockSize, cblockCode->contrastCoefficient, cblockCode->brightnessDifference); break;
			case 2:affineTransformed = frotate180(downSized, cblockCode->blockSize);
				copyPixelSquare(affineTransformed, tPixels, 0, 0, cblockCode->xoffset, cblockCode->yoffset, cblockCode->blockSize, cblockCode->contrastCoefficient, cblockCode->brightnessDifference); break;
			case 3:affineTransformed = frotate270(downSized, cblockCode->blockSize);
				copyPixelSquare(affineTransformed, tPixels, 0, 0, cblockCode->xoffset, cblockCode->yoffset, cblockCode->blockSize, cblockCode->contrastCoefficient, cblockCode->brightnessDifference); break;
			case 4:affineTransformed = fflipHorizontal(downSized, cblockCode->blockSize);
				copyPixelSquare(affineTransformed, tPixels, 0, 0, cblockCode->xoffset, cblockCode->yoffset, cblockCode->blockSize, cblockCode->contrastCoefficient, cblockCode->brightnessDifference); break;
			case 5:affineTransformed = fflipVertical(downSized, cblockCode->blockSize);
				copyPixelSquare(affineTransformed, tPixels, 0, 0, cblockCode->xoffset, cblockCode->yoffset, cblockCode->blockSize, cblockCode->contrastCoefficient, cblockCode->brightnessDifference); break;
			case 6:affineTransformed = fflipAlongMainDiagonal(downSized, cblockCode->blockSize);
				copyPixelSquare(affineTransformed, tPixels, 0, 0, cblockCode->xoffset, cblockCode->yoffset, cblockCode->blockSize, cblockCode->contrastCoefficient, cblockCode->brightnessDifference); break;
			case 7:affineTransformed = fflipAlongSubDiagonal(downSized, cblockCode->blockSize);
				copyPixelSquare(affineTransformed, tPixels, 0, 0, cblockCode->xoffset, cblockCode->yoffset, cblockCode->blockSize, cblockCode->contrastCoefficient, cblockCode->brightnessDifference); break;
			default: std::cout << "affine error" << '\n'; break;
			}
			if (cblockCode->transformType != 0)
			{
				free2fDimensions(affineTransformed, cblockCode->blockSize);
			}
			free2fDimensions(downSized, cblockCode->blockSize);
		}
	}
	pixelfromfloat(pixels, tPixels, sizex, sizey);
}

int main()
{
	BITMAPFILEHEADER* fheader = nullptr;
	BITMAPINFOHEADER* iheader = nullptr;
	fheader = (BITMAPFILEHEADER*)malloc(sizeof(BITMAPFILEHEADER));
	iheader = (BITMAPINFOHEADER*)malloc(sizeof(BITMAPINFOHEADER));
	byte** pixels = nullptr;
	byte*** reftopixels = &pixels;
	LoadPixels("t2.bmp", reftopixels, fheader, iheader);
	byte **blue, **red, **green;
	blue = (byte**)malloc(sizeof(byte*) * iheader->biHeight);
	red = (byte**)malloc(sizeof(byte*) * iheader->biHeight);
	green = (byte**)malloc(sizeof(byte*) * iheader->biHeight);
	for (int i = 0; i < iheader->biHeight; i++)
	{
		blue[i] = (byte*)malloc(sizeof(byte) * iheader->biWidth);
		red[i] = (byte*)malloc(sizeof(byte) * iheader->biWidth);
		green[i] = (byte*)malloc(sizeof(byte) * iheader->biWidth);
	}
	colorChannelSeparator(pixels, blue, green, red, iheader->biWidth, iheader->biHeight);
	colorChannelCombinator(pixels, blue, blue, blue, iheader->biWidth, iheader->biHeight);
	SavePixels("r256b11.bmp", pixels, fheader, iheader);
	colorChannelCombinator(pixels, green, green, green, iheader->biWidth, iheader->biHeight);
	SavePixels("r256g11.bmp", pixels, fheader, iheader);
	colorChannelCombinator(pixels, red, red, red, iheader->biWidth, iheader->biHeight);
	SavePixels("r256r11.bmp", pixels, fheader, iheader);
	HEADEROFFCOMFILE* cheader = (HEADEROFFCOMFILE*)malloc(sizeof(HEADEROFFCOMFILE));
	COMPRESSEDBLOCKCODE** blueCode = (COMPRESSEDBLOCKCODE**)malloc(sizeof(COMPRESSEDBLOCKCODE*)*4096);
	COMPRESSEDBLOCKCODE** greenCode = (COMPRESSEDBLOCKCODE**)malloc(sizeof(COMPRESSEDBLOCKCODE*)* 4096);
	COMPRESSEDBLOCKCODE** redCode = (COMPRESSEDBLOCKCODE**)malloc(sizeof(COMPRESSEDBLOCKCODE*)* 4096);
	int initialBlockSize = powerOf2Before(min(iheader->biHeight, iheader->biWidth)) / 2;
	int blueblocks = 0;
	int redblocks = 0;
	int greenblocks = 0;
	////blue
	//fractalCompressionStep3(blue, 0, 0, initialBlockSize, &blueblocks, blueCode, iheader->biWidth, iheader->biHeight, 100);
	//fractalCompressionStep3(blue, 0, initialBlockSize, initialBlockSize, &blueblocks, blueCode, iheader->biWidth, iheader->biHeight, 100);
	//fractalCompressionStep3(blue, initialBlockSize, 0, initialBlockSize, &blueblocks, blueCode, iheader->biWidth, iheader->biHeight, 100);
	//fractalCompressionStep3(blue, initialBlockSize, initialBlockSize, initialBlockSize, &blueblocks, blueCode, iheader->biWidth, iheader->biHeight, 100);
	////red
	//fractalCompressionStep3(red, 0, 0, initialBlockSize, &redblocks, redCode, iheader->biWidth, iheader->biHeight, 10);
	//fractalCompressionStep3(red, 0, initialBlockSize, initialBlockSize, &redblocks, redCode, iheader->biWidth, iheader->biHeight, 10);
	//fractalCompressionStep3(red, initialBlockSize, 0, initialBlockSize, &redblocks, redCode, iheader->biWidth, iheader->biHeight, 10);
	//fractalCompressionStep3(red, initialBlockSize, initialBlockSize, initialBlockSize, &redblocks, redCode, iheader->biWidth, iheader->biHeight, 10);
	////green
	//fractalCompressionStep3(green, 0, 0, initialBlockSize, &greenblocks, greenCode, iheader->biWidth, iheader->biHeight, 100);
	//fractalCompressionStep3(green, 0, initialBlockSize, initialBlockSize, &greenblocks, greenCode, iheader->biWidth, iheader->biHeight, 100);
	//fractalCompressionStep3(green, initialBlockSize, 0, initialBlockSize, &greenblocks, greenCode, iheader->biWidth, iheader->biHeight, 100);
	//fractalCompressionStep3(green, initialBlockSize, initialBlockSize, initialBlockSize, &greenblocks, greenCode, iheader->biWidth, iheader->biHeight, 100);
	//cheader->blueDomainCount = blueblocks;
	//cheader->greenDomainCount = greenblocks;
	//cheader->redDomainCount = redblocks;
	//SaveCompressed("fcompressed256_1.frc", fheader, iheader, cheader, blueCode, redCode, greenCode);
	//free(fheader);
	//free2Dimensions(pixels, iheader->biHeight);
	//free2Dimensions(blue, iheader->biHeight);
	//free2Dimensions(red, iheader->biHeight);
	//free2Dimensions(green, iheader->biHeight);
	//free(iheader);
	//free2Dimensions((byte**)blueCode, cheader->blueDomainCount);
	//free2Dimensions((byte**)redCode, cheader->redDomainCount);
	//free2Dimensions((byte**)greenCode, cheader->greenDomainCount);
	//free(cheader);
	//endofcompression

	//startofdecompression
	fheader = (BITMAPFILEHEADER*)malloc(sizeof(BITMAPFILEHEADER));
	iheader = (BITMAPINFOHEADER*)malloc(sizeof(BITMAPINFOHEADER));
	cheader = (HEADEROFFCOMFILE*)malloc(sizeof(HEADEROFFCOMFILE));
	COMPRESSEDBLOCKCODE*** ptoblueCode = (COMPRESSEDBLOCKCODE***)malloc(sizeof(COMPRESSEDBLOCKCODE**));
	COMPRESSEDBLOCKCODE*** ptoredCode = (COMPRESSEDBLOCKCODE***)malloc(sizeof(COMPRESSEDBLOCKCODE**));
	COMPRESSEDBLOCKCODE*** ptogreenCode = (COMPRESSEDBLOCKCODE***)malloc(sizeof(COMPRESSEDBLOCKCODE**));
	LoadCompressed("fcompressed256_1.frc", fheader, iheader, cheader, ptoblueCode, ptoredCode, ptogreenCode);
	byte** bluePixels = (byte**)malloc(sizeof(byte*)*iheader->biHeight);
	byte** redPixels = (byte**)malloc(sizeof(byte*)*iheader->biHeight);
	byte** greenPixels = (byte**)malloc(sizeof(byte*)*iheader->biHeight);
	for (int i = 0; i < iheader->biHeight; i++)
	{
		bluePixels[i] = (byte*)calloc(iheader->biWidth, sizeof(byte));
		redPixels[i] = (byte*)calloc(iheader->biWidth, sizeof(byte));
		greenPixels[i] = (byte*)calloc(iheader->biWidth, sizeof(byte));
	}
	fractalDecompressionStep3(bluePixels, *ptoblueCode, iheader->biWidth, iheader->biHeight, cheader->blueDomainCount);
	fractalDecompressionStep3(redPixels, *ptoredCode,  iheader->biWidth, iheader->biHeight, cheader->redDomainCount);
	fractalDecompressionStep3(greenPixels, *ptogreenCode, iheader->biWidth, iheader->biHeight, cheader->greenDomainCount);
	pixels = (byte**)malloc(sizeof(byte*) * iheader->biHeight);//чет фигня какаято
	for (int i = 0; i < iheader->biHeight; i++)
	{
		pixels[i] = (byte*)malloc(sizeof(byte)* iheader->biSizeImage / iheader->biHeight);
	}
	colorChannelCombinator(pixels, bluePixels, greenPixels, redPixels, iheader->biWidth, iheader->biHeight);
	SavePixels("r256_11.bmp", pixels, fheader, iheader);
	colorChannelCombinator(pixels, bluePixels, bluePixels, bluePixels, iheader->biWidth, iheader->biHeight);
	SavePixels("r256b121.bmp", pixels, fheader, iheader);
	colorChannelCombinator(pixels, greenPixels, greenPixels, greenPixels, iheader->biWidth, iheader->biHeight);
	SavePixels("r256g121.bmp", pixels, fheader, iheader);
	colorChannelCombinator(pixels, redPixels, redPixels, redPixels, iheader->biWidth, iheader->biHeight);
	SavePixels("r256r121.bmp", pixels, fheader, iheader);
	free2Dimensions(pixels, iheader->biHeight);
	free2Dimensions(bluePixels, iheader->biHeight);
	free2Dimensions(greenPixels, iheader->biHeight);
	free2Dimensions(redPixels, iheader->biHeight);
    return 0;
}

