
#include "LCTB.h"

void DUMP_PISQ(type_PiSq *PiSq)
{int i; char PI[16]=".PNBRQKZpnbrqk";
  for (i=0;i<6;i++)
    if (PiSq->pi[i]) printf("%d%c%c%c ",i,PI[PiSq->pi[i]],
                            'a'+((PiSq->sq[i])&7),'1'+((PiSq->sq[i])>>3));
  printf("| n:%d wtm:%d pawn:%d %lld %lld\n",
         PiSq->n,PiSq->wtm,PiSq->pawn,PiSq->index,PiSq->reflect);
}

#include <sys/time.h>
uint64 GetClock ()
{ uint64 x; struct timeval tv; gettimeofday (&tv, NULL);
  x = tv.tv_sec; x *= 1000000; x += tv.tv_usec; return x;}

uint64 BINOMIAL(int x,int y)
{uint64 u,t=1; for (u=0;u<y;u++) t*=x--; for (u=0;u<y;u++) t/=(u+1); return t;}

// 1-bit vert, 2-bit horz, 4-bit diag


/* I don't think these are actually used here, only in building ?! */

static void undo_rep2 ()
{ int a, b, s = 0;
  for (a = A1; a <= H8; a++)
    for (b = A1; b < a; b++) UNDO_REP2[s++] = a | (b << 6); s = 0;
  for (a = A2; a <= H7; a++)
    for (b = A2; b < a; b++) UNDO_REP2P[s++] = a | (b << 6); s = 0;
  for (a = A2; a <= H6; a++)
    for (b = A2; b < a; b++) UNDO_REP2Z[s++] = a | (b << 6);}

static void undo_rep3 ()
{ int a, b, c, s = 0;
  for (a = A1; a <= H8; a++) for (b = A1; b < a; b++)
   for (c = A1; c < b; c++) UNDO_REP3[s++] = a | (b << 6) | (c << 12); s=0;
  for (a = A2; a <= H7; a++) for (b = A2; b < a; b++)
   for (c = A2; c < b; c++) UNDO_REP3P[s++] = a | (b << 6) | (c << 12); s=0;
  for (a = A2; a <= H6; a++) for (b = A2; b < a; b++)
   for (c = A2; c < b; c++) UNDO_REP3Z[s++] = a | (b << 6) | (c << 12);}

static void undo_rep4 ()
{ int a, b, c, d, s = 0;
  for (a = A1; a <= H8; a++) for (b = A1; b < a; b++)
   for (c = A1; c < b; c++) for (d = A1; d < c; d++)
    UNDO_REP4[s++] = a | (b << 6) | (c << 12) | (d << 18); s=0;
  for (a = A2; a <= H7; a++) for (b = A2; b < a; b++)
   for (c = A2; c < b; c++) for (d = A2; d < c; d++)
    UNDO_REP4P[s++] = a | (b << 6) | (c << 12) | (d << 18); s=0;
  for (a = A2; a <= H6; a++) for (b = A2; b < a; b++)
   for (c = A2; c < b; c++) for (d = A2; d < c; d++)
    UNDO_REP4Z[s++] = a | (b << 6) | (c << 12) | (d << 18);}

void init_rep2()
{int i,j; for (i=0;i<64;i++) for (j=0;j<64;j++)
 REP2[i][j]= (i>j) ? BINOMIAL(i+1,2)+(j-i) : BINOMIAL(j+1,2)+(i-j);}

void init_rep3()
{int i,j,k,a,b=0,c;
 for (i=0;i<64;i++) for (j=0;j<64;j++) for (k=0;k<64;k++)
 {a=MAX(MAX(i,j),k); c=MIN(MIN(i,j),k);
  if (i==a) b=MAX(j,k); if (j==a) b=MAX(i,k); if (k==a) b=MAX(i,j);
  REP3[i][j][k]=BINOMIAL(a,3)+BINOMIAL(b+1,2)+(c-b);}}

void init_rep4()
{int i,j,k,l,a,b,c,d;
 for (i=0;i<64;i++) for (j=0;j<64;j++) for (k=0;k<64;k++) for (l=0;l<64;l++)
 {a=MAX(MAX(i,j),MAX(k,l)); d=MIN(MIN(i,j),MIN(k,l));
  if ((i==a && j==d) || (i==d && j==a)) {b=MAX(k,l); c=MIN(k,l);}
  if ((i==a && k==d) || (i==d && k==a)) {b=MAX(j,l); c=MIN(j,l);}
  if ((i==a && l==d) || (i==d && l==a)) {b=MAX(j,k); c=MIN(j,k);}
  if ((j==a && k==d) || (j==d && k==a)) {b=MAX(i,l); c=MIN(i,l);}
  if ((j==a && l==d) || (j==d && l==a)) {b=MAX(i,k); c=MIN(i,k);}
  if ((k==a && l==d) || (k==d && l==a)) {b=MAX(i,j); c=MIN(i,j);}
  REP4[i][j][k][l]=BINOMIAL(a,4)+BINOMIAL(b,3)+BINOMIAL(c+1,2)+(d-c);}}

#define ABOVE (~(0x80c0e0f0f8fcfeff))
#define A1B2C3D4 (0x8040201)
void init_pivot2()
{int sq1,sq2,k1,k2,s1,s2,s,k,u=0,w=0;
 for (sq1=A1;sq1<=H8;sq1++) for (sq2=A1;sq2<=H8;sq2++)
 {k1=KEY_MAP[sq1]; k2=KEY_MAP[sq2];
  s1=GRP_ACT[k1][sq1]; s2=GRP_ACT[k2][sq2];
  if (SqSet[s1]&A1B2C3D4 && SqSet[GRP_ACT[k1][sq2]]&ABOVE) k1^=4;
  if (SqSet[s2]&A1B2C3D4 && SqSet[GRP_ACT[k2][sq1]]&ABOVE) k2^=4;
  if (k1==k2) {KEY_MAP2[sq1][sq2]=k1; continue;}
  s1=GRP_ACT[k1][sq1]; s2=GRP_ACT[k2][sq2];
  if (s1<s2) {KEY_MAP2[sq1][sq2]=k1; continue;}
  if (s1>s2) {KEY_MAP2[sq1][sq2]=k2; continue;}
  s1=GRP_ACT[k2][sq1]; s2=GRP_ACT[k1][sq2]; // needs more ?
  if (s1<s2) KEY_MAP2[sq1][sq2]=k2; else KEY_MAP2[sq1][sq2]=k1;}
 if (0) for (sq1=A1;sq1<=H8;sq1++) for (sq2=A1;sq2<=H8;sq2++)
 {k=KEY_MAP2[sq1][sq2]; s1=GRP_ACT[k][sq1]; s2=GRP_ACT[k][sq2];
  printf("%c%c %c%c %d %c%c %c%c\n",
	 (sq1&7)+'a',(sq1>>3)+'1',(sq2&7)+'a',(sq2>>3)+'1',k,
	 (s1&7)+'a',(s1>>3)+'1',(s2&7)+'a',(s2>>3)+'1');}
 for (sq1=A1;sq1<=H8;sq1++) for (sq2=A1;sq2<=H8;sq2++) MAP2_10[sq1][sq2]=0xfff;
 for (sq1=A1;sq1<=H8;sq1++) for (sq2=A1;sq2<=H8;sq2++)
 {k=KEY_MAP2[sq1][sq2]; s1=GRP_ACT[k][sq1]; s2=GRP_ACT[k][sq2];
  if (s1==s2) continue; if (s1>s2) SWAP(s1,s2); MAP2_10[s1][s2]=1;}
 for (s1=A1;s1<=H8;s1++) for (s2=A1;s2<=H8;s2++)
  if (MAP2_10[s1][s2]==1) {MAP2_10[s1][s2]=u; MAP2_10[s2][s1]=u;
                           UNMAP2_10[u]=(s1<<6)|s2; u++;}
 if (0) for (k=0;k<u;k++)
 {s=UNMAP2_10[k]; s2=s&077; s1=s>>6;
   printf("%d %c%c %c%c\n",k,(s1&7)+'a',(s1>>3)+'1',(s2&7)+'a',(s2>>3)+'1');}
 for (s1=A1;s1<=H8;s1++) for (s2=A1;s2<=H8;s2++) ROTATOR2[s1][s2]=0;
 for (w=1;w<8;w++) for (s1=A1;s1<=H8;s1++) for (s2=A1;s2<=H8;s2++)
   if (GRP_ACT[w][s1]==s2 && GRP_ACT[w][s2]==s1) ROTATOR2[s1][s2]|=1<<w;
 for (s1=A1;s1<=H8;s1++) for (s2=A1;s2<=H8;s2++)
   if (GRP_ACT[4][s1]==s1 && GRP_ACT[4][s2]==s2) ROTATOR2[s1][s2]|=1<<4;}

void init_reps()
{init_rep2(); init_rep3(); init_rep4();
 undo_rep2(); undo_rep3(); undo_rep4(); init_pivot2();}

void setup_key_map()
{int sq,r,f,k;
 for (sq=0;sq<64;sq++)
 {r=Rank(sq); f=File(sq); k=0;
  if (r>3) {k|=1; r=7-r;} if (f>3) {k|=2; f=7-f;}
  if (r>f) k|=4; KEY_MAP[sq]=k;}}

void setup_grp_act()
{int sq,r,f,k;
 for (k=0;k<8;k++) for (sq=0;sq<64;sq++)
 {r=Rank(sq); f=File(sq);
  if (k&1) r=7-r; if (k&2) f=7-f; if (k&4) {SWAP(f,r);}
  GRP_ACT[k][sq]=8*r+f;}}

void init_tb_stuff()
{int i; NUM_TBS=0;
 for (i = 0; i < 0100; i++) SqSet[i] |= (1ULL << i);
 setup_key_map(); setup_grp_act(); init_reps();}

int canonical_name(char *A)
{int l,i,j,n=strlen(A); char O[16]="KQRBNPZkqrbnp",SO[8];
 if (n<2) return n; // Z endgame is trivial anyway
 for (i=0;i<n;i++) for (j=0;j<strlen(O);j++) if (O[j]==A[i]) SO[i]=j;
 for (i=n-1;i>=0;i--) for (j=i+1;j<n && SO[j-1]>SO[j];j++)
 {SWAP(A[j-1],A[j]); SWAP(SO[j-1],SO[j]);}
 l=1; while (A[0]==A[l]) l++;
 if (l!=1)
 {char u=A[0]; for (i=l;i<n;i++) A[i-l]=A[i]; for (i=l;i>0;i--) A[n-i]=u;}
 l=1; while (A[0]==A[l]) l++;
 if (l!=1)
 {char u=A[0]; for (i=l;i<n;i++) A[i-l]=A[i]; for (i=l;i>0;i--) A[n-i]=u;}
 l=1; while (A[0]==A[l]) l++;
 if (l>2)
 {char u=A[0]; for (i=l;i<n;i++) A[i-l]=A[i]; for (i=l;i>0;i--) A[n-i]=u;}
 return n;}

#define FLAT_PREFIX "FLAT."
static int RegFileDir (char* NOME, char* DIRECTORY)
{int n=strlen(FLAT_PREFIX); if (memcmp(NOME,FLAT_PREFIX,n)) return 0;
 RegisterTB4(NOME+n,DIRECTORY); return 1;}

#include <fcntl.h>
#include <dirent.h>
#include <string.h>
int GetTBsFromDir (char* A)
{DIR *D; struct dirent *DE; int u=0;
 printf ("info string Reading directory %s\n", A); D=opendir(A);
 if (!D) {printf ("info string Directory %s not found\n", A); return 0;}
 while ((DE=readdir(D))) u+=RegFileDir(DE->d_name,A); closedir(D); return u;}

int TB_INIT=FALSE;
void load_tbs(char *D)
{int pi1, pi2, pi3, pi4, pi5, pi6; int i,u=0; char E[128]; TB_INIT=FALSE;
 NUM_TBS=0; strcpy(COMPRESSION_PREFIX,"iCOMP.");
 for (pi1 = 0; pi1 < 14; pi1++)
  for (pi2 = 0; pi2 < 14; pi2++)
   for (pi3 = 0; pi3 < 14; pi3++)
    for (pi4 = 0; pi4 < 14; pi4++)
     for (pi5 = 0; pi5 < 14; pi5++)
      for (pi6 = 0; pi6 < 14; pi6++)
	  TB_LOOKUP[pi1][pi2][pi3][pi4][pi5][pi6] = 0xffffffff;
 for (i = 0; i < 0x4000; i++) (TB_TABLE + i)->Fdata = NULL;
 InitInitTotalBaseCache(1024);
 sprintf(E,"%s/2/",D); u+=GetTBsFromDir(E);
 sprintf(E,"%s/3/",D); u+=GetTBsFromDir(E);
 sprintf(E,"%s/4/",D); u+=GetTBsFromDir(E);
 if (u<1401) printf("Flat tablebases seem incomplete? Not using them\n");
 else TB_INIT=TRUE;
 // sprintf(E,"%s/5/",D); GetTBsFromDir(E);
}

void simple_tb_setup()
{int pi1, pi2, pi3, pi4, pi5, pi6; int i;
 for (pi1 = 0; pi1 < 14; pi1++) for (pi2 = 0; pi2 < 14; pi2++)
  for (pi3 = 0; pi3 < 14; pi3++) for (pi4 = 0; pi4 < 14; pi4++)
   for (pi5 = 0; pi5 < 14; pi5++) for (pi6 = 0; pi6 < 14; pi6++)
    TB_LOOKUP[pi1][pi2][pi3][pi4][pi5][pi6] = 0xffffffff;
 for (i = 0; i < 0x4000; i++) (TB_TABLE + i)->Fdata = NULL;
 init_tb_stuff();
 strcpy(COMPRESSION_PREFIX,"iCOMP."); InitInitTotalBaseCache(1024);}

void symm(char *O,char *I)
{int i; for (i=0;i<strlen(I);i++)
 {if (I[i]>='A' && I[i]<='Y') O[i]=I[i]+'a'-'A'; if (I[i]=='Z') O[i]='Z';
  if (I[i]>='a' && I[i]<='z') O[i]=I[i]+'A'-'a';}
 O[strlen(I)]=0; canonical_name(O);}
