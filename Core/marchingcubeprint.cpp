/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "marchingcubeprint.h"
#include "Precompiled.h"
#include <string>
#include <vector>

using namespace std;

vectissuedescr *hypermeshascii_read(const char *filename)
{
	vectissuedescr *tissdescvec = new vectissuedescr; //(vector<tissuedescript> *)
	FILE *fp;
	//	FILE *fp1=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","w");

	vector<tissuedescript>::iterator itinner, itouter;
	vector<V3F> *vertices = new vector<V3F>;
	V3F vertex;
	tissuedescript ts;

	if ((fp = fopen(filename, "r")) == NULL)
		return NULL;

	unsigned dummy, corner1, corner2, corner3;
	//	float x,y,z;
	int ch = fgetc(fp);
	int count = 0;
	char str[5] = "aaaa";
	char name[100];
	string string1, string2, exteriorstring;
	exteriorstring.assign("Exterior");
	bool exterior1, exterior2;

	while (feof(fp) == 0)
	{
		if (ch == (int)'*')
		{
			//			fseek(fp,ftell(fp),SEEK_SET);
			fread(&str, 4, 1, fp);
			if (str[0] == 'n' && str[1] == 'o' && str[2] == 'd' && str[3] == 'e')
			{
				fscanf(fp, "(%u,%f,%f,%f", &dummy, &vertex.v[0], &vertex.v[1],
							 &vertex.v[2]);
				vertices->push_back(vertex);
			}
			else if (str[0] == 't' && str[1] == 'r' && str[2] == 'i' && str[3] == '3')
			{
				fscanf(fp, "(%u,1,%u,%u,%u)", &dummy, &corner1, &corner2, &corner3);
				if (!exterior1)
				{
					itinner->index_array->push_back(corner1 - 1);
					itinner->index_array->push_back(corner2 - 1);
					itinner->index_array->push_back(corner3 - 1);
				}
				if (!exterior2)
				{
					itouter->index_array->push_back(corner1 - 1);
					itouter->index_array->push_back(corner3 - 1);
					itouter->index_array->push_back(corner2 - 1);
				}
				//				fprintf(fp1,"%u %u %u; ",corner1,corner2,corner3);
			}
			else if (str[0] == 'c' && str[1] == 'o' && str[2] == 'm' && str[3] == 'p')
			{
				fscanf(fp, "onent(%u,", &dummy);
				fgetc(fp);
				ch = fgetc(fp);
				count = 0;
				while (ch != '-')
				{
					name[count] = ch;
					count++;
					ch = fgetc(fp);
				}
				name[count] = str[4];
				string1.assign(name);

				itinner = tissdescvec->vtd.begin();
				while (itinner != tissdescvec->vtd.end() && itinner->name != string1)
					itinner++;
				if (itinner == tissdescvec->vtd.end())
				{
					if (string1 == exteriorstring)
						exterior1 = true;
					else
					{
						exterior1 = false;
						tissdescvec->vtd.push_back(ts);
						itinner = tissdescvec->vtd.end();
						itinner--;
						itinner->index_array = new vector<unsigned>;
						itinner->name.assign(name);
						itinner->rgb[0] = itinner->rgb[1] = itinner->rgb[2] = 1;
						itinner->vertex_array = vertices;
					}
				}

				ch = fgetc(fp);
				count = 0;
				while (ch != '"')
				{
					name[count] = ch;
					count++;
					ch = fgetc(fp);
				}
				name[count] = str[4];
				string2.assign(name);
				itouter = tissdescvec->vtd.begin();
				while (itouter != tissdescvec->vtd.end() && itouter->name != string2)
					itouter++;
				if (itouter == tissdescvec->vtd.end())
				{
					if (string2 == exteriorstring)
						exterior2 = true;
					else
					{
						exterior2 = false;
						tissdescvec->vtd.push_back(ts);
						itouter = --(tissdescvec->vtd.end());
						itouter->index_array = new vector<unsigned>;
						itouter->name.assign(name);
						itouter->rgb[0] = itouter->rgb[1] = itouter->rgb[2] = 1;
						itouter->vertex_array = vertices;

						itinner = tissdescvec->vtd.begin();
						while (itinner->name != string1)
							itinner++;
					}
				}
			}
		}

		ch = fgetc(fp);
	}

	fclose(fp);
	//	fclose(fp1);
	return tissdescvec;
}
