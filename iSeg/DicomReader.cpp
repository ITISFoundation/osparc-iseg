/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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

namespace iseg {

bool DicomReader::Opendicom(const char* filename)
{
	m_Depth = 0;
	if ((m_Fp = fopen(filename, "rb")) == nullptr)
		return false;
	else
		return true;
}

void DicomReader::Closedicom() { fclose(m_Fp); }

unsigned short DicomReader::GetWidth()
{
	unsigned short i = 0;

	if (ReadField(40, 17))
	{
		i = (unsigned short)Bin2int();
	}

	m_Width = i;
	return i;
}

unsigned short DicomReader::GetHeight()
{
	unsigned short i = 0;

	if (ReadField(40, 16))
	{
		i = (unsigned short)Bin2int();
	}

	m_Height = i;
	return i;
}

bool DicomReader::LoadPictureGdcm(const char* filename, float* bits)
{
	return gdcmvtk_rtstruct::GetDicomUsingGDCM(filename, bits, m_Width, m_Height);
}

bool DicomReader::LoadPicture(float* bits)
{
	GetHeight();
	GetWidth();
	GetBitdepth();

	unsigned i;

	if (GoLabel(32736, 16))
	{
		if (fread(m_Buffer1, 1, 4, m_Fp) == 4)
		{
			if (m_Buffer1[0] == 'O' && m_Buffer1[1] == 'W')
			{
				if (fread(m_Buffer1, 1, 4, m_Fp) != 4)
					return false;
			}

			i = (unsigned)m_Buffer1[0] +
					256 *
							((unsigned)m_Buffer1[1] +
									256 * ((unsigned)m_Buffer1[2] + 256 * (unsigned)m_Buffer1[3]));

			if (i == unsigned(m_Width) * m_Height * m_Bitdepth / 8)
			{
				unsigned char* bits1 = (unsigned char*)malloc(i);
				if (m_Bitdepth == 8)
				{
					size_t readnr = fread(bits1, 1, i, m_Fp);
					if (readnr == i)
					{
						unsigned k = unsigned(m_Width) * (m_Height - 1);
						unsigned j = 0;
						for (unsigned short h = 0; h < m_Height; h++)
						{
							for (unsigned short w = 0; w < m_Width; w++)
							{
								bits[k] = float(bits1[j]);
								k++;
								j++;
							}
							k -= 2 * m_Width;
						}
						free(bits1);
						return true;
					}
					else
					{
						unsigned k = unsigned(m_Width) * (m_Height - 1);
						unsigned j = 0;
						for (unsigned short h = 0; h < m_Height; h++)
						{
							for (unsigned short w = 0; w < m_Width; w++)
							{
								if (j < readnr)
									bits[k] = float(bits1[j]);
								else
									bits[k] = 0;
								k++;
								j++;
							}
							k -= 2 * m_Width;
						}

						free(bits1);
						return true;
					}
				}
				else if (m_Bitdepth == 16)
				{
					size_t readnr = fread(bits1, 1, i, m_Fp);
					if (readnr == i)
					{
						unsigned k = 0;
						unsigned j = unsigned(m_Width) * (m_Height - 1);
						for (unsigned short h = 0; h < m_Height; h++)
						{
							for (unsigned short w = 0; w < m_Width; w++)
							{
								bits[j] = float(bits1[k]);
								k++;
								bits[j] += 256 * float(bits1[k]);
								k++;
								j++;
							}
							j -= (2 * m_Width);
						}

						free(bits1);
						return true;
					}
					else
					{
						unsigned k = 0;
						unsigned j = unsigned(m_Width) * (m_Height - 1);
						for (unsigned short h = 0; h < m_Height; h++)
						{
							for (unsigned short w = 0; w < m_Width; w++)
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
							j -= (2 * m_Width);
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

bool DicomReader::LoadPicture(float* bits, Point p, unsigned short dx, unsigned short dy)
{
	GetHeight();
	GetWidth();
	GetBitdepth();

	unsigned i;
	if (GoLabel(32736, 16))
	{
		if (fread(m_Buffer1, 1, 4, m_Fp) == 4)
		{
			if (m_Buffer1[0] == 'O' && m_Buffer1[1] == 'W')
			{
				if (fread(m_Buffer1, 1, 4, m_Fp) != 4)
					return false;
			}

			i = (unsigned)m_Buffer1[0] +
					256 *
							((unsigned)m_Buffer1[1] +
									256 * ((unsigned)m_Buffer1[2] + 256 * (unsigned)m_Buffer1[3]));

			if (i == unsigned(m_Width) * m_Height * m_Bitdepth / 8)
			{
				unsigned char* bits1 = (unsigned char*)malloc(i);
				if (m_Bitdepth == 8)
				{
					if (fread(bits1, 1, i, m_Fp) == i)
					{
						unsigned j, k;
						j = unsigned(dx) * (dy - 1);
						k = unsigned(m_Height - dy - p.py) * m_Width + p.px;

						for (unsigned h = 0; h < dy; h++)
						{
							for (unsigned w = 0; w < dx; w++)
							{
								bits[j++] = float(bits1[k++]);
							}
							k += m_Width - dx;
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
				else if (m_Bitdepth == 16)
				{
					if (fread(bits1, 1, i, m_Fp) == i)
					{
						unsigned j, k;
						j = unsigned(dx) * (dy - 1);
						k = 2 * (unsigned(m_Height - dy - p.py) * m_Width + p.px);

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
							k += 2 * (m_Width - dx);
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

bool DicomReader::LoadPicture(float* bits, float center, float contrast)
{
	if (LoadPicture(bits))
	{
		for (unsigned i = 0; i < unsigned(m_Width) * m_Height; i++)
		{
			bits[i] = (bits[i] - center - contrast / 2) * 255.0f / contrast;
		}
		return true;
	}
	else
		return false;
}

bool DicomReader::LoadPicture(float* bits, float center, float contrast, Point p, unsigned short dx, unsigned short dy)
{
	if (LoadPicture(bits, p, dx, dy))
	{
		for (unsigned i = 0; i < unsigned(m_Width) * m_Height; i++)
		{
			bits[i] = (bits[i] - center - contrast / 2) * 255.0f / contrast;
		}
		return true;
	}
	else
		return false;
}

bool DicomReader::Imagepos(float f[3])
{
	bool ok = false;
	if (ReadField(32, 50))
	{
		Dsarray2float(f, 3);
		ok = true;
	}
	return ok;
}

bool DicomReader::Imageorientation(float f[6])
{
	bool ok = false;
	if (ReadField(32, 55))
	{
		Dsarray2float(f, 6);
		ok = true;
	}
	return ok;
}

float DicomReader::Slicepos()
{
	float f = 0;

	if (ReadField(32, 50))
	{
		float loc[3];
		Dsarray2float(loc, 3);
		if (ReadField(32, 55))
		{
			float orient[6];
			Dsarray2float(orient, 6);
			float vec1[3];
			vec1[0] = orient[1] * orient[5] - orient[2] * orient[4];
			vec1[1] = orient[2] * orient[3] - orient[0] * orient[5];
			vec1[2] = orient[0] * orient[4] - orient[1] * orient[3];
			float l = vec1[0] * vec1[0] + vec1[1] * vec1[1] + vec1[2] * vec1[2];
			if (l > 0)
			{
				f = (vec1[0] * loc[0] + vec1[1] * loc[1] + vec1[2] * loc[2]) / std::sqrt(l);
			}
		}
	}

	if (f == 0)
	{
		if (ReadField(32, 4161))
		{
			f = Ds2float();
		}
		else if (ReadField(32, 19))
		{
			f = float(Is2int());
		}
	}

	return f;
}

unsigned DicomReader::Seriesnr()
{
	unsigned nr = 0;

	if (ReadField(32, 17))
		nr = unsigned(Is2int());

	return nr;
}

float DicomReader::Thickness()
{
	float f = 1;

	if (ReadField(24, 136))
	{
		f = Ds2float();
	}
	else if (ReadField(24, 80))
	{
		f = Ds2float();
	}

	return f;
}

Pair DicomReader::Pixelsize()
{
	Pair p;
	p.low = p.high = 0;
	if (ReadField(40, 48))
	{
		float f = 0;
		float d = 1;
		float m = 0;
		float d1 = 1;

		int pos = 0;

		if (m_Buffer[pos] == '+')
			pos++;
		if (m_Buffer[pos] == '-')
		{
			pos++;
			d = -1;
		}
		while (pos < (int)m_L && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9')
		{
			f = f * 10 + float(m_Buffer[pos] - '0');
			pos++;
		}
		if (pos < length && m_Buffer[pos] == '.')
		{
			pos++;
			while (pos < length && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9')
			{
				f = f * 10 + float(m_Buffer[pos] - '0');
				d *= 10;
				pos++;
			}
		}

		if (pos < (int)m_L && (m_Buffer[pos] == 'e' || m_Buffer[pos] == 'E'))
		{
			pos++;
			if (m_Buffer[pos] == '+')
				pos++;
			if (m_Buffer[pos] == '-')
			{
				pos++;
				d1 = -1;
			}
			while ((pos < length && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9') ||
						 m_Buffer[pos] == ' ')
			{
				if (m_Buffer[pos] != ' ')
					m = m * 10 + float(m_Buffer[pos] - '0');
				pos++;
			}
		}

		if (pos < (int)m_L &&
				(m_Buffer[pos] == '\\' || m_Buffer[pos] == ':' || m_Buffer[pos] == '/'))
		{
			p.low = f / d;
			p.low *= pow(10.0f, d1 * m);
			f = 0;
			d = 1;
			d1 = 1;
			m = 0;
			pos++;

			if (m_Buffer[pos] == '+')
				pos++;
			if (m_Buffer[pos] == '-')
			{
				pos++;
				d = -1;
			}
			while (pos < (int)m_L && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9')
			{
				f = f * 10 + float(m_Buffer[pos] - '0');
				pos++;
			}
			if (pos < length && m_Buffer[pos] == '.')
			{
				pos++;
				while (pos < length && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9')
				{
					f = f * 10 + float(m_Buffer[pos] - '0');
					d *= 10;
					pos++;
				}
			}
			if (pos < (int)m_L && (m_Buffer[pos] == 'e' || m_Buffer[pos] == 'E'))
			{
				pos++;
				if (m_Buffer[pos] == '+')
					pos++;
				if (m_Buffer[pos] == '-')
				{
					pos++;
					d1 = -1;
				}
				while ((pos < length && m_Buffer[pos] >= '0' &&
									 m_Buffer[pos] <= '9') ||
							 m_Buffer[pos] == ' ')
				{
					if (m_Buffer[pos] != ' ')
						m = m * 10 + float(m_Buffer[pos] - '0');
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
	else if (ReadField(40, 52))
	{
		p.high = 1.0f;

		float i, j;
		i = j = 0;
		int pos = 0;

		while (pos < (int)m_L && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9')
		{
			i = i * 10 + float(m_Buffer[pos] - '0');
			pos++;
		}
		if (pos < (int)m_L &&
				(m_Buffer[pos] == '\\' || m_Buffer[pos] == ':' || m_Buffer[pos] == '/'))
		{
			pos++;
			while (pos < length && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9')
			{
				j = j * 10 + float(m_Buffer[pos] - '0');
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

unsigned short DicomReader::GetBitdepth()
{
	unsigned short i = 16;

	if (ReadField(40, 256))
	{
		i = (unsigned short)(m_Buffer[0]) + 256 * (unsigned short)(m_Buffer[1]);
	}

	m_Bitdepth = i;
	return i;
}

bool DicomReader::ReadField(unsigned a, unsigned b)
{
	GoStart();
	unsigned a1, b1;

	if (ReadFieldnr(a1, b1))
	{
		while (a1 < a || ((a1 == a) && (b1 < b)) || m_Depth > 0)
		{
			if (!NextField())
				return false;
			if (!ReadFieldnr(a1, b1))
				return false;
		}
		if (a1 == a && b1 == b)
		{
			if (ReadFieldcontent())
				return true;
			return false;
		}
		else
		{
			GoStart();
			return false;
		}
	}
	else
		return false;
}

bool DicomReader::ReadFieldnr(unsigned& a, unsigned& b)
{
	if (fread(m_Buffer1, 1, 4, m_Fp) == 4)
	{
		a = (unsigned)m_Buffer1[0] + 256 * (unsigned)m_Buffer1[1];
		b = (unsigned)m_Buffer1[2] + 256 * (unsigned)m_Buffer1[3];

		if (a == 65534 && b == 57344)
		{
			if (fread(m_Buffer1, 1, 4, m_Fp) == 4)
			{
				if (m_Buffer1[3] == 255 && m_Buffer1[2] == 255 &&
						m_Buffer1[1] == 255 && m_Buffer1[0] == 255)
				{
					m_Depth++;
				}
				return ReadFieldnr(a, b);
			}
			else
				return false;
		}
		else if (a == 65534 && (b == 57357 || b == 57565))
		{
			m_Depth--;
			if (fread(m_Buffer1, 1, 4, m_Fp) == 4)
				return ReadFieldnr(a, b);
			else
				return false;
		}

		return true;
	}
	else
		return false;
}

bool DicomReader::ReadFieldcontent()
{
	ReadLength();
	if (m_L == 1000000)
		return false;
	else
	{
		if (m_L + 1 < length && unsigned(fread(m_Buffer, 1, m_L, m_Fp)) == m_L)
		{
			m_Buffer[m_L] = '\0';
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool DicomReader::NextField()
{
	ReadLength();
	if (m_L == 1000000)
		return false;
	else
	{
#ifdef _MSC_VER
		if (_fseeki64(m_Fp, m_L, SEEK_CUR))
#else
		if (fseek(m_Fp, m_L, SEEK_CUR))
#endif
			return false;
		else
			return true;
	}
}

unsigned DicomReader::ReadLength()
{
	unsigned i = 0;
	if (fread(m_Buffer1, 1, 4, m_Fp) == 4)
	{
		if (m_Buffer1[0] >= 'A' && m_Buffer1[0] <= 'z' && m_Buffer1[1] >= 'A' &&
				m_Buffer1[1] <= 'z')
		{
			if (((m_Buffer1[0] != 'O' || m_Buffer1[1] != 'B') &&
							(m_Buffer1[0] != 'O' || m_Buffer1[1] != 'W') &&
							(m_Buffer1[0] != 'S' || m_Buffer1[1] != 'Q') &&
							(m_Buffer1[0] != 'U' || m_Buffer1[1] != 'N')))
			{
				i = (unsigned)m_Buffer1[2] + 256 * (unsigned)m_Buffer1[3];
			}
			else
			{
				if (m_Buffer1[2] == 0 || m_Buffer1[3] == 0)
				{
					if (fread(m_Buffer1, 1, 4, m_Fp) == 4)
					{
						if (m_Buffer1[3] == 255 && m_Buffer1[2] == 255 &&
								m_Buffer1[1] == 255 && m_Buffer1[0] == 255)
						{
							i = 0;
							m_Depth++;
						}
						else
						{
							i = (unsigned)m_Buffer1[0] + 256 * ((unsigned)m_Buffer1[1] + 256 * ((unsigned)m_Buffer1[2] + 256 * (unsigned)m_Buffer1[3]));
						}
					}
					else
					{
						i = 1000000;
					}
				}
				else
				{
					i = (unsigned)m_Buffer1[2] + 256 * (unsigned)m_Buffer1[3];
				}
			}
		}
		else
		{
			if (m_Buffer1[3] == 255 && m_Buffer1[2] == 255 &&
					m_Buffer1[1] == 255 && m_Buffer1[0] == 255)
			{
				i = 0;
				m_Depth++;
			}
			else
			{
				i = (unsigned)m_Buffer1[0] + 256 * ((unsigned)m_Buffer1[1] + 256 * ((unsigned)m_Buffer1[2] + 256 * (unsigned)m_Buffer1[3]));
			}
		}
		m_L = i;
	}
	else
	{
		m_L = 1000000;
	}

	return m_L;
}

void DicomReader::GoStart()
{
#ifdef _MSC_VER
	int result = _fseeki64(m_Fp, 128, SEEK_SET);
#else
	int result = fseek(m_Fp, 128, SEEK_SET);
#endif

	if (result == 0)
	{
		if (fread(m_Buffer1, 1, 4, m_Fp) == 4)
		{
			if (m_Buffer1[0] != 'D' || m_Buffer1[1] != 'I' || m_Buffer1[2] != 'C' ||
					m_Buffer1[3] != 'M')
#ifdef _MSC_VER
				_fseeki64(m_Fp, 0, SEEK_SET);
#else
				fseek(m_Fp, 0, SEEK_SET);
#endif
			else
			{
				fread(m_Buffer1, 1, 4, m_Fp);
//				if(buffer1[0]!=2||buffer1[1]!=0||buffer1[2]!=0||buffer1[3]!=0){
#ifdef _MSC_VER
				_fseeki64(m_Fp, 132, SEEK_SET);
#else
				fseek(m_Fp, 132, SEEK_SET);
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
			_fseeki64(m_Fp, 0, SEEK_SET);
#else
			fseek(m_Fp, 0, SEEK_SET);
#endif
	}
	else
	{
#ifdef _MSC_VER
		_fseeki64(m_Fp, 0, SEEK_SET);
#else
		fseek(m_Fp, 0, SEEK_SET);
#endif
	}
}

bool DicomReader::GoLabel(unsigned a, unsigned b)
{
	GoStart();
	unsigned a1, b1;

	if (ReadFieldnr(a1, b1))
	{
		while (a1 < a || b1 < b)
		{
			if (!NextField())
				return false;
			if (!ReadFieldnr(a1, b1))
				return false;
		}
		if (a1 == a && b1 == b)
		{
			return true;
		}
		else
		{
			GoStart();
			return false;
		}
	}
	else
		return false;
}

float DicomReader::Ds2float()
{
	float f = 0;
	float d = 1;
	float m = 0;
	float d1 = 1;

	int pos = 0;

	if (m_Buffer[pos] == '+')
		pos++;
	if (m_Buffer[pos] == '-')
	{
		pos++;
		d = -1;
	}
	while ((pos < length && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9') ||
				 m_Buffer[pos] == ' ')
	{
		if (m_Buffer[pos] != ' ')
			f = f * 10 + float(m_Buffer[pos] - '0');
		pos++;
	}
	if (pos < length && m_Buffer[pos] == '.')
	{
		pos++;
		while ((pos < length && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9') ||
					 m_Buffer[pos] == ' ')
		{
			if (m_Buffer[pos] != ' ')
			{
				f = f * 10 + float(m_Buffer[pos] - '0');
				d *= 10;
			}
			pos++;
		}
	}
	if (pos < length && (m_Buffer[pos] == 'e' || m_Buffer[pos] == 'E'))
	{
		pos++;
		if (m_Buffer[pos] == '+')
			pos++;
		if (m_Buffer[pos] == '-')
		{
			pos++;
			d1 = -1;
		}
		while ((pos < length && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9') ||
					 m_Buffer[pos] == ' ')
		{
			if (m_Buffer[pos] != ' ')
				m = m * 10 + float(m_Buffer[pos] - '0');
			pos++;
		}
	}

	f /= d;
	f *= pow(10.0f, d1 * m);

	return f;
}

float DicomReader::Ds2float(int& pos)
{
	float f = 0;
	float d = 1;
	float m = 0;
	float d1 = 1;

	if (pos < 0 || pos >= length)
		return 0;

	if (m_Buffer[pos] == '+')
		pos++;
	if (m_Buffer[pos] == '-')
	{
		pos++;
		d = -1;
	}
	while ((pos < length && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9') ||
				 m_Buffer[pos] == ' ')
	{
		if (m_Buffer[pos] != ' ')
			f = f * 10 + float(m_Buffer[pos] - '0');
		pos++;
	}
	if (pos < length && m_Buffer[pos] == '.')
	{
		pos++;
		while ((pos < length && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9') ||
					 m_Buffer[pos] == ' ')
		{
			if (m_Buffer[pos] != ' ')
			{
				f = f * 10 + float(m_Buffer[pos] - '0');
				d *= 10;
			}
			pos++;
		}
	}
	if (pos < length && (m_Buffer[pos] == 'e' || m_Buffer[pos] == 'E'))
	{
		pos++;
		if (m_Buffer[pos] == '+')
			pos++;
		if (m_Buffer[pos] == '-')
		{
			pos++;
			d1 = -1;
		}
		while ((pos < length && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9') ||
					 m_Buffer[pos] == ' ')
		{
			if (m_Buffer[pos] != ' ')
				m = m * 10 + float(m_Buffer[pos] - '0');
			pos++;
		}
	}

	f /= d;
	f *= pow(10.0f, d1 * m);

	return f;
}

void DicomReader::Dsarray2float(float* vals, unsigned short nrvals)
{
	int pos = 0;
	unsigned short counter = 0;
	for (unsigned short i = 0; i < nrvals; i++)
		vals[i] = 0;
	while (counter < nrvals)
	{
		vals[counter] = Ds2float(pos);
		if (m_Buffer[pos] != '\\' && counter + 1 != nrvals)
			return;
		pos++;
		counter++;
	}
}

int DicomReader::Is2int()
{
	int i = 0;
	int d = 1;

	int pos = 0;

	if (m_Buffer[pos] == '+')
		pos++;
	if (m_Buffer[pos] == '-')
	{
		pos++;
		d = -1;
	}
	while (pos < length && m_Buffer[pos] >= '0' && m_Buffer[pos] <= '9')
	{
		i = i * 10 + int(m_Buffer[pos] - '0');
		pos++;
	}

	i *= d;

	return i;
}

int DicomReader::Bin2int()
{
	int i = 0;
	int pos = 0;
	int power = 1;

	while (pos < (int)m_L)
	{
		i = i + int(m_Buffer[pos]) * power;
		power *= 256;
		pos++;
	}

	return i;
}

} // namespace iseg
