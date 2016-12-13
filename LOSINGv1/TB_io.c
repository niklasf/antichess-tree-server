
#define NEED_TB_720
#include "LCTB.h"

void RemoveTB(TableBase *tb)
{int u; for (u=0;u<720;u++)
 {uint8 *s=S720[u],*p=tb->p;
  TB_LOOKUP[p[s[0]]][p[s[1]]][p[s[2]]][p[s[3]]][p[s[4]]][p[s[5]]]=0xffffffff;}}

int TB_lookup(char *X)
{int i,p[6]={0,0,0,0,0,0}; uint32 v; char I[32];
 strcpy(I,X); canonical_name(I);
 for (i=0;i<6;i++) p[i]=0;
 for (i=0;i<6 && I[i];i++) switch (I[i]) // need efgh,ij,uvwx
 {case 'K': p[i]=6; break; case 'k': p[i]=13; break;
  case 'Q': p[i]=5; break; case 'q': p[i]=12; break;
  case 'R': p[i]=4; break; case 'r': p[i]=11; break;
  case 'B': p[i]=3; break; case 'b': p[i]=10; break;
  case 'N': p[i]=2; break; case 'n': p[i]=9; break;
  case 'P': p[i]=1; break; case 'p': p[i]=8; break; case 'Z': p[i]=7; break;}
 v=TB_LOOKUP[p[0]][p[1]][p[2]][p[3]][p[4]][p[5]];
 if (v==0xffffffff) return 0xffffffff; v&=0x3fff; return v;}

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
int STAT (char *filename)
{struct stat buf[1]; if (stat(filename,buf)==-1) return -1;
 return (buf->st_mode & S_IRUSR) ? 0 : -1;}

#define FLAT_PREFIX "FLAT."
boolean RegisterTB4(char *NAME,char *DIREC)
{char FN[128]; FILE *F; TableBase *tb;
 int v=TB_lookup(NAME); if (v!=-1 && v!=0x3fff) return FALSE;
 sprintf(FN,"%s%s%s",DIREC,FLAT_PREFIX,NAME); F=fopen(FN, "rb");
 if (!F) return FALSE;
 tb = TB_TABLE + NUM_TBS; add_TB(tb,NAME,DIREC);
 tb->FLAT=malloc(tb->size>>2); fread(tb->FLAT,tb->size>>2,1,F);
 strcpy(tb->string,NAME); fclose (F); return TRUE;}

boolean RegisterTB5(char *NAME,char *DIREC)
{char FN[128]; FILE *F; uint8 A[4]; TableBase *tb;
 int i; uint32 num_indici;
 int v=TB_lookup(NAME); if (v!=-1 && v!=0x3fff) return FALSE;
 sprintf(FN,"%s%s%s",DIREC,COMPRESSION_PREFIX+1,NAME); // printf("fn %s\n",FN);
 if (STAT(FN)==-1) return FALSE;
 sprintf(FN,"%s%s%s",DIREC,COMPRESSION_PREFIX,NAME); F=fopen(FN, "rb");
 if (!F) return FALSE; fread (A, 1, 4, F);
 if (A[0] != 0xca || A[1] != 0x7f || A[2] != 0x0b || A[3] != 0xaa) return 0;
 tb = TB_TABLE + NUM_TBS; add_TB(tb,NAME,DIREC); tb->DATA=NULL;
 strcpy(tb->string,NAME);
 fread (A, 1, 4, F); tb->BLOCCO_pro=0x10000; fread (A, 1, 4, F);
 fread (A, 1, 4, F); /* Header, number of indices */
 num_indici=0;
 for (i = 0; i < 4; i++) num_indici |= A[i] << (8 * i);
 tb->indici = malloc (4 * num_indici);
 fread (tb->indici, 4, num_indici, F); /* ENDIAN */
 for (i=1;i<num_indici;i++)
   if (tb->indici[i-1]>tb->indici[i]) printf("INDEX %s %d\n",NAME,i);
 fclose (F); tb->num_indici=num_indici; return TRUE;}

void add_TB(TableBase *tb,char *I,char *D)
{int k,u,i,j; uint32 w,v;
 canonical_name(I); v=TB_lookup(I); if (v!=0xffffffff) return;
 // printf("Adding TB %s %s\n",I,D); // I should be canonical name
 sprintf(tb->DIR_NOME,"%s",D); strcpy(tb->string,I); tb->Fdata=NULL;
 for (i=0;i<6;i++) tb->p[i]=0;
 for (i=0;i<6 && I[i];i++)
 {tb->p[i]=0; switch(I[i])
  {case 'K': tb->p[i]=6; break; case 'k': tb->p[i]=13; break;
   case 'Q': tb->p[i]=5; break; case 'q': tb->p[i]=12; break;
   case 'R': tb->p[i]=4; break; case 'r': tb->p[i]=11; break;
   case 'B': tb->p[i]=3; break; case 'b': tb->p[i]=10; break;
   case 'N': tb->p[i]=2; break; case 'n': tb->p[i]=9; break;
   case 'P': tb->p[i]=1; break; case 'p': tb->p[i]=8; break;
   case 'Z': tb->p[i]=7;}}
 for (i=0;i<6;i++)
 {tb->m[i]=1; if (tb->p[i]) tb->m[i]=64;
  if (tb->p[i]==wEnumP || tb->p[i]==bEnumP) tb->m[i]=48;
  if (tb->p[i]==BlockedPawn) tb->m[i]=40;}
 tb->pawn_flag=FALSE;
 for (i=0;i<6;i++)
 {if (tb->p[i]==wEnumP || tb->p[i]==bEnumP || tb->p[i]==BlockedPawn)
     tb->pawn_flag=TRUE;}
 for (k=0;k<6;k++) {for (u=k+1;u<6 && tb->p[u]==tb->p[k];u++); tb->rep[k]=u-k;}
 for (k=5;k>0;k--) if (tb->rep[k-1]!=1) tb->rep[k]=1;
 for (k=0;k<6;k++) if (!tb->p[k]) tb->rep[k]=1;
 if (tb->rep[0]==1) {if (tb->pawn_flag) tb->m[0]/=2; else tb->m[0]=10;}
 for (u=(tb->rep[0]==1)?1:0;u<6;u++) if (tb->rep[u]!=1)
 {tb->m[u]=BINOMIAL(tb->m[u],tb->rep[u]); // stupid compiler warning "u+j<6"
  for (j=1;j<tb->rep[u] && u+j<6;j++) tb->m[u+j]=1;} tb->num=NUM_TBS;
 tb->size=1; for (i=0;i<6;i++) tb->size*=tb->m[i]; v=NUM_TBS++;
 for (u=0;u<720;u++)
 {uint8 *s=S720[u],si[6],*p=tb->p; for (k=0;k<6;k++) si[s[k]]=k;
  w=v; for (k=0;k<6;k++) w|=((uint32) (si[k]))<<(14+3*k);
  TB_LOOKUP[p[s[0]]][p[s[1]]][p[s[2]]][p[s[3]]][p[s[4]]][p[s[5]]]=w;}}
