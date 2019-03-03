#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define QUERY_LENGTH 350
#define DEG 8

union type {
	int intData;
	char charData[30];
	float floatData;
};

struct row {
	int index;
	fpos_t data;
};

struct BTnode {
	int degree;
	struct row keyData[2 * DEG - 1];
	fpos_t childs[2 * DEG];
	int currNodes;
	int isLeaf;
};

struct table {
	char tableName[20];
	int numberOfColumn;
	char columns[10][15];
	int dataType[10];
	int isRoot;
	fpos_t rootpos;
};

struct QueryToken {
	char **token;
	int size;
};

char *getQuery()
{
	char *q = (char *)malloc(sizeof(char) * QUERY_LENGTH);
	scanf("%[^\n]s", q);
	getchar();

	return q;
}
struct QueryToken parseQuery(char *string)
{
	char *scopy = (char *)malloc(sizeof(char) * strlen(string));
	strcpy(scopy, string);
	char *delimeter = " ,()";
	char *token;
	token = strtok(scopy, delimeter);
	int i = 0, n = 0;

	// count no of words
	while (token != NULL) {
		n++; // no of words
		token = strtok(NULL, delimeter);
	}

	scopy = (char *)malloc(sizeof(char) * strlen(string));
	strcpy(scopy, string);

	token = strtok(string, delimeter);
	char **tokenArray = (char **)malloc(sizeof(char *) * n);
	
	while (token != NULL) {
		tokenArray[i] = token;
		i++;

		token = strtok(NULL, delimeter);
	}

	struct QueryToken qt;
	qt.token = tokenArray;
	qt.size = n;

	return qt;
}

int findDataType(char datatype[20])
{
	if (strcmp(datatype, "integer") == 0 || strcmp(datatype, "INTEGER") == 0) {
		return 1;
	}
	if (strcmp(datatype, "string") == 0 || strcmp(datatype, "STRING") == 0) {
		return 2;
	}
	if (strcmp(datatype, "float") == 0 || strcmp(datatype, "FLOAT") == 0) {
		return 3;
	}
	return -1;
}

void putString(char *c, char *t)
{
	int i;
	for (i = 0; t[i] != '\0'; i++) {
		c[i] = t[i];
	}
	c[i] = '\0';
}

void splitChild(struct BTnode *x, int i, struct BTnode *y, FILE *fp, fpos_t *fpos)
{
	struct BTnode z;
	int j;
	z.degree = y->degree;
	z.isLeaf = y->isLeaf;
	z.currNodes = x->degree - 1;
	for (j = 0; j < x->degree - 1; j++) {
		z.keyData[j] = y->keyData[j + x->degree];
	}

	if (y->isLeaf == 0) {
		for (j = 0; j < x->degree; j++)
			z.childs[j] = y->childs[j + x->degree];
	}

	y->currNodes = x->degree - 1;

	for (j = x->currNodes; j >= i + 1; j--) {
		x->childs[j + 1] = x->childs[j];
	}

	x->childs[i + 1] = *fpos;
	fsetpos(fp, fpos);
	fwrite(&z, sizeof(struct BTnode), 1, fp);
	fgetpos(fp, fpos);

	for (j = x->currNodes - 1; j >= i; j--) {
		x->keyData[j + 1] = x->keyData[j];
	}

	x->keyData[i] = y->keyData[x->degree - 1];

	x->currNodes = x->currNodes + 1;
}

void insertNonFull(struct BTnode *root, struct QueryToken qt, FILE *fp, fpos_t *fpos, struct table t)
{
	int i;
	i = root->currNodes - 1;
	if (root->isLeaf == 1) {
		while (i >= 0 && root->keyData[i].index > atoi(qt.token[2])) {
			root->keyData[i + 1] = root->keyData[i];
			i--;
		}

		int j;
		root->keyData[i + 1].index = atoi(qt.token[2]);
		
		root->keyData[i + 1].data=*fpos;
		union type data[10];

		for (j = 2; j < qt.size; j++) {
			int type = t.dataType[j - 2];
			switch (type) {
				case 1:
					data[j - 2].intData = atoi(qt.token[j]);
					break;
				case 2:
					strcpy(data[j - 2].charData, qt.token[j]);
					break;
				case 3:
					data[j - 2].floatData =	round(atof(qt.token[j]) * 100) / 100;
					break;
			}
		}
		
		fsetpos(fp,fpos);
		fwrite(data,sizeof(union type),10,fp);
		fgetpos(fp,fpos);
		
		root->currNodes = root->currNodes + 1;
	} else {
		while (i >= 0 && root->keyData[i].index > atoi(qt.token[2])) {
			i--;
		}
		
		struct BTnode child;
		fsetpos(fp, &(root->childs[i + 1]));
		fread(&child, sizeof(struct BTnode), 1, fp);
		if (child.currNodes == 2 * (root->degree) - 1) {
			splitChild(root, i + 1, &child, fp, fpos);
			fsetpos(fp, &(root->childs[i + 1]));
			fwrite(&child, sizeof(struct BTnode), 1, fp);
			if (root->keyData[i + 1].index < atoi(qt.token[2])) {
				i++;
			}
		}
		fsetpos(fp, &(root->childs[i + 1]));
		fread(&child, sizeof(struct BTnode), 1, fp);
	
		insertNonFull(&child, qt, fp, fpos, t);
		
		fsetpos(fp, &(root->childs[i + 1]));
		fwrite(&child, sizeof(struct BTnode), 1, fp);
	}
}

void insert(struct table *t, FILE *fp, fpos_t *fpos, struct QueryToken qt)
{
	if ((*t).isRoot == 0) {
		struct BTnode bt;
		bt.degree = DEG;
		bt.isLeaf = 1;
		bt.currNodes = 1;

		(bt.keyData[0]).index = atoi(qt.token[2]);
		union type data[10];
		(bt.keyData[0]).data=*fpos;

		for (int i = 0; i < t->numberOfColumn; i++) {
			int type = t->dataType[i];

			switch (type) {
				case 1:
					data[i].intData = atoi(qt.token[2 + i]);
					break;
				case 2:
					strcpy(data[i].charData, qt.token[2 + i]);
					break;
				case 3:
					data[i].floatData = round(atof(qt.token[2 + i]) * 100) / 100;
					break;
			}
		}
		fsetpos(fp, fpos);
		fwrite(data,sizeof(union type),10,fp);
		fgetpos(fp,fpos);

		fsetpos(fp, fpos);
		(*t).isRoot = 1;
		(*t).rootpos = *fpos;
		fwrite(&bt, sizeof(struct BTnode), 1, fp);
		fgetpos(fp, fpos);
		
		fseek(fp, 0, SEEK_SET);
		fwrite(t, sizeof(struct table), 1, fp);
	}

	else {
		struct BTnode bt;
		fsetpos(fp, &((*t).rootpos));
		fread(&bt, sizeof(struct BTnode), 1, fp);
		if (bt.currNodes == 2 * DEG - 1) {
			struct BTnode s;
			s.degree = DEG;
			s.isLeaf = 0;
			s.childs[0] = (*t).rootpos;
			s.currNodes = 0;
			(*t).rootpos = *fpos;
			fsetpos(fp, fpos);
			fwrite(&s, sizeof(struct BTnode), 1, fp);
			fgetpos(fp, fpos);
			
			splitChild(&s, 0, &bt, fp, fpos);
			int i = 0;
			if (s.keyData[0].index < atoi(qt.token[2])) {
				i++;
			}
			
			struct BTnode child;
			fsetpos(fp, &(s.childs[i]));
			fread(&child, sizeof(struct BTnode), 1, fp);
			
			insertNonFull(&child, qt, fp, fpos, *t);
			
			fsetpos(fp, &(s.childs[i]));
			fwrite(&child, sizeof(struct BTnode), 1, fp);
			
			fsetpos(fp, &((*t).rootpos));
			fwrite(&s, sizeof(struct BTnode), 1, fp);
			
			fsetpos(fp, &(s.childs[0]));
			fwrite(&bt, sizeof(struct BTnode), 1, fp);
			
			fseek(fp, 0, SEEK_SET);
			fwrite(t, sizeof(struct table), 1, fp);
		} else {
			insertNonFull(&bt, qt, fp, fpos, *t);
			fsetpos(fp, &((*t).rootpos));
			fwrite(&bt, sizeof(struct BTnode), 1, fp);
		}
	}
}

void center_print(const char *s, int width)
{
	int length = strlen(s);
	int i;
	for (i = 0 ; i <= (width - length) / 2; i++) {
		fputs(" ", stdout);
	}
	fputs(s, stdout);
	i += length;
	for (; i <= width; i++) {
		fputs(" ", stdout);
	}
}

void traverse(FILE *fp, fpos_t root, struct table t)
{
	fsetpos(fp, &root);
	int j;
	struct BTnode bt;
	fread(&bt, sizeof(struct BTnode), 1, fp);
	int i;
	for (i = 0; i < bt.currNodes; i++) {
		if (bt.isLeaf == 0)
			traverse(fp, bt.childs[i], t);

		fsetpos(fp,&bt.keyData[i].data);
		union type data[10];
		fread(data,sizeof(union type),10,fp);

		for (j = 0; j < t.numberOfColumn; j++) {
			int type = t.dataType[j];

			switch (type) {
				case 1:
					printf("	%d	", data[j].intData);
					break;
				case 2:
					//printf("	%s	", data[j].charData);
					printf("%*s%*s", 15+strlen(data[j].charData)/2, data[j].charData, 15-strlen(data[j].charData)/2, "");
					break;
				case 3:
					printf("	%.2f	", data[j].floatData);
					break;
				default:
					break;
			}
		}
		printf("\n");
	}

	if (bt.isLeaf == 0) {
		traverse(fp, bt.childs[i], t);
	}
}

struct row search(FILE *fp, fpos_t root, int k, int *flag)
{
	fsetpos(fp, &root);
	struct BTnode bt;
	fread(&bt, sizeof(struct BTnode), 1, fp);
	int i = 0;
	while (i < bt.currNodes && k > bt.keyData[i].index) {
		i++;
	}

	if (bt.keyData[i].index == k) {
		*flag = 1;
		return bt.keyData[i];
	}

	if (bt.isLeaf == 1) {
		return bt.keyData[i];
	}

	return search(fp, bt.childs[i], k, flag);
}

void update(FILE *fp, fpos_t root, struct table t, int *flag, struct QueryToken qt)
{
	fsetpos(fp, &root);
	struct BTnode bt;
	fread(&bt, sizeof(struct BTnode), 1, fp);
	int i = 0;
	while (i < bt.currNodes && atoi(qt.token[1]) > bt.keyData[i].index) {
		i++;
	}

	if (bt.keyData[i].index == atoi(qt.token[1])) {
		*flag = 1;
		
		fsetpos(fp,&bt.keyData[i].data);
		union type data[10];
		fread(data,sizeof(union type),10,fp);
			
		for (int j = 1; j < t.numberOfColumn; j++) {
			int type = t.dataType[j];

			switch (type) {
				case 1:
					data[j].intData = atoi(qt.token[1 + j]);
					break;
				case 2:
					strcpy(data[j].charData, qt.token[1 + j]);
					break;
				case 3:
					data[j].floatData = round(atof(qt.token[1 + j]) * 100) / 100;
					break;
			}
		}

		fsetpos(fp,&bt.keyData[i].data);
                fwrite(data,sizeof(union type),10,fp);

		fsetpos(fp, &root);
		fwrite(&bt, sizeof(struct BTnode), 1, fp);

		return;
	}

	if (bt.isLeaf == 1) {
		return;
	}

	update(fp, bt.childs[i], t, flag, qt);
}

int main()
{
	struct table t1;
	t1.numberOfColumn = 0;
	int i;

	FILE *fp;
	fpos_t fpos;
	fp = fopen("data.bin", "r+");

	if (fp != NULL) {
		fseek(fp, 0, SEEK_SET);
		fread(&t1, sizeof(struct table), 1, fp);
		printf("%s table already exists.\n", t1.tableName);
		fseek(fp, 0, SEEK_END);
		fgetpos(fp, &fpos);
	} else {
		fp = fopen("data.bin", "w+");
		t1.isRoot = 0;
		fgetpos(fp, &fpos);
	}

	char *query;
	struct QueryToken qt;

	while (1) {
		// printf(" ");
		query = getQuery();

		if (strcmp(query, "exit") == 0) {
			printf("Exiting...\n");
			return 0;
		}

		qt = parseQuery(query);

		// CREATE TABLE TABLE_NAME (SLNO INTEGER, NAME STRING, ROLL INTEGER)
		if ((strcmp(qt.token[0], "create") == 0 || strcmp(qt.token[0], "CREATE") == 0) && qt.size > 3) {
			fp = freopen("data.bin", "w+", fp);
			fgetpos(fp, &fpos);
			strcpy(t1.tableName, qt.token[2]);
			t1.isRoot = 0;
			t1.numberOfColumn = (qt.size - 3) / 2;
			for (i = 0; i < qt.size - 3; i = i + 2) {
				strcpy(t1.columns[i / 2], qt.token[3 + i]);
				t1.dataType[i / 2] = findDataType(qt.token[3 + i + 1]);
			}
			printf("\n");
			fwrite(&t1, sizeof(struct table), 1, fp);
			fgetpos(fp, &fpos);
		}
		// INSERT TABLE_NAME (COLUMN_DATA1, COLUMN_DATA2)
		else if (strcmp(qt.token[0], "insert") == 0 || strcmp(qt.token[0], "INSERT") == 0) {
			if (t1.numberOfColumn == 0) {
				printf("Table not present.\n");
			} else {
				if (t1.isRoot == 0) {
					insert(&t1, fp, &fpos, qt);
				} else {
					int k = atoi(qt.token[2]);
					int flag = 0;
					struct row r = search(fp, t1.rootpos, k, &flag);
					if (flag) {
						printf("The primary key already exists.\n");
					} else {
						insert(&t1, fp, &fpos, qt);
					}
				}
			}
		}
		// UPDATE ID (COL2, COL3, ....)  //START FROM COL2
		else if (strcmp(qt.token[0], "update") == 0 || strcmp(qt.token[0], "UPDATE") == 0) {
			int flag = 0;
			update(fp, t1.rootpos, t1, &flag, qt);
			if (flag) {
				printf("Updated successfully!!\n");
			} else {
				printf("Primary Key Not Found\n");
			}
		}

		// SHOWALL
		else if (strcmp(qt.token[0], "SHOWALL") == 0 ||	strcmp(qt.token[0], "showall") == 0) {
			if (t1.isRoot == 0) {
				printf("No records found.\n");
			} else {
				printf("From %s table : \n", t1.tableName);

				int k;
				for (k = 0; k < t1.numberOfColumn; k++) {
					//printf("	%s	", t1.columns[k]);
					int type = t1.dataType[k];
                                      	switch (type) {
                                                        case 1:
                                                                printf("	%s	", t1.columns[k]);
                                                                break;
                                                        case 2:
                                                                //printf("      %s      ", data[k].charData);
                                                                printf("%*s%*s", 15+strlen(t1.columns[k])/2, t1.columns[k], 15-strlen(t1.columns[k])/2, "");
                                                                break;
                                                        case 3:
                                                                printf("	%s	", t1.columns[k]);
                                                                break;
                                                        default:
                                                                break;
                                                }
				}
				printf("\n");
			
				traverse(fp, t1.rootpos, t1);
			}
		}
		// SHOW WHERE (ID)
		else if ((strcmp(qt.token[0], "SHOW") == 0 || strcmp(qt.token[0], "show") == 0) && qt.size > 2) {

			if (t1.isRoot == 0) {
				printf("No records found.\n");
			} else {
				int k = atoi(qt.token[2]);
				int flag = 0;
				struct row r = search(fp, t1.rootpos, k, &flag);
				if (flag) {
					printf("From %s table where primary key = %d\n", t1.tableName, k);
					int k;
					for (k = 0; k < t1.numberOfColumn; k++) {
						//printf("	%s	", t1.columns[k]);
						int type = t1.dataType[k];
                                                switch (type) {
                                                        case 1:
                                                                printf("	%s	", t1.columns[k]);
                                                                break;
                                                        case 2:
                                                                //printf("      %s      ", data[k].charData);
                                                                printf("%*s%*s", 15+strlen(t1.columns[k])/2, t1.columns[k], 15-strlen(t1.columns[k])/2, "");
                                                                break;
                                                        case 3:
                                                                printf("	%s	", t1.columns[k]);
                                                                break;
                                                        default:
                                                                break;
                                                }

					}
					printf("\n");

					fsetpos(fp,&r.data);
					union type data[10];
					fread(data,sizeof(union type),10,fp);

					for (k = 0; k < t1.numberOfColumn; k++) {
						int type = t1.dataType[k];
						switch (type) {
							case 1:
								printf("	%d	", data[k].intData);
								break;
							case 2:
								//printf("	%s	", data[k].charData);
								printf("%*s%*s", 15+strlen(data[k].charData)/2, data[k].charData, 15-strlen(data[k].charData)/2, "");
								break;
							case 3:
								printf("	%.2f	", data[k].floatData);
								break;
							default:
								break;
						}
					}
					printf("\n");
				} else {
					printf("Primary key not found.\n");
				}
			}
		} else {
			printf("Wrong syntax!!!");
		}
	}

	fclose(fp);

	return 0;
}
