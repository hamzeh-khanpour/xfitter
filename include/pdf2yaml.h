#ifndef pdf2yaml_h
#define pdf2yaml_h

#include <yaml.h>
#include "c2yaml.h"
#include "list.h"

// Info
typedef struct info_node_s {
        enum {STRING, DARRAY} node_type;
        char* key;
        union {
                char* string;
                struct {
                        double *p;
                        double size;
                } darray;
        } value;
} Info_Node;

typedef List Info;

void info_free(Info *info);
Info *info_add_node_str(Info *info, char* key, char *value);
Info *info_add_node_darray(Info *info, char* key, double *darray, int size);

void info_save(const Info *info, FILE *output);

Info_Node *info_node_where(Info *info, char *key);

Info *info_load(FILE *input);

// Pdf member
typedef struct Pdf_s {
Info *info;

int nx;
double *x;

int nq;
double *q;

int n_pdf_flavours;
int *pdf_flavours;

double ***val; /* over x, q, pdf_flavour */
} Pdf;

typedef struct PdfSet_s {
        Info *info;
        int n_members;
        Pdf *members;
} PdfSet;

void pdf_initialize(Pdf *pdf, int nx, int nq, int n_pdf_flavours);
int load_lhapdf6_member(Pdf *pdf, char *path);
int save_lhapdf6_member(Pdf *pdf, char *path);
void pdf_free(Pdf *pdf);

//void pdf_set_initialize(PdfSet *pdf_set);
int load_lhapdf6_set(PdfSet *pdf_set, char *path);
int save_lhapdf6_set(PdfSet *pdf_set, char *path);
void pdf_set_free(PdfSet *pdf_set);

//utility function for int/double convertion to char*
char *n2str(char *s, double n );
#endif
