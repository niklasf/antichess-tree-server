
#include "losing.h"

void TB_output(typePOS *POS,char *O); // note: losing.h and LCTB.h incompatible

// Basic data format
// Each node has 2 fields: 4 bytes of data and 2 bytes of move

// For the data:
// bit 31 of data set: has trans (in particular, does not have child)
// bit 30 of data set: has child, or has sibling when has trans
// 00 no child, not trans, data is sibling (or zero)
// 01 has child [always the next node], data is sibling (or zero)
// 10 trans, no sibling, data is where trans points to
// 11 trans and sibling, data is where trans points to, sibling is next node

// The move corresponds to the arc that caused arrival here
// it is encoded with FROM as bits 6-11 and TO as bits 0-5,
// bits 12-14 is PROM which is 2-6 for NKBRQ, bit 15 is ep flag

// The data/move for the zeroth node are special.
// The data, after removing bit 30 if extant, gives the tree size
// The move gives the count of "prolog" moves, which appear next in the file

// od of e3c6.done: 54 15 21 40 | 05 00 14 03 | aa 0c 61 01 | a1 0a 59 02
// data is (1<<30)+(0x21<<16)+(0x15<<8)+0x54 for 2168148 nodes (counting zero)
// then move is 5, and the next 10 bytes are
// "14 03  aa 0c  61 01  a1 0a  59 02" for: e2e3 c7c6 f1b5 c6b5 b2b4
// The next 2168147*6 bytes are all data/move packets for each node

// At the end of file, there can be a list of nodes and subtree sizes

// The same format is used with KNOWN files, though with the new system
// there are headers and footers (and stored HASH) to ensure consistency

// Hacks for unsolved moves:
// [old] if data is 0x3fffffff, then UNWON, if (1U<<30) is set, has sibling
// if move>>12 is 7 or more than 8, then UNWON, xor the move with 0xf000

// See also dump_wintree.c for a simplistic parser example

#define SET_CHILD(A,x) (A[x].data|=(1U<<30))
#define DATA_IS_TRANS(A,x) (((A[x].data&(1U<<31)))==(1U<<31))
#define DATA_HAS_CHILD(A,x)\
  (((A[x].data&(3U<<30)))==(1U<<30) && ((A[x].data&0x3fffffff)!=0x3fffffff))
#define TRANS_AND_SIBLING(A,x) (((A[x].data&(3U<<30)))==(3U<<30))
#define DATA_GET_NODE(A,x) (A[x].data&0x3fffffff)
#define DATA_GET_NODEns(A,x) \
  (((A[x].data&0x3fffffff)!=0x3fffffff) ? A[x].data&0x3fffffff : \
   (((A[x].data&(1U<<30))==(1U<<30)) ? ((x)+1) : 0))
#define NEXT_SIBLING(A,x) ((TRANS_AND_SIBLING(A,x)) ? ((x)+1) : \
			   (DATA_IS_TRANS(A,x) ? 0 : DATA_GET_NODEns(A,x)))

////////////////////////////////////////////////////////////////////////

static boolean UNWON; static uint64 *ARR;
#define GET_ARRAY_BIT(A,x) (A[(x)>>6]&(1ULL<<((x)&0x3f)))
#define SET_ARRAY_BIT(A,x) A[(x)>>6]|=(1ULL<<((x)&0x3f))

// 0,2,3,4,5,6,8 -> // 15,13,12,11,10,9,7
static uint16 UU(uint16 mv)
{uint16 k=mv>>12;if (mv==0xfedc) return mv;
 if (k>8 || k==7) return mv^0xf000; return mv;}
static boolean VV(uint16 mv)
{uint16 k=mv>>12; return (mv==0xfedc || (k!=7 && k<9));}

static boolean is_unsolved(WIN_NODE *W,uint32 n)
{if (DATA_GET_NODE(W,n)==0x3fffffff) return TRUE;
 if (!VV(W[n].move)) return TRUE; return FALSE;}

static void walk_tree(WIN_NODE *W,uint32 n,boolean t)
{uint32 c; if (is_unsolved(W,n)) {UNWON=TRUE; return;}
 if (t) {if (GET_ARRAY_BIT(ARR,n)) return; SET_ARRAY_BIT(ARR,n);}
 if (DATA_IS_TRANS(W,n)) {walk_tree(W,DATA_GET_NODE(W,n),t); return;}
 if (!t) {if (GET_ARRAY_BIT(ARR,n)) return; SET_ARRAY_BIT(ARR,n);}
 if (!DATA_HAS_CHILD(W,n)) return; c=n+1; walk_tree(W,c,t);
 while ((c=NEXT_SIBLING(W,c))) walk_tree(W,c,t);}

static uint32 COMPUTE_HASH(uint32 n) /* totally random (the method that is) */
{uint32 k=n*n; k+=(n>>10)^((~n)&0x3ff); k+=((n>>5)^((~n)&0x7fff))<<5;
 k+=(((~n)>>15)^(n&0x1f))<<5; k+=(n>>4)&0x55aa55; k+=((~n)>>8)&0xaa55aa;
 return k&0xfffff;}

#define SIGN(x) (((x)>0)?1:-1)
typedef struct {uint32 n; sint32 sz;} tCOUNTED;
static tCOUNTED *COUNTED;
static sint32 SUBTREE_SIZE_IS_COUNTED(WIN_NODE *W,uint32 n)
{uint32 k; while (DATA_IS_TRANS(W,n)) n=DATA_GET_NODE(W,n); k=COMPUTE_HASH(n);
 while (COUNTED[k].n)
 {if (n==COUNTED[k].n) return COUNTED[k].sz; k++; if (k==0x100000) k=0;}
 return 0;}
static void SAVE_SUBTREE_SIZE(WIN_NODE *W,uint32 n,sint32 ct)
{uint32 k; while (DATA_IS_TRANS(W,n)) n=DATA_GET_NODE(W,n);
 k=COMPUTE_HASH(n); while (COUNTED[k].n) {k++; if (k==0x100000) k=0;}
 COUNTED[k].sz=ct; COUNTED[k].n=n; if (!DATA_HAS_CHILD(W,n)) return;
 if (!NEXT_SIBLING(W,n+1)) SAVE_SUBTREE_SIZE(W,n+1,(ABS(ct)-1)*SIGN(ct));}

static uint32 subtree_size(WIN_NODE *W,uint32 n,boolean t)
{uint32 s,u,ct=0,sz=W[0].data&0x3fffffff; if (!n) return sz; sz=(sz+63)/64;
 if (is_unsolved(W,n)) return -1;
 while (DATA_IS_TRANS(W,n)) n=DATA_GET_NODE(W,n);
 if ((s=SUBTREE_SIZE_IS_COUNTED(W,n))) return s;
 for (u=0;u<sz;u++) ARR[u]=0; UNWON=FALSE; walk_tree(W,n,t);
 for (u=0;u<sz;u++) ct+=POPCNT(ARR[u]); if (UNWON) ct=-ct;
 SAVE_SUBTREE_SIZE(W,n,ct); return ct;}

typedef struct {sint32 size; uint32 n; uint16 move;} tRECORD;
static int compare(const void *x,const void *y)
{tRECORD a=*((tRECORD*) (x)); tRECORD b=*((tRECORD*) (y));
 if (a.size<0 && b.size>=0) return -1; if (b.size<0 && a.size>=0) return 1;
 if (ABS(a.size)>ABS(b.size)) return -1;
 if (ABS(a.size)<ABS(b.size)) return 1; return 0;}

static void output_children(WIN_NODE *W,uint32 n,boolean t)
{uint32 z,u,c,r=0; sint32 sz; tRECORD REC[256]; char N[8]; c=n+1;
 REC[r].n=c; REC[r].move=W[c].move; REC[r++].size=subtree_size(W,c,t);
 while ((c=NEXT_SIBLING(W,c)))
 {REC[r].n=c; REC[r].move=W[c].move; REC[r++].size=subtree_size(W,c,t);}
 qsort(REC,r,sizeof(tRECORD),compare); sz=subtree_size(W,n,t);
 printf("Val: %d%s %.2f%%\n",ABS(sz),(sz<0)?"*":" ",
	100.0*ABS(sz)/(W[0].data&0x3fffffff));
 for (u=0;u<r;u++)
 {printf("%s %d%s %.2f%%",Notate(N,REC[u].move),ABS(REC[u].size),
	 REC[u].size<0?"*":" ",100.0*ABS(REC[u].size)/ABS(sz));
  z=DATA_IS_TRANS(W,REC[u].n)?DATA_GET_NODE(W,REC[u].n):REC[u].n;
  printf(" %s\n",DATA_HAS_CHILD(W,z) && !is_unsolved(W,z)
	         && !NEXT_SIBLING(W,z+1) ? Notate(N,W[z+1].move) : "---");}}

static uint32 tree_move(WIN_NODE *W,uint16 mv,uint32 n)
{uint32 c; if (n==-1) return -1; if (!DATA_HAS_CHILD(W,n)) return -1;
 c=n+1; if (UU(W[c].move)==mv) return c;
 while ((c=NEXT_SIBLING(W,c))) if (UU(W[c].move)==mv) return c; return -1;}

static uint32 PROLOG_LEN; static uint64 PROLOG_HASH;
static uint32 align_node(WIN_NODE *W,typePOS *POS)
{uint32 n=0; typeDYNAMIC *D; D=POS->DYN_ROOT+PROLOG_LEN+1;
 while (D!=POS->DYN)
 {D++; n=tree_move(W,D->mv,n); if (n==-1) continue;
  while (DATA_IS_TRANS(W,n)) n=DATA_GET_NODE(W,n);} return n;}

static boolean COUNT_TRANS=FALSE;
static void DoQuery(WIN_NODE *W,typePOS *POS)
{uint32 n=0; n=align_node(W,POS);
 if (!n && POS->DYN->HASH!=PROLOG_HASH) goto NONE;
 if (n!=-1 && DATA_HAS_CHILD(W,n)) {output_children(W,n,COUNT_TRANS); return;}
#ifdef USE_TBS
 if (POPCNT(wBitboardOcc|bBitboardOcc)<=4) {TB_output(POS,NULL); return;}
#endif
 printf("No children\n"); return; NONE: printf("Not in BOOK\n");}

WIN_STRUCT* load_wintree_file_with_counts(char *A)
{FILE *F; WIN_STRUCT *WS; WIN_NODE *W;
 uint32 s,u,d; uint64 sz; char N[8]; char B[256];
 F=fopen(A,"rb"); if (!F) {sprintf(B,"PROOFS/%s",A); F=fopen(B,"rb");}
 if (!F) {printf("Cannot open %s\n",A); return NULL;}
 printf("Loading win-tree %s\n",A);
 WS=malloc(sizeof(WIN_STRUCT)); WS->W=malloc(4096); W=WS->W;
 fread(&(W[0].data),4,1,F); fread(&(W[0].move),2,1,F);
 d=W[0].data&0x3fffffff; s=W[0].move; WS->MOVE_LIST=malloc(s*sizeof(uint16));
 for (u=0;u<s;u++) fread(&(WS->MOVE_LIST[u]),2,1,F);
 if (s) printf("Move list:");
 for (u=0;u<s;u++) printf(" %s",Notate(N,WS->MOVE_LIST[u]));
 if (s) printf("\n");
 sz=((uint64) d)*sizeof(WIN_NODE); WS->W=realloc(WS->W,sz+65536); W=WS->W;
 for (u=1;u<d;u++)
 {fread(&(WS->W[u].data),4,1,F); fread(&(WS->W[u].move),2,1,F);}
 printf("Tree size is %d [%lldmb]\n",d,sz>>20);
 uint32 node; sint32 size;
 while (fread(&node,4,1,F))
 {fread(&size,4,1,F);
  if (!SUBTREE_SIZE_IS_COUNTED(W,node)) SAVE_SUBTREE_SIZE(W,node,size);}
 fclose(F); return WS;}

int main(int argc,char **argv)
{WIN_STRUCT *WS; WIN_NODE *W; typePOS POS[1]; uint64 u,sz; int i,c;
 memset((void*) POS,0,sizeof(typePOS));
 POS->ZTAB=CALLOC(65536,sizeof(HASH)); magic_mult_init(); REV_LIMIT=125;
 POS->DYN_ROOT=CALLOC(MAX_PLY,sizeof(typeDYNAMIC)); POS->DYN=POS->DYN_ROOT+1;
 init_position(POS,"startpos"); printf ("PARSE WIN-TREE\n"); fflush(stdout);
 if (!strcmp(argv[1],"--trans")) {COUNT_TRANS=TRUE; c=2;} else c=1;
#ifdef USE_TBS
 simple_tb_setup();
#endif
 // COUNT_TRANS is incompatible with pre-counts
 COUNTED=malloc(0x100000*sizeof(tCOUNTED)); // hash of subgraph counts
 for (u=0;u<0x100000;u++) COUNTED[u].n=COUNTED[u].sz=0;
 WS=load_wintree_file_with_counts(argv[c]); W=WS->W;
 PROLOG_LEN=W[0].move; if (PROLOG_LEN)
 {char N[8]; printf("prolog");
  for (i=0;i<PROLOG_LEN;i++) printf(" %s",Notate(N,WS->MOVE_LIST[i]));
  printf("\n"); fflush(stdout);}
 printf ("LOSING CHESS\n"); fflush(stdout);
 for (i=0;i<PROLOG_LEN;i++) MakeMove(POS,WS->MOVE_LIST[i]);
 PROLOG_HASH=POS->DYN->HASH; sz=W[0].data&0x3fffffff; ARR=malloc(sz/8+64);
 printf("Ready for input\n"); fflush(stdout);
 while (TRUE)
 {char A[4096]; fgets (A,4000,stdin); A[strlen(A)-1] = 0;
  if (!strcmp(A,"quit")) return FALSE;
  if (!memcmp(A,"query ",6)) // need to start at PROLOG_POS
  {init_position(POS,"startpos"); parse_moves(POS,A+6);
   DoQuery(W,POS); printf("QUERY COMPLETE\n"); fflush(stdout);}}
 return 0;}
