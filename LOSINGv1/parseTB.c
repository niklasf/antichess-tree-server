
#include "LCTB.h"

static inline uint64 Attacks (typePOS *POSITION, int pi, int sq)
{if (pi>7) pi-=7; if (pi==2) return AttN[sq]; if (pi==3) return AttB(sq);
 if (pi==4) return AttR(sq); if (pi==5) return AttQ(sq);
 if (pi==6) return AttK[sq]; return 0ULL;}

#define FIFTH_RANK 0x000000ff00000000ULL
int GenMoveListBlack (typePOS *POS,uint16 *ml,uint64 *EP)
{int j, p, sq, to, ep = POS->DYN->ep; uint16 *u = ml; uint64 T, U;
 *EP = 0; *ml = 0; if (!bBitboardOcc) return 0;
#if 1
 if (ep)
 {if (FILE (ep) != FH && POS->sq[ep + 9] == bEnumP)
     *ml++ = FlagEP | ((ep + 9) << 6) | (ep);
   if (FILE (ep) != FA && POS->sq[ep + 7] == bEnumP)
     *ml++ = FlagEP | ((ep + 7) << 6) | (ep);}
#endif
 T = ((bBitboardP & ~FILEa) >> 9) & wBitboardOcc;
 while (T)
 {to = BSF (T); T &= (T - 1);
  if (to >= A2) *ml++ = ((to + 9) << 6) | (to);
  else for (p = 9; p <= 13; p++) *ml++ = ((p-7)<<12) | ((to + 9) << 6) | (to);}
 T = ((bBitboardP & ~FILEh) >> 7) & wBitboardOcc;
 while (T)
 {to = BSF (T); T &= (T - 1);
  if (to >= A2) *ml++ = ((to + 7) << 6) | (to);
  else for (p = 9; p <= 13; p++) *ml++ = ((p-7)<<12) | ((to + 7) << 6) | (to);}
 for (j = 9; j <= 13; j++) for (U = POSITION->bitboard[j]; U; U &= (U - 1))
 {sq = BSF (U);  T = Attacks (POS, j, sq) & wBitboardOcc;
  while (T) { to = BSF (T); T &= (T - 1); *ml++ = (sq << 6) | (to);}}
 (*ml) = 0; if (ml != u) return ml - u;
 T=(bBitboardP>>8)&~POS->OCCUPIED; U=(T>>8)&~POS->OCCUPIED&FIFTH_RANK;
 while (T)
 {to = BSF (T); T &= (T - 1);
  if (to >= A2) *ml++ = ((to + 8) << 6) | (to);
  else for (p = 9; p <= 13; p++) *ml++ = ((p-7)<<12) | ((to + 8) << 6) | (to);}
 *EP=U&(((wBitboardP&FIFTH_RANK)<<1)|((wBitboardP&FIFTH_RANK)>>1));
 while (U) { to = BSF (U); U &= (U - 1); *ml++ = ((to + 16) << 6) | (to);}
 for (j = 9; j <= 13; j++) for (U = POSITION->bitboard[j]; U; U &= (U - 1))
 {sq = BSF (U); T = Attacks (POS, j, sq) & ~POS->OCCUPIED;
  while (T) { to = BSF (T); T &= (T - 1); *ml++ = (sq << 6) | (to);}}
 (*ml) = 0; return ml - u;}

#define FOURTH_RANK 0x00000000ff000000ULL
int GenMoveList (typePOS *POS,uint16 *ml,uint64 *EP)
{int j, p, sq, to, ep = POS->DYN->ep; uint16 *u = ml; uint64 T, U;
 if (!POS->wtm) return GenMoveListBlack(POS,ml,EP);
 *EP = 0; *ml = 0; if (!wBitboardOcc) return 0;
 if (ep)
 {if (FILE (ep) != FA && POS->sq[ep - 9] == wEnumP)
     *ml++ = FlagEP | ((ep - 9) << 6) | (ep);
   if (FILE (ep) != FH && POS->sq[ep - 7] == wEnumP)
     *ml++ = FlagEP | ((ep - 7) << 6) | (ep);}
 T = ((wBitboardP & ~FILEa) << 7) & bBitboardOcc;
 while (T)
 {to = BSF (T); T &= (T - 1);
  if (to < A8) *ml++ = ((to - 7) << 6) | (to);
  else for (p = 2; p <= 6; p++) *ml++ = (p << 12) | ((to - 7) << 6) | (to);}
 T = ((wBitboardP & ~FILEh) << 9) & bBitboardOcc;
 while (T)
 {to = BSF (T); T &= (T - 1);
  if (to < A8) *ml++ = ((to - 9) << 6) | (to);
  else for (p = 2; p <= 6; p++) *ml++ = (p << 12) | ((to - 9) << 6) | (to);}
 for (j = 2; j <= 6; j++) for (U = POSITION->bitboard[j]; U; U &= (U - 1))
 {sq = BSF (U);  T = Attacks (POS, j, sq) & bBitboardOcc;
  while (T) { to = BSF (T); T &= (T - 1); *ml++ = (sq << 6) | (to);}}
 (*ml) = 0; if (ml != u) return ml - u;
 T=(wBitboardP<<8)&~POS->OCCUPIED; U=(T<<8)&~POS->OCCUPIED&FOURTH_RANK;
 while (T)
 {to = BSF (T); T &= (T - 1);
  if (to < A8) *ml++ = ((to - 8) << 6) | (to);
  else for (p = 2; p <= 6; p++) *ml++ = (p << 12) | ((to - 8) << 6) | (to);}
 *EP=U&(((bBitboardP&FOURTH_RANK)<<1)|((bBitboardP&FOURTH_RANK)>>1));
 while (U) { to = BSF (U); U &= (U - 1); *ml++ = ((to - 16) << 6) | (to);}
 for (j = 2; j <= 6; j++) for (U = POSITION->bitboard[j]; U; U &= (U - 1))
 {sq = BSF (U); T = Attacks (POS, j, sq) & ~POS->OCCUPIED;
  while (T) { to = BSF (T); T &= (T - 1); *ml++ = (sq << 6) | (to);}}
 (*ml) = 0; return ml - u;}

#define DEBUG 0
uint32 MakePiSqCap(type_PiSq *PiSq,uint16 mv) // must be a cap, not ep
{uint32 u; uint8 to=TO(mv),from=FROM(mv); int f,t,k;
 if (DEBUG) {printf("MAKE1 "); DUMP_PISQ(PiSq);}
 for (k=0;k<PiSq->n;k++) // TODO BlockedPawn
 {if (PiSq->sq[k]==from) f=k; if (PiSq->sq[k]==to) t=k;}
 u=mv|(PiSq->pi[t]<<16);
 PiSq->sq[f]=to; if (mv>>12) PiSq->pi[f]=(mv>>12)+7*!PiSq->wtm;
 PiSq->sq[t]=PiSq->sq[PiSq->n-1]; PiSq->pi[t]=PiSq->pi[PiSq->n-1];
 PiSq->sq[PiSq->n-1]=PiSq->pi[PiSq->n-1]=0;
 PiSq->n--; PiSq->wtm^=1;
 PiSq->pawn=FALSE;
 for (k=0;k<PiSq->n;k++)
   if (PiSq->pi[k]==wEnumP || PiSq->pi[k]==bEnumP || PiSq->pi[k]==BlockedPawn)
     PiSq->pawn=TRUE;
 if (DEBUG) {printf("MAKE2 %d ",u>>16); DUMP_PISQ(PiSq);}
 return u;}

void UndoPiSqCap(type_PiSq *PiSq,uint32 mv) // must be a cap, not ep
{uint8 to=TO(mv),from=FROM(mv); int f,k;
 if (DEBUG) {printf("UNDO1 %d ",mv>>16); DUMP_PISQ(PiSq);}
 for (k=0;k<PiSq->n;k++) // TODO BlockedPawn
 {if (PiSq->sq[k]==to) f=k;}
 PiSq->sq[f]=from; if ((mv>>12)&7) PiSq->pi[f]=(PiSq->pi[f]<7)?wEnumP:bEnumP;
 PiSq->pi[PiSq->n]=mv>>16; PiSq->sq[PiSq->n]=to; PiSq->n++; PiSq->wtm^=1;
 PiSq->pawn=FALSE;
 for (k=0;k<PiSq->n;k++)
   if (PiSq->pi[k]==wEnumP || PiSq->pi[k]==bEnumP || PiSq->pi[k]==BlockedPawn)
     PiSq->pawn=TRUE;
 if (DEBUG) {printf("UNDO2 "); DUMP_PISQ(PiSq);}}

uint32 MakePiSqProm(type_PiSq *PiSq,uint16 mv) // must be prom, not cap
{uint8 to=TO(mv),from=FROM(mv); int f,k;
 for (k=0;k<PiSq->n;k++) {if (PiSq->sq[k]==from) f=k;}
 PiSq->pi[f]=(mv>>12)+7*!PiSq->wtm; PiSq->sq[f]=to; PiSq->wtm^=1;
 PiSq->pawn=FALSE;
 for (k=0;k<PiSq->n;k++)
   if (PiSq->pi[k]==wEnumP || PiSq->pi[k]==bEnumP || PiSq->pi[k]==BlockedPawn)
     PiSq->pawn=TRUE;
 return mv;}

void UndoPiSqProm(type_PiSq *PiSq,uint32 mv) // must be prom, not cap
{uint8 to=TO(mv),from=FROM(mv); int f,k; PiSq->wtm^=1;
 for (k=0;k<PiSq->n;k++) {if (PiSq->sq[k]==to) f=k;}
 PiSq->pi[f]=(PiSq->pi[f]<7)?wEnumP:bEnumP; PiSq->sq[f]=from; PiSq->pawn=TRUE;}

static uint8 EPtabW1[8]={B4,C4,D4,E4,F4,G4,H4,G4};
static uint8 EPtabW2[8]={B4,A4,B4,C4,D4,E4,F4,G4};
static uint8 EPtabB1[8]={B5,C5,D5,E5,F5,G5,H5,G5};
static uint8 EPtabB2[8]={B5,A5,B5,C5,D5,E5,F5,G5};
uint32 MakePiSqPlain(type_PiSq *PiSq,uint16 mv) // must be plain
{uint8 to=TO(mv),from=FROM(mv); int f,k; PiSq->wtm^=1;
 for (k=0;k<PiSq->n;k++) {if (PiSq->sq[k]==from) f=k;} PiSq->sq[f]=to;
 PiSq->ep=0;
 if (PiSq->pi[f]==wEnumP && (to-from)==16)
 {for (k=0;k<PiSq->n;k++)
   if (PiSq->pi[k]==bEnumP &&
       (PiSq->sq[k]==EPtabW1[to&7] || PiSq->sq[k]==EPtabW2[to&7]))
     PiSq->ep=to-8;}
 if (PiSq->pi[f]==bEnumP && (to-from)==-16)
 {for (k=0;k<PiSq->n;k++)
   if (PiSq->pi[k]==wEnumP &&
       (PiSq->sq[k]==EPtabB1[to&7] || PiSq->sq[k]==EPtabB2[to&7]))
     PiSq->ep=to+8;}}

void UndoPiSqPlain(type_PiSq *PiSq,uint32 mv) // must be prom, not cap
{uint8 to=TO(mv),from=FROM(mv); int f,k; PiSq->wtm^=1; PiSq->ep=0;
 for (k=0;k<PiSq->n;k++) {if (PiSq->sq[k]==to) f=k;} PiSq->sq[f]=from;}

////////////////////////////////////////////////////////////////////////

static char* NOTATE (uint32 move, char* M)
{ int fr, to, pr; char c[16] = "01nbrqk01nbrqk";
  fr = FROM (move); to = TO (move);
  sprintf (M, "%c%c%c%c", 'a' + (fr & 7), '1' + ((fr >> 3) & 7),
           'a' + (to & 7), '1' + ((to >> 3) & 7));
  if ((move>>12)&7) { pr = move >> 12; sprintf(M+4,"%c",c[pr]);} return M;}

static char STRING[32];
static char *res_string(int v,int conv)
{if (v==1) strcpy(STRING,"Win"); if (v==2) strcpy(STRING,"Draw");
 if (v>=3 && !conv) sprintf(STRING,"Loss.In.%d",v-3);
 if (v>=3 && conv) sprintf(STRING,"[Loss.In.%d]",v-3); return STRING;}

static char STRING2[32];
static char *move_str(int v,int conv)
{if (v==1) strcpy(STRING2,"Loss"); if (v==2) strcpy(STRING2,"Draw");
 if (v>=3 && !conv) sprintf(STRING2,"Win.In.%d",v-3);
 if (v>=3 && conv) sprintf(STRING2,".[Win.In.%d].",v-3); return STRING2;}

static boolean do_result(type_PiSq *PiSq,boolean print,char *OP)
{uint8 v; if (!TB_PiSq_score(PiSq,&v))
 {if (print) printf("No result\nComplete with moves\n"); return FALSE;}
 if (print)
 {if (!OP) printf("Val: %s 0x%llx . .\n",res_string(v,0),PiSq->index);
  else sprintf(OP,"Val: %s 0x%llx . .\n",res_string(v,0),PiSq->index);}
 return TRUE;}

#define Bitboard2(x,y) ( ( (uint64) 1) << (x) ) | ( ( (uint64) 1) << (y) )
const uint64 EPtableW[8]=
  { Bitboard2 (B5,B5), Bitboard2 (A5,C5),Bitboard2 (B5,D5),Bitboard2 (C5,E5),
    Bitboard2 (D5,F5), Bitboard2 (E5,G5),Bitboard2 (F5,H5),Bitboard2 (G5,G5)};
const uint64 EPtableB[8]=
  { Bitboard2 (B4,B4), Bitboard2 (A4,C4),Bitboard2 (B4,D4), Bitboard2 (C4,E4),
    Bitboard2 (D4,F4), Bitboard2 (E4,G4),Bitboard2 (F4,H4), Bitboard2 (G4,G4)};

void CLONE_PISQ(type_PiSq *O,type_PiSq *I) {memcpy(O,I,sizeof(type_PiSq));}

void BITBOARD(typePOS *POSITION,type_PiSq *PiSq)
{int i; wBitboardR=wBitboardQ=wBitboardN=wBitboardB=wBitboardP=wBitboardK=0;
 bBitboardR=bBitboardQ=bBitboardN=bBitboardB=bBitboardP=bBitboardK=0;
 POSITION->wtm=PiSq->wtm;
 for (i=0;i<6;i++)
 {if (!PiSq->pi[i]) continue; POSITION->sq[PiSq->sq[i]]=PiSq->pi[i];
  POSITION->bitboard[PiSq->pi[i]]|=(1ULL<<PiSq->sq[i]);}
  wBitboardOcc=wBitboardK|wBitboardQ|wBitboardR|
               wBitboardB|wBitboardN|wBitboardP;
  bBitboardOcc=bBitboardK|bBitboardQ|bBitboardR|
               bBitboardB|bBitboardN|bBitboardP;
  POSITION->OccupiedBW=wBitboardOcc|bBitboardOcc;}

static char OUTS[128][32]; static uint8 num;

boolean do_move_results(type_PiSq *PiSq,boolean print,uint8 *bv,char *OP)
{typePOS POSITION[1]; int sq,n,i,k,cp,c; uint64 A,x; type_PiSq PiSq2[1];
 uint16 mv,ML[256]; uint8 v; uint32 u; char STR[64];
 boolean b,retval=TRUE,conv,con2,bc=FALSE;
 if (!do_result(PiSq,print,OP)) return FALSE; *bv=0xff;
 POSITION->DYN_ROOT=malloc(MAXIMUM_PLY*sizeof(typeDYNAMIC));
 POSITION->DYN=POSITION->DYN_ROOT+1;
 for (sq=A1;sq<=H8;sq++) POSITION->sq[sq]=0;
 POSITION->DYN->rev=0; POSITION->DYN->ep=0; // 0
 BITBOARD(POSITION,PiSq); n=GenMoveList(POS,ML,&x); // no ep caps generated
 free(POSITION->DYN_ROOT);
 if (PiSq->ep) // en passant captures // TB_PiSq_score handles ep now
 {if (PiSq->wtm)
  {for (k=0;k<PiSq->n;k++) if (PiSq->sq[k]==(PiSq->ep-8)) break;
   A=EPtableW[FILE(PiSq->sq[k])]&wBitboardP;
   while (A)
   {CLONE_PISQ(PiSq2,PiSq); cp=BSF(A); A&=(A-1); PiSq2->ep=0; PiSq2->wtm^=1;
    for (c=0;c<PiSq->n;c++) if (PiSq->sq[c]==cp && PiSq->pi[c]==wEnumP) break;
    PiSq2->sq[c]=PiSq->sq[k]+8; PiSq2->n--; mv=(cp<<6)|(PiSq2->sq[c]);
    PiSq2->pi[k]=PiSq2->pi[PiSq2->n]; PiSq2->sq[k]=PiSq2->sq[PiSq2->n];
    b=TB_PiSq_score(PiSq2,&v); if (print)
    {if (!b) sprintf(OUTS[num++],"%s NoResultKnown -255 0\n",NOTATE(mv,STR));
     else if (v!=dWIN) sprintf(OUTS[num++],"%s %s %d --\n",
			      NOTATE(mv,STR),move_str(v,TRUE),v);
     else {b=do_move_results(PiSq2,FALSE,&v,OP);
           con2=(v>128); if (con2) v-=128;
           if (!b) sprintf(OUTS[num++],"%s 1 ? --\n",NOTATE(mv,STR));
	   else sprintf(OUTS[num++],
			"%s [%sLoss.In.%d%s] %d --\n",NOTATE(mv,STR),
		       con2?"[":".",v-3+!con2,con2?"]":".",-(v+!con2));}}
    else {if (!b) retval=FALSE; if (v>dDRAW) {if (!bc) *bv=v; bc=TRUE;}
          if (v>dDRAW && v<*bv) *bv=v;}}}
  else // black-to-move, with ep
  {for (k=0;k<PiSq->n;k++) if (PiSq->sq[k]==(PiSq->ep+8)) break;
   A=EPtableB[FILE(PiSq->sq[k])]&bBitboardP;
   while (A)
   {CLONE_PISQ(PiSq2,PiSq); cp=BSF(A); A&=(A-1); PiSq2->ep=0; PiSq2->wtm^=1;
    for (c=0;c<PiSq->n;c++) if (PiSq->sq[c]==cp && PiSq->pi[c]==bEnumP) break;
    PiSq2->sq[c]=PiSq->sq[k]-8; PiSq2->n--; mv=(cp<<6)|(PiSq2->sq[c]);
    PiSq2->pi[k]=PiSq2->pi[PiSq2->n]; PiSq2->sq[k]=PiSq2->sq[PiSq2->n];
    b=TB_PiSq_score(PiSq2,&v); if (print)
    {if (!b) sprintf(OUTS[num++],"%s NoResultKnown -255 0\n",NOTATE(mv,STR));
     else if (v!=dWIN) sprintf(OUTS[num++],"%s %s %d --\n",
			       NOTATE(mv,STR),move_str(v,TRUE),v);
     else {b=do_move_results(PiSq2,FALSE,&v,OP);
           con2=(v>128); if (con2) v-=128;
           if (!b) sprintf(OUTS[num++],"%s 1 ? --\n",NOTATE(mv,STR));
	   else sprintf(OUTS[num++],
			"%s [%sLoss.In.%d%s] %d --\n",NOTATE(mv,STR),
		       con2?"[":".",v-3+!con2,con2?"]":".",-(v+!con2));}}
    else {if (!b) retval=FALSE; if (v>dDRAW) {if (!bc) *bv=v; bc=TRUE;}
          if (v>dDRAW && v<*bv) *bv=v;}}}}
 if (!n && !PiSq->ep) {if (print) if (!OP) printf("No children\n");
                       else strcat(OP,"No children\n"); *bv=dLOST0;}
 for (i=0;i<n;i++) // loop over generated moves, main case, no ep
 {if (POSITION->sq[TO(ML[i])]) {u=MakePiSqCap(PiSq,ML[i]); conv=TRUE;}
  else if (PiSq->ep) continue; // if ep cap exists, ignore below
  else if (ML[i]>>12) {u=MakePiSqProm(PiSq,ML[i]); conv=TRUE;}
  else {MakePiSqPlain(PiSq,ML[i]); conv=FALSE;} // should set PiSq->ep properly
  b=TB_PiSq_score(PiSq,&v);// printf("Make %s %d %d\n",NOTATE(ML[i],STR),b,v);
  if (print)
  {if (!b) sprintf(OUTS[num++],"%s NoResultKnown -255 0\n",NOTATE(ML[i],STR));
   else if (v!=dWIN)
     sprintf(OUTS[num++],"%s %s %d --\n",NOTATE(ML[i],STR),move_str(v,conv),v);
   else {b=do_move_results(PiSq,FALSE,&v,OP);
         con2=(v>128); if (con2) v-=128;
         if (!b) sprintf(OUTS[num++],"%s 1 ? --\n",NOTATE(ML[i],STR));
	 else sprintf(OUTS[num++],
		      "%s %s%sLoss.In.%d%s%s %d --\n",NOTATE(ML[i],STR),
		     conv?"[":".",con2?"[":".",
		     v-3+!con2,con2?"]":".",conv?"]":".",-(v+!con2));}}
  else {if (!b) retval=FALSE; if (v>dDRAW && conv) {if (!bc) *bv=v; bc=TRUE;}
        if (v>dDRAW && v<*bv && (!bc || conv)) *bv=v;}
  if (POSITION->sq[TO(ML[i])]) UndoPiSqCap(PiSq,u);
  else if (ML[i]>>12) UndoPiSqProm(PiSq,ML[i]);
  else UndoPiSqPlain(PiSq,ML[i]); /*printf("Undo %s\n",NOTATE(ML[i],STR));*/}
 if (print) printf("Complete with moves\n"); if (bc) *bv+=128; return retval;}

void dump_OUTS(char *O)
{int i,j,s,v,tmp; int VAL[128]; char TMPSTR[32];
 for (i=0;i<num;i++)
 {s=0; while (OUTS[i][s]!=' ') s++; while (OUTS[i][s]==' ') s++;
  while (OUTS[i][s]!=' ') s++; while (OUTS[i][s]==' ') s++;
  v=atoi(OUTS[i]+s); if (v>=3 && v<4000) v=4000-v;
  if (v>0 && strstr(OUTS[i],"[[")) v+=1000;
  if (v<0 && !strstr(OUTS[i],"[[")) v-=1000;
  if (v>0 && strstr(OUTS[i],"[")) v+=1000;
  if (v<0 && !strstr(OUTS[i],"[")) v-=1000; VAL[i]=(v>0)?v:-4096-v;}
 //for (i=0;i<num;i++) printf("%d %s",VAL[i],OUTS[i]); printf("===========\n");
 for (i=num-1;i>=0;i--)
 {tmp=VAL[i]; strcpy(TMPSTR,OUTS[i]);
  for (j=i+1;j<num;j++)
  {if (VAL[j]<=tmp) break; VAL[j-1]=VAL[j]; strcpy(OUTS[j-1],OUTS[j]);}
  VAL[j-1]=tmp; strcpy(OUTS[j-1],TMPSTR);}
 if (!O) for (i=0;i<num;i++) printf("%s",OUTS[i]);
 else for (i=0;i<num;i++) strcat(O,OUTS[i]);}

void TB_output(typePOS *POS,char *OP) // losing.h and LCTB.h are incompatible
{int i,n=0,sq,pi; uint64 O; type_PiSq PiSq[1]; uint8 bv;
 PiSq->ep=POS->DYN->ep; for (i=0;i<6;i++) PiSq->sq[i]=PiSq->pi[i]=0; num=0;
 PiSq->wtm=POSITION->wtm; O=wBitboardOcc|bBitboardOcc; PiSq->Occupied=O;
 PiSq->n=0; PiSq->pawn=FALSE; PiSq->Blocked=0;
 while (O)
 {sq=BSF(O); O&=(O-1); pi=POSITION->sq[sq]; PiSq->pi[n]=pi; PiSq->sq[n++]=sq;
  if (pi==wEnumP || pi==bEnumP) PiSq->pawn=TRUE;}
 PiSq->n=n; do_move_results(PiSq,TRUE,&bv,OP); dump_OUTS(OP);}
