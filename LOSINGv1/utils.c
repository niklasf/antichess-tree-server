
#include "losing.h"

#include <sys/time.h>
double Now ()
{uint64 x; struct timeval tv; gettimeofday(&tv,NULL);
 x=tv.tv_sec; x*=1000000; x+=tv.tv_usec; return x/1000000.0;}

void dump_path (typePOS *POS)
{typeDYNAMIC *D = POS->DYN_ROOT + 1; char N[64];
 while (D != POS->DYN) {printf("%s ", Notate(N, (D+1)->mv)); D++;}
 printf ("\n"); }

void spit(PN_NODE *N)
{if (!N->bad) printf("WON"); else if (!N->good) printf("LOST");
 else if (N->bad==MY_INFoo && N->good==MY_INFoo) printf("INF/INF");
 else if (N->good==MY_INFoo) printf("INF/%d",N->bad);
 else if (N->bad==MY_INFoo) printf("%d/INF",N->good);
  // else if (N->killer) printf("[%d/%d]",N->good,N->bad);
 else printf("%d/%d",N->good,N->bad);}

static void board_fen (typePOS* POSITION, char* I)
{int tr = 7, co = 0, c = 0, i, p;
 for (i = A1; i <= H8; i++) POSITION->sq[i] = 0;
 while (TRUE)
 {p=I[c++]; if (p==0) return;
  switch (p)
  {case '/': tr--; co = 0; break;
   case 'p': POSITION->sq[co + 8 * tr] = bEnumP; co++; break;
   case 'b': POSITION->sq[co + 8 * tr] = bEnumB; co++; break;
   case 'n': POSITION->sq[co + 8 * tr] = bEnumN; co++; break;
   case 'r': POSITION->sq[co + 8 * tr] = bEnumR; co++; break;
   case 'q': POSITION->sq[co + 8 * tr] = bEnumQ; co++; break;
   case 'k': POSITION->sq[co + 8 * tr] = bEnumK; co++; break;
   case 'P': POSITION->sq[co + 8 * tr] = wEnumP; co++; break;
   case 'B': POSITION->sq[co + 8 * tr] = wEnumB; co++; break;
   case 'N': POSITION->sq[co + 8 * tr] = wEnumN; co++; break;
   case 'R': POSITION->sq[co + 8 * tr] = wEnumR; co++; break;
   case 'Q': POSITION->sq[co + 8 * tr] = wEnumQ; co++; break;
   case 'K': POSITION->sq[co + 8 * tr] = wEnumK; co++; break;
   case '1': co += 1; break; case '2': co += 2; break;
   case '3': co += 3; break; case '4': co += 4; break;
   case '5': co += 5; break; case '6': co += 6; break;
   case '7': co += 7; break; case '8': co += 8; break;}
  if ((tr == 0) && (co >= 8)) break;}}

static void parse_fen (typePOS* POS, char* I)
{char i[1024]; boolean ok; int ep;
 sscanf (I,"%s",i); memset(POS->DYN_ROOT,0,MAX_PLY*sizeof (typeDYNAMIC));
 POS->DYN=POS->DYN_ROOT+1; POS->DYN->rev=0;
 if (!strcmp(i,"startpos"))
 {board_fen(POS,"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
  POSITION->wtm=TRUE; POS->DYN->ep=POS->DYN->rev=0; return;}
 board_fen(POS,i); I+=strlen(i)+1; sscanf(I,"%s",i);
 if (i[0]=='w') POS->wtm=TRUE; else if (i[0]=='b') POS->wtm=FALSE;
 I+=strlen(i)+1; sscanf(I,"%s",i); I+=strlen(i)+1; sscanf(I,"%s",i);
 POS->DYN->ep=0;
 if (!strcmp (i,"-")) ep=0; else {ep=(i[0]-'a')+8*(i[1]-'1'); ok=0; }
 if (ep)
 { if (POS->wtm)
     { if (FILE (ep) != FA && (POS->sq[ep - 9] == wEnumP)) ok = TRUE;
       if (FILE (ep) != FH && (POS->sq[ep - 7] == wEnumP)) ok = TRUE; }
   else
     { if (FILE (ep) != FA && (POS->sq[ep + 7] == bEnumP)) ok = TRUE;
       if (FILE (ep) != FH && (POS->sq[ep + 9] == bEnumP)) ok = TRUE; }
   if (ok) POS->DYN->ep = ep; }}

boolean parse_moves(typePOS* POS,char* I)
{uint16 moves[256],mv; char T[8]; int i,u; while (I[0]==' ') I++;
 while (I[0])
 {sscanf(I,"%s",T); mv=get_move(POS,T); u=GenMoves(POS,moves);
  if (mv==0xfedc) {MakeNullMove(POS); goto FOUND;}
  for (i=0;moves[i];i++) if (mv==moves[i])
  {if (!MakeMoveCheck(POS,mv,0)) // should never Rep now
   {printf("Repetition!\n"); /* SaveBook(); */ exit(-1);} break;}
  if (i==u) {printf("Move not found %s\n",T); return FALSE;}
  FOUND: if (I[strlen(T)]==0) break; I+=strlen(T)+1;} return TRUE;}

static void init_bitboards (typePOS *POS)
{int i, pi, sq; POS->DYN->HASH=0; for (i=0;i<14;i++) POS->bitboard[i]=0;
 for (sq=A1;sq<=H8;sq++)
  if ((pi=POS->sq[sq]))
  {POS->DYN->HASH^=Zobrist(pi,sq); POS->bitboard[pi]|=SqSet[sq];}
 wBitboardOcc=wBitboardK|wBitboardQ|wBitboardR|
              wBitboardB|wBitboardN|wBitboardP;
 bBitboardOcc=bBitboardK|bBitboardQ|bBitboardR|
              bBitboardB|bBitboardN|bBitboardP;
 POS->OCCUPIED=wBitboardOcc|bBitboardOcc;
 if (POS->DYN->ep) POS->DYN->HASH^=ZobristEP[POS->DYN->ep&7];
 if (POS->wtm) POS->DYN->HASH^=ZobristWTM;}

void init_position (typePOS* POS, char* I)
{char *J; J=strstr (I,"moves "); parse_fen(POS,I); init_bitboards(POS);
 Validate(POS); if (J) parse_moves(POS,J+6); Validate(POS);}

char* Notate (char *M, uint16 move)
{ int fr, to; char c[16] = "01nbrqk7"; fr = FROM (move); to = TO (move);
  if (move==0xfedc) {sprintf(M,"NULL"); return M;} // is d7 to e3
  if (move==0xedc) {sprintf(M,"NULL"); return M;} // is d7 to e3
  if ((move>>12)>8 || (move>>12)==7) move^=0xf000; // hack for unsolved proof
  sprintf (M, "%c%c%c%c", 'a' + (fr & 7), '1' + ((fr >> 3) & 7),
           'a' + (to & 7), '1' + ((to >> 3) & 7));
  if (move & (7 << 12)) sprintf (M + 4, "%c", c[move >> 12]); return M;}

uint16 get_move (typePOS *POS, char *I)
{uint16 mv; int k,pi; char A[8]="0pnbrqk";
 if (!strcmp(I,"NULL")) return 0xfedc;
 mv=(I[2]-'a') + ((I[3]-'1')<<3) + ((I[0]-'a')<<6) + ((I[1]-'1')<<9);
 if (strlen(I)==5) {for (k=2;k<=6;k++) if (I[4]==A[k]) mv|=k<<12;}
 pi=POSITION->sq[FROM(mv)];
 if (POSITION->DYN->ep && TO(mv)==POSITION->DYN->ep &&
     ((POSITION->wtm && pi==wEnumP) || (!POSITION->wtm && pi==bEnumP)))
   mv|=1<<15; return mv;}

void SaveWinTree(WIN_NODE *WIN,uint16 *ML,int w,char *filename)
{uint32 u; FILE *F; F=fopen(filename,"w");
 WIN[0].move=w; fwrite(&(WIN[0].data),4,1,F); fwrite(&(WIN[0].move),2,1,F);
 for (u=0;u<w;u++) fwrite(&(ML[u]),2,1,F);
 for (u=1;u<(WIN[0].data&0x3fffffff);u++)
 {fwrite(&(WIN[u].data),4,1,F); fwrite(&(WIN[u].move),2,1,F);} fclose(F);
 printf("Saved to %s, size %d\n",filename,(WIN[0].data&0x3fffffff));}

#define SET_CHILD(A,x) (A[x].data|=(1U<<30))

WIN_STRUCT* LoadWintreeFile(char *A,boolean SILENT)
{FILE *F; WIN_STRUCT *WS; WIN_NODE *W;
 uint32 s,u,d; uint64 sz; char N[8]; char B[256]; B[0]=0;
 F=fopen(A,"rb"); if (!F) {sprintf(B,"PROOFS/%s",A); F=fopen(B,"rb");}
 if (!F) {printf("Cannot open %s\n",A); return NULL;}
 if (!SILENT) printf("Loading win-tree %s\n",B[0]?B:A);
 WS=malloc(sizeof(WIN_STRUCT)); WS->W=malloc(4096); W=WS->W;
 fread(&(W[0].data),4,1,F); fread(&(W[0].move),2,1,F);
 d=W[0].data&0x3fffffff; s=W[0].move; WS->MOVE_LIST=malloc(s*sizeof(uint16));
 for (u=0;u<s;u++) fread(&(WS->MOVE_LIST[u]),2,1,F);
 if (s && !SILENT)
 {printf("Move list:");
  for (u=0;u<s;u++) printf(" %s",Notate(N,WS->MOVE_LIST[u])); printf("\n");}
 sz=((uint64) d)*sizeof(WIN_NODE); WS->W=realloc(WS->W,sz+65536);
 for (u=1;u<d;u++)
 {fread(&(WS->W[u].data),4,1,F); fread(&(WS->W[u].move),2,1,F);} fclose(F);
 if (!SILENT) printf("Tree size is %d [%lldmb]\n",d,sz>>20); return WS;}

