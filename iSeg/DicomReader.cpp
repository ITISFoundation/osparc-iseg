/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "DicomReader.h"
#include "config.h"

#include "vtkMyGDCMPolyDataReader.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace iseg;

bool DicomReader::opendicom(const char* filename)
{
	depth = 0;
	if ((fp = fopen(filename, "rb")) == NULL)
		return false;
	else
		return true;
}

void DicomReader::closedicom() { fclose(fp); }

unsigned short DicomReader::get_width()
{
	unsigned short i = 0;

	if (read_field(40, 17))
	{
		i = (unsigned short)bin2int();
	}

	width = i;
	return i;
}

unsigned short DicomReader::get_height()
{
	unsigned short i = 0;

	if (read_field(40, 16))
	{
		i = (unsigned short)bin2int();
	}

	height = i;
	return i;
}

bool DicomReader::load_pictureGDCM(const char* filename, float* bits)
{
	return gdcmvtk_rtstruct::GetDicomUsingGDCM(filename, bits, width, height);
	return true;
}

bool DicomReader::load_picture(float* bits)
{
	get_height();
	get_width();
	get_bitdepth();

	unsigned i;

	if (go_label(32736, 16))
	{
		if (fread(buffer1, 1, 4, fp) == 4)
		{
			if (buffer1[0] == 'O' && buffer1[1] == 'W')
			{
				if (fread(buffer1, 1, 4, fp) != 4)
					return false;
			}

			i = (unsigned)buffer1[0] +
				256 *
					((unsigned)buffer1[1] +
					 256 * ((unsigned)buffer1[2] + 256 * (unsigned)buffer1[3]));

			if (i == unsigned(width) * height * bitdepth / 8)
			{
				unsigned char* bits1 = (unsigned char*)malloc(i);
				if (bitdepth == 8)
				{
					size_t readnr = fread(bits1, 1, i, fp);
					if (readnr == i)
					{
						unsigned k = unsigned(width) * (height - 1);
						unsigned j = 0;
						for (unsigned short h = 0; h < height; h++)
						{
							for (unsigned short w = 0; w < width; w++)
							{
								bits[k] = float(bits1[j]);
								k++;
								j++;
							}
							k -= 2 * width;
						}
						free(bits1);
						return true;
					}
					else
					{
						unsigned k = unsigned(width) * (height - 1);
						unsigned j = 0;
						for (unsigned short h = 0; h < height; h++)
						{
							for (unsigned short w = 0; w < width; w++)
							{
								if (j < readnr)
									bits[k] = float(bits1[j]);
								else
									bits[k] = 0;
								k++;
								j++;
							}
							k -= 2 * width;
						}

						free(bits1);
						return true;
					}
				}
				else if (bitdepth == 16)
				{
					size_t readnr = fread(bits1, 1, i, fp);
					if (readnr == i)
					{
						unsigned k = 0;
						unsigned j = unsigned(width) * (height - 1);
						for (unsigned short h = 0; h < height; h++)
						{
							for (unsigned short w = 0; w < width; w++)
							{
								bits[j] = float(bits1[k]);
								k++;
								bits[j] += 256 * float(bits1[k]);
								k++;
								j++;
							}
							j -= (2 * width);
						}

						free(bits1);
						return true;
					}
					else
					{
						unsigned k = 0;
						unsigned j = unsigned(width) * (height - 1);
						for (unsigned short h = 0; h < height; h++)
						{
							for (unsigned short w = 0; w < width; w++)
							{
								if (k + 1 < readnr)
								{
									bits[j] = float(bits1[k]);
									k++;
									bits[j] += 256 * float(bits1[k]);
									k++;
								}
								else
									bits[j] = 0;
								j++;
							}
							j -= (2 * width);
						}

						free(bits1);
						return true;
						//return false;
					}
				}
				else
				{
					free(bits1);
					return false;
				}
			}
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;
}

bool DicomReader::load_picture(float* bits, Point p, unsigned short dx,
							   unsigned short dy)
{
	get_height();
	get_width();
	get_bitdepth();

	unsigned i;
	if (go_label(32736, 16))
	{
		if (fread(buffer1, 1, 4, fp) == 4)
		{
			if (buffer1[0] == 'O' && buffer1[1] == 'W')
			{
				if (fread(buffer1, 1, 4, fp) != 4)
					return false;
			}

			i = (unsigned)buffer1[0] +
				256 *
					((unsigned)buffer1[1] +
					 256 * ((unsigned)buffer1[2] + 256 * (unsigned)buffer1[3]));

			if (i == unsigned(width) * height * bitdepth / 8)
			{
				unsigned char* bits1 = (unsigned char*)malloc(i);
				if (bitdepth == 8)
				{
					if (fread(bits1, 1, i, fp) == i)
					{
						unsigned j, k;
						j = unsigned(dx) * (dy - 1);
						k = unsigned(height - dy - p.py) * width + p.px;

						for (unsigned h = 0; h < dy; h++)
						{
							for (unsigned w = 0; w < dx; w++)
							{
								bits[j++] = float(bits1[k++]);
							}
							k += width - dx;
							j -= 2 * dx;
						}
						free(bits1);
						return true;
					}
					else
					{
						free(bits1);
						return false;
					}
				}
				else if (bitdepth == 16)
				{
					if (fread(bits1, 1, i, fp) == i)
					{
						unsigned j, k;
						j = unsigned(dx) * (dy - 1);
						k = 2 * (unsigned(height - dy - p.py) * width + p.px);

						for (unsigned h = 0; h < dy; h++)
						{
							for (unsigned w = 0; w < dx; w++)
							{
								bits[j] = float(bits1[k]);
								k++;
								bits[j] += 256 * float(bits1[k]);
								k++;
								j++;
							}
							j -= 2 * dx;
							k += 2 * (width - dx);
						}
						free(bits1);
						return true;
					}
					else
					{
						free(bits1);
						return false;
					}
				}
				else
				{
					free(bits1);
					return false;
				}
			}
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;
}

bool DicomReader::load_picture(float* bits, float center, float contrast)
{
	if (load_picture(bits))
	{
		for (unsigned i = 0; i < unsigned(width) * height; i++)
		{
			bits[i] = (bits[i] - center - contrast / 2) * 255.0f / contrast;
		}
		return true;
	}
	else
		return false;
}

bool DicomReader::load_picture(float* bits, float center, float contrast, Point p,
							   unsigned short dx, unsigned short dy)
{
	if (load_picture(bits, p, dx, dy))
	{
		for (unsigned i = 0; i < unsigned(width) * height; i++)
		{
			bits[i] = (bits[i] - center - contrast / 2) * 255.0f / contrast;
		}
		return true;
	}
	else
		return false;
}

bool DicomReader::imagepos(float f[3])
{
	bool ok = false;
	if (read_field(32, 50))
	{
		dsarray2float(f, 3);
		ok = true;
	}
	return ok;
}

bool DicomReader::imageorientation(float f[6])
{
	bool ok = false;
	if (read_field(32, 55))
	{
		dsarray2float(f, 6);
		ok = true;
	}
	return ok;
}

float DicomReader::slicepos()
{
	float f = 0;

	if (read_field(32, 50))
	{
		float loc[3];
		dsarray2float(loc, 3);
		if (read_field(32, 55))
		{
			float orient[6];
			dsarray2float(orient, 6);
			float vec1[3];
			vec1[0] = orient[1] * orient[5] - orient[2] * orient[4];
			vec1[1] = orient[2] * orient[3] - orient[0] * orient[5];
			vec1[2] = orient[0] * orient[4] - orient[1] * orient[3];
			float l = vec1[0] * vec1[0] + vec1[1] * vec1[1] + vec1[2] * vec1[2];
			if (l > 0)
				f = (vec1[0] * loc[0] + vec1[1] * loc[1] + vec1[2] * loc[2]) /
					sqrt(l);
		}
	}

	if (f == 0)
	{
		if (read_field(32, 4161))
		{
			f = ds2float();
		}
		else if (read_field(32, 19))
		{
			f = float(is2int());
		}
	}

	return f;
}

unsigned DicomReader::seriesnr()
{
	unsigned nr = 0;

	if (read_field(32, 17))
		nr = unsigned(is2int());

	return nr;
}

float DicomReader::thickness()
{
	float f = 1;

	if (read_field(24, 136))
	{
		f = ds2float();
	}
	else if (read_field(24, 80))
	{
		f = ds2float();
	}

	return f;
}

Pair DicomReader::pixelsize()
{
	Pair p;
	p.low = p.high = 0;
	if (read_field(40, 48))
	{
		float f = 0;
		float d = 1;
		float m = 0;
		float d1 = 1;

		int pos = 0;

		if (buffer[pos] == '+')
			pos++;
		if (buffer[pos] == '-')
		{
			pos++;
			d = -1;
		}
		while (pos < (int)l && buffer[pos] >= '0' && buffer[pos] <= '9')
		{
			f = f * 10 + float(buffer[pos] - '0');
			pos++;
		}
		if (pos < length && buffer[pos] == '.')
		{
			pos++;
			while (pos < length && buffer[pos] >= '0' && buffer[pos] <= '9')
			{
				f = f * 10 + float(buffer[pos] - '0');
				d *= 10;
				pos++;
			}
		}

		if (pos < (int)l && (buffer[pos] == 'e' || buffer[pos] == 'E'))
		{
			pos++;
			if (buffer[pos] == '+')
				pos++;
			if (buffer[pos] == '-')
			{
				pos++;
				d1 = -1;
			}
			while ((pos < length && buffer[pos] >= '0' && buffer[pos] <= '9') ||
				   buffer[pos] == ' ')
			{
				if (buffer[pos] != ' ')
					m = m * 10 + float(buffer[pos] - '0');
				pos++;
			}
		}

		if (pos < (int)l &&
			(buffer[pos] == '\\' || buffer[pos] == ':' || buffer[pos] == '/'))
		{
			p.low = f / d;
			p.low *= pow(10.0f, d1 * m);
			f = 0;
			d = 1;
			d1 = 1;
			m = 0;
			pos++;

			if (buffer[pos] == '+')
				pos++;
			if (buffer[pos] == '-')
			{
				pos++;
				d = -1;
			}
			while (pos < (int)l && buffer[pos] >= '0' && buffer[pos] <= '9')
			{
				f = f * 10 + float(buffer[pos] - '0');
				pos++;
			}
			if (pos < length && buffer[pos] == '.')
			{
				pos++;
				while (pos < length && buffer[pos] >= '0' && buffer[pos] <= '9')
				{
					f = f * 10 + float(buffer[pos] - '0');
					d *= 10;
					pos++;
				}
			}
			if (pos < (int)l && (buffer[pos] == 'e' || buffer[pos] == 'E'))
			{
				pos++;
				if (buffer[pos] == '+')
					pos++;
				if (buffer[pos] == '-')
				{
					pos++;
					d1 = -1;
				}
				while ((pos < length && buffer[pos] >= '0' &&
						buffer[pos] <= '9') ||
					   buffer[pos] == ' ')
				{
					if (buffer[pos] != ' ')
						m = m * 10 + float(buffer[pos] - '0');
					pos++;
				}
			}

			if (f > 0)
			{
				p.high = f / d;
				p.high *= pow(10.0f, d1 * m);
			}
			else
				p.low = p.high = 1.0f;
		}
		else
			p.high = p.low = 1.0f;
	}
	else if (read_field(40, 52))
	{
		p.high = 1.0f;

		float i, j;
		i = j = 0;
		int pos = 0;

		while (pos < (int)l && buffer[pos] >= '0' && buffer[pos] <= '9')
		{
			i = i * 10 + float(buffer[pos] - '0');
			pos++;
		}
		if (pos < (int)l &&
			(buffer[pos] == '\\' || buffer[pos] == ':' || buffer[pos] == '/'))
		{
			pos++;
			while (pos < length && buffer[pos] >= '0' && buffer[pos] <= '9')
			{
				j = j * 10 + float(buffer[pos] - '0');
				pos++;
			}
			if (j > 0)
				p.low = i / j;
			else
				p.low = 1.0f;
		}
		else
			p.low = 1.0f;
	}

	return p;
}

unsigned short DicomReader::get_bitdepth()
{
	unsigned short i = 16;

	if (read_field(40, 256))
	{
		i = (unsigned short)(buffer[0]) + 256 * (unsigned short)(buffer[1]);
	}

	bitdepth = i;
	return i;
}

bool DicomReader::read_field(unsigned a, unsigned b)
{
	go_start();
	unsigned a1, b1;

	if (read_fieldnr(a1, b1))
	{
		while (a1 < a || ((a1 == a) && (b1 < b)) || depth > 0)
		{
			//			FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","a");
			//			fprintf(fp3,"a%i %i \n",(int)a1,(int)b1);
			//			fclose(fp3);
			if (!next_field())
				return false;
			if (!read_fieldnr(a1, b1))
				return false;
		}
		//		FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","a");
		//		fprintf(fp3,"b%i %i \n",(int)a1,(int)b1);
		//		fclose(fp3);
		if (a1 == a && b1 == b)
		{
			if (read_fieldcontent())
				return true;
			return false;
		}
		else
		{
			go_start();
			return false;
		}
	}
	else
		return false;
}

bool DicomReader::read_fieldnr(unsigned& a, unsigned& b)
{
	if (fread(buffer1, 1, 4, fp) == 4)
	{
		a = (unsigned)buffer1[0] + 256 * (unsigned)buffer1[1];
		b = (unsigned)buffer1[2] + 256 * (unsigned)buffer1[3];

		if (a == 65534 && b == 57344)
		{
			if (fread(buffer1, 1, 4, fp) == 4)
			{
				if (buffer1[3] == 255 && buffer1[2] == 255 &&
					buffer1[1] == 255 && buffer1[0] == 255)
				{
					depth++;
					//FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","a");
					//fprintf(fp3,"+1 %i\n",(int)depth);
					//fclose(fp3);
				}
				return read_fieldnr(a, b);
			}
			else
				return false;
		}
		else if (a == 65534 && (b == 57357 || b == 57565))
		{
			depth--;
			//FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","a");
			//fprintf(fp3,"-1 %i\n",(int)depth);
			//fclose(fp3);
			if (fread(buffer1, 1, 4, fp) == 4)
				return read_fieldnr(a, b);
			else
				return false;
		}

		/*if((a==65535&&b==65535)||(a==65534&&b==57344)) {
			return read_fieldnr(a,b);
		} else if(a==65534&&(b==57357||b==57565)) {
			if(fread(buffer1, 1, 4, fp)==4) return read_fieldnr(a,b);
			else return false;
		}*/

		//FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","a");
		//fprintf(fp3,"%i %i\n",(int)a,(int)b);
		//fclose(fp3);

		return true;
	}
	else
		return false;
}

bool DicomReader::read_fieldcontent()
{
	read_length();
	if (l == 1000000)
		return false;
	else
	{
		if (l + 1 < length && unsigned(fread(buffer, 1, l, fp)) == l)
		{
			buffer[l] = '\0';

			/*FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test102.txt","w");
			fprintf(fp3,"%s \n",buffer);
			fclose(fp3);*/
			return true;
		}
		else
			return false;
	}
}

bool DicomReader::next_field()
{
	read_length();
	if (l == 1000000)
		return false;
	else
	{
#ifdef _MSC_VER
		if (_fseeki64(fp, l, SEEK_CUR))
#else
		if (fseek(fp, l, SEEK_CUR))
#endif
			return false;
		else
			return true;
	}
}

unsigned DicomReader::read_length()
{
	unsigned i = 0;
	if (fread(buffer1, 1, 4, fp) == 4)
	{
		if (buffer1[0] >= 'A' && buffer1[0] <= 'z' && buffer1[1] >= 'A' &&
			buffer1[1] <= 'z')
		{
			if (((buffer1[0] != 'O' || buffer1[1] != 'B') &&
				 (buffer1[0] != 'O' || buffer1[1] != 'W') &&
				 (buffer1[0] != 'S' || buffer1[1] != 'Q') &&
				 (buffer1[0] != 'U' || buffer1[1] != 'N')))
			{
				i = (unsigned)buffer1[2] + 256 * (unsigned)buffer1[3];
			}
			else
			{
				if (buffer1[2] == 0 || buffer1[3] == 0)
				{
					if (fread(buffer1, 1, 4, fp) == 4)
					{
						if (buffer1[3] == 255 && buffer1[2] == 255 &&
							buffer1[1] == 255 && buffer1[0] == 255)
						{
							i = 0;
							depth++;
							/*FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","a");
							fprintf(fp3,"+1 %i\n",(int)depth);
							fclose(fp3);*/
						}
						else
							i = (unsigned)buffer1[0] +
								256 * ((unsigned)buffer1[1] +
									   256 * ((unsigned)buffer1[2] +
											  256 * (unsigned)buffer1[3]));
					}
					else
						i = 1000000;
				}
				else
					i = (unsigned)buffer1[2] + 256 * (unsigned)buffer1[3];
			}
		}
		else
		{
			if (buffer1[3] == 255 && buffer1[2] == 255 && buffer1[1] == 255 &&
				buffer1[0] == 255)
			{
				i = 0;
				depth++;
				//FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","a");
				//fprintf(fp3,"+1 %i\n",(int)depth);
				//fclose(fp3);
			}
			else
				i = (unsigned)buffer1[0] +
					256 * ((unsigned)buffer1[1] +
						   256 * ((unsigned)buffer1[2] +
								  256 * (unsigned)buffer1[3]));
		}
		l = i;
	}
	else
	{
		l = 1000000;
	}

	return l;
}

void DicomReader::go_start()
{
#ifdef _MSC_VER
	int result = _fseeki64(fp, 128, SEEK_SET);
#else
	int result = fseek(fp, 128, SEEK_SET);
#endif

	if (result == 0)
	{
		if (fread(buffer1, 1, 4, fp) == 4)
		{
			if (buffer1[0] != 'D' || buffer1[1] != 'I' || buffer1[2] != 'C' ||
				buffer1[3] != 'M')
#ifdef _MSC_VER
				_fseeki64(fp, 0, SEEK_SET);
#else
				fseek(fp, 0, SEEK_SET);
#endif
			else
			{
				fread(buffer1, 1, 4, fp);
//				if(buffer1[0]!=2||buffer1[1]!=0||buffer1[2]!=0||buffer1[3]!=0){
#ifdef _MSC_VER
				_fseeki64(fp, 132, SEEK_SET);
#else
				fseek(fp, 132, SEEK_SET);
#endif
				/*				} else {
					read_fieldcontent();
					long l1=buffer[0]+256*(buffer[1]+256*(buffer[2]+256*(buffer[3])));
					fseek(fp,l1, SEEK_CUR);
				}*/
			}
		}
		else
#ifdef _MSC_VER
			_fseeki64(fp, 0, SEEK_SET);
#else
			fseek(fp, 0, SEEK_SET);
#endif
	}
	else
	{
#ifdef _MSC_VER
		_fseeki64(fp, 0, SEEK_SET);
#else
		fseek(fp, 0, SEEK_SET);
#endif
	}
}

bool DicomReader::go_label(unsigned a, unsigned b)
{
	go_start();
	unsigned a1, b1;

	if (read_fieldnr(a1, b1))
	{
		while (a1 < a || b1 < b)
		{
			if (!next_field())
				return false;
			if (!read_fieldnr(a1, b1))
				return false;
		}
		if (a1 == a && b1 == b)
		{
			return true;
		}
		else
		{
			go_start();
			return false;
		}
	}
	else
		return false;
}

float DicomReader::ds2float()
{
	float f = 0;
	float d = 1;
	float m = 0;
	float d1 = 1;

	int pos = 0;

	if (buffer[pos] == '+')
		pos++;
	if (buffer[pos] == '-')
	{
		pos++;
		d = -1;
	}
	while ((pos < length && buffer[pos] >= '0' && buffer[pos] <= '9') ||
		   buffer[pos] == ' ')
	{
		if (buffer[pos] != ' ')
			f = f * 10 + float(buffer[pos] - '0');
		pos++;
	}
	if (pos < length && buffer[pos] == '.')
	{
		pos++;
		while ((pos < length && buffer[pos] >= '0' && buffer[pos] <= '9') ||
			   buffer[pos] == ' ')
		{
			if (buffer[pos] != ' ')
			{
				f = f * 10 + float(buffer[pos] - '0');
				d *= 10;
			}
			pos++;
		}
	}
	if (pos < length && (buffer[pos] == 'e' || buffer[pos] == 'E'))
	{
		pos++;
		if (buffer[pos] == '+')
			pos++;
		if (buffer[pos] == '-')
		{
			pos++;
			d1 = -1;
		}
		while ((pos < length && buffer[pos] >= '0' && buffer[pos] <= '9') ||
			   buffer[pos] == ' ')
		{
			if (buffer[pos] != ' ')
				m = m * 10 + float(buffer[pos] - '0');
			pos++;
		}
	}

	f /= d;
	f *= pow(10.0f, d1 * m);

	return f;
}

float DicomReader::ds2float(int& pos)
{
	float f = 0;
	float d = 1;
	float m = 0;
	float d1 = 1;

	if (pos < 0 || pos >= length)
		return 0;

	if (buffer[pos] == '+')
		pos++;
	if (buffer[pos] == '-')
	{
		pos++;
		d = -1;
	}
	while ((pos < length && buffer[pos] >= '0' && buffer[pos] <= '9') ||
		   buffer[pos] == ' ')
	{
		if (buffer[pos] != ' ')
			f = f * 10 + float(buffer[pos] - '0');
		pos++;
	}
	if (pos < length && buffer[pos] == '.')
	{
		pos++;
		while ((pos < length && buffer[pos] >= '0' && buffer[pos] <= '9') ||
			   buffer[pos] == ' ')
		{
			if (buffer[pos] != ' ')
			{
				f = f * 10 + float(buffer[pos] - '0');
				d *= 10;
			}
			pos++;
		}
	}
	if (pos < length && (buffer[pos] == 'e' || buffer[pos] == 'E'))
	{
		pos++;
		if (buffer[pos] == '+')
			pos++;
		if (buffer[pos] == '-')
		{
			pos++;
			d1 = -1;
		}
		while ((pos < length && buffer[pos] >= '0' && buffer[pos] <= '9') ||
			   buffer[pos] == ' ')
		{
			if (buffer[pos] != ' ')
				m = m * 10 + float(buffer[pos] - '0');
			pos++;
		}
	}

	f /= d;
	f *= pow(10.0f, d1 * m);

	return f;
}

void DicomReader::dsarray2float(float* vals, unsigned short nrvals)
{
	int pos = 0;
	unsigned short counter = 0;
	for (unsigned short i = 0; i < nrvals; i++)
		vals[i] = 0;
	while (counter < nrvals)
	{
		vals[counter] = ds2float(pos);
		if (buffer[pos] != '\\' && counter + 1 != nrvals)
			return;
		pos++;
		counter++;
	}
}

int DicomReader::is2int()
{
	int i = 0;
	int d = 1;

	int pos = 0;

	if (buffer[pos] == '+')
		pos++;
	if (buffer[pos] == '-')
	{
		pos++;
		d = -1;
	}
	while (pos < length && buffer[pos] >= '0' && buffer[pos] <= '9')
	{
		i = i * 10 + int(buffer[pos] - '0');
		pos++;
	}

	i *= d;

	return i;
}

int DicomReader::bin2int()
{
	int i = 0;
	int pos = 0;
	int power = 1;

	while (pos < (int)l)
	{
		i = i + int(buffer[pos]) * power;
		power *= 256;
		pos++;
	}

	return i;
}
