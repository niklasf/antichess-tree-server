
#define NEED_TB_ARRAYS
#include "LCTB.h"

// version for parse_win

static void try_register(type_PiSq *PiSq)
{char s[8]; int i; char C[16]="\0PNBRQKZpnbrqk";
 for (i=0;i<6;i++) s[i]=C[PiSq->pi[i]]; s[6]=s[7]=0;
 canonical_name(s); // in place
 if (RegisterTB5(s,"LCTB/2/")) {printf("Registered %s\n",s); return;}
 if (RegisterTB5(s,"LCTB/3/")) {printf("Registered %s\n",s); return;}
 if (RegisterTB5(s,"LCTB/4/")) {printf("Registered %s\n",s); return;}
 if (RegisterTB5(s,"LCTB/5/")) {printf("Registered %s\n",s); return;}
 if (RegisterTB5(s,"LCTB/41/")) {printf("Registered %s\n",s); return;}
 if (RegisterTB5(s,"LCTB/32/")) {printf("Registered %s\n",s); return;}
 if (RegisterTB5(s,"LCTB/6/")) {printf("Registered %s\n",s); return;}
 printf("FAILED to register %s\n",s);}

static uint32 get_rep5(uint32 v,uint32 w,uint32 x,uint32 y,uint32 z)
{uint32 a,b,c,d,e; a=v; if (w>a) a=w; if (x>a) a=x; if (y>a) a=y; if (z>a) a=z;
 if (a==v) v=0; if (a==w) w=0; if (a==x) x=0; if (a==y) y=0; if (a==z) z=0;
 b=v; if (w>b) b=w; if (x>b) b=x; if (y>b) b=y; if (z>b) b=z;
 if (b==v) v=0; if (b==w) w=0; if (b==x) x=0; if (b==y) y=0; if (b==z) z=0;
 c=v; if (w>c) c=w; if (x>c) c=x; if (y>c) c=y; if (z>c) c=z;
 if (c==v) v=0; if (c==w) w=0; if (c==x) x=0; if (c==y) y=0; if (c==z) z=0;
 d=v; if (w>d) d=w; if (x>d) d=x; if (y>d) d=y; if (z>d) d=z;
 if (d==v) v=0; if (d==w) w=0; if (d==x) x=0; if (d==y) y=0; if (d==z) z=0;
 e=v; if (w>e) e=w; if (x>e) e=x; if (y>e) e=y; if (z>e) e=z;
 if (e==v) v=0; if (e==w) w=0; if (e==x) x=0; if (e==y) y=0; if (e==z) z=0;
 return BINOMIAL(a,5)+BINOMIAL(b,4)+BINOMIAL(c,3)+BINOMIAL(d,2)+BINOMIAL(e,1);}

boolean get_index(type_PiSq *PiSq,int *tb_num,uint64 *ind,uint64* reflect)
{int u,w=0,i,x; uint32 v; uint32 Pi[6],Sq[6],SR[6]; uint64 z; TableBase *TB;
 for (i=PiSq->n;i<6;i++) {PiSq->pi[i]=0; PiSq->sq[i]=0;}
 if (reflect) *reflect=-1; // DUMP_PISQ(PiSq);
 if (!PiSq->wtm)
 {for (i=0;i<PiSq->n;i++) PiSq->pi[i]=color_switch[PiSq->pi[i]];}
 v=TB_LOOKUP[PiSq->pi[0]][PiSq->pi[1]][PiSq->pi[2]]
            [PiSq->pi[3]][PiSq->pi[4]][PiSq->pi[5]];
#if 1 /* register on the fly */
 if (v==0xffffffff) {try_register(PiSq); fflush(stdout);}
 v=TB_LOOKUP[PiSq->pi[0]][PiSq->pi[1]][PiSq->pi[2]]
            [PiSq->pi[3]][PiSq->pi[4]][PiSq->pi[5]];
#endif
 if (!PiSq->wtm)
 {for (i=0;i<PiSq->n;i++) PiSq->pi[i]=color_switch[PiSq->pi[i]];}
 if (v==0xffffffff) return FALSE;
 *tb_num=v&0x3fff; TB=TB_TABLE+*tb_num;
 for (i=0;i<PiSq->n;i++)
 {x=(v>>(14+3*i))&7; Sq[i]=PiSq->sq[x]; Pi[i]=PiSq->pi[x];}
 if (NOW_DEBUG) printf("HI1 %d %d %d %d %d %d %d %d\n",
		       Pi[0],Sq[0],Pi[1],Sq[1],Pi[2],Sq[2],Pi[3],Sq[3]);
 if (TB->rep[0]==1)
 {if (PiSq->pawn)
  {w=2*(File(Sq[0])>=FE)+(!PiSq->wtm);
   for (i=0;i<PiSq->n;i++) Sq[i]=GRP_ACT[w][Sq[i]];} // TODO Blocked adjust
  else // get KEY
  {if (TB->rep[0]==1) w=KEY_MAP[Sq[0]];
   for (i=0;i<PiSq->n;i++) Sq[i]=GRP_ACT[w][Sq[i]];}}
 if (TB->rep[0]==2 && PiSq->pawn && !PiSq->wtm)
 {for (i=0;i<PiSq->n;i++) Sq[i]=GRP_ACT[1][Sq[i]];} // TODO Blocked adjust
 if (NOW_DEBUG) printf("HI2(%d) %d %d %d %d %d %d %d %d\n",
		       w,Pi[0],Sq[0],Pi[1],Sq[1],Pi[2],Sq[2],Pi[3],Sq[3]);
 for (i=0;i<PiSq->n;i++) if (!Pi[i]) Sq[i]=0;
 for (i=0;i<PiSq->n;i++)
  if (Pi[i]==wEnumP || Pi[i]==bEnumP || Pi[i]==BlockedPawn) Sq[i]-=8; // here??
 if (NOW_DEBUG) {for (u=0;u<PiSq->n;u++) printf("%d ",Sq[u]); DUMP_PISQ(PiSq);}
 if (!PiSq->pawn && TB->rep[0]==1)
  for (u=0;u<PiSq->n;u++) SR[u]=GRP_ACT[4][Sq[u]];
 if (TB->rep[0]==1) // easy
 {if (PiSq->pawn)
  {if (Pi[0]==wEnumP || Pi[0]==bEnumP || Pi[0]==BlockedPawn)
      Sq[0]=MAP24[Sq[0]]; else Sq[0]=MAP32[Sq[0]];}
  else Sq[0]=MAP10[Sq[0]];
  if (!PiSq->pawn) SR[0]=MAP10[SR[0]];}
 for (u=(TB->rep[0]==1)?1:0;u<PiSq->n;u++)
 {if (TB->rep[u]==1) continue;
  if (TB->rep[u]==2) {Sq[u]=REP2[Sq[u]][Sq[u+1]]; Sq[u+1]=0; u++; continue;}
  if (TB->rep[u]==3)
  {Sq[u]=REP3[Sq[u]][Sq[u+1]][Sq[u+2]]; Sq[u+1]=Sq[u+2]=0; u++; u++; continue;}
  if (TB->rep[u]==4)
  {Sq[u]=REP4[Sq[u]][Sq[u+1]][Sq[u+2]][Sq[u+3]]; Sq[u+1]=Sq[u+2]=Sq[u+3]=0;
   u++; u++; u++; continue;}
  if (TB->rep[u]==5)
  {Sq[u]=get_rep5(Sq[u],Sq[u+1],Sq[u+2],Sq[u+3],Sq[u+4]);
   Sq[u+1]=Sq[u+2]=Sq[u+3]=Sq[u+4]=0; u+=4; continue;}}
 if (NOW_DEBUG) {for (u=0;u<PiSq->n;u++) printf("%d ",Sq[u]); DUMP_PISQ(PiSq);}
 z=0; for (i=0;i<PiSq->n;i++) z=z*TB->m[i]+Sq[i]; *ind=z; PiSq->index=z;
 if (z>=TB->size) {printf("z %lld ",z); DUMP_PISQ(PiSq); raise(SIGTRAP);}
 if (!reflect || PiSq->pawn || TB->rep[0]!=1 || Sq[0]!=SR[0]) return TRUE;
 for (u=1;u<PiSq->n;u++)
 {if (TB->rep[u]==1) continue;
  if (TB->rep[u]==2) {SR[u]=REP2[SR[u]][SR[u+1]]; SR[u+1]=0; u++; continue;}
  if (TB->rep[u]==3)
  {SR[u]=REP3[SR[u]][SR[u+1]][SR[u+2]]; SR[u+1]=SR[u+2]=0; u++; u++; continue;}
  if (TB->rep[u]==4)
  {SR[u]=REP4[SR[u]][SR[u+1]][SR[u+2]][SR[u+3]]; SR[u+1]=SR[u+2]=SR[u+3]=0;
   u++; u++; u++; continue;}
  if (TB->rep[u]==5)
  {SR[u]=get_rep5(SR[u],SR[u+1],SR[u+2],SR[u+3],SR[u+4]);
   SR[u+1]=SR[u+2]=SR[u+3]=SR[u+4]=0; u+=4; continue;}}
 z=0; for (i=0;i<PiSq->n;i++) z=z*TB->m[i]+SR[i]; *reflect=z;
 PiSq->reflect=z; return TRUE;
}

boolean TB_PiSq_score(type_PiSq *PiSq,uint8 *v)
{int tb_num; uint64 ind; int i,w=0,b=0; TableBase *TB;
 PiSq->index=0;
 for (i=0;i<PiSq->n;i++) if (PiSq->pi[i]<7) w++;
 for (i=0;i<PiSq->n;i++) if (PiSq->pi[i]>7) b++;
 if (w==0 && PiSq->wtm) {*v=dWIN; return TRUE;}
 if (b==0 && !PiSq->wtm) {*v=dWIN; return TRUE;}
 if (!get_index(PiSq,&tb_num,&ind,NULL)) return FALSE;
 TB=TB_TABLE+tb_num;
 // if (PiSq->n<=4) {*v=((TB->FLAT[ind>>2])>>(2*(ind&3)))&3; return ((*v)!=0);}
 *v=TB_value(TB_TABLE+tb_num,ind); return TRUE;}

#ifdef HAS_POPCNT
static inline int POPCNT (uint64 w)
{
  uint64 x;
  asm ("popcntq %1,%0\n": "=&r" (x):"r" (w));
  return x;
}
#else
static inline int POPCNT (uint64 w)
{
  w = w - ((w >> 1) & 0x5555555555555555ull);
  w = (w & 0x3333333333333333ull) + ((w >> 2) & 0x3333333333333333ull);
  w = (w + (w >> 4)) & 0x0f0f0f0f0f0f0f0full;
  return (w * 0x0101010101010101ull) >> 56;
}
#endif /* HAS_POPCNT */

boolean Get_TB_Score(typePOS *POS,uint8 *va,boolean ONLY_FLAT)
{int i,n=0; type_PiSq PiSq[1]; int pi,sq; uint64 O; if (!TB_INIT) return FALSE;
 if (POS->DYN->ep) return FALSE; for (i=0;i<6;i++) PiSq->sq[i]=PiSq->pi[i]=0;
 PiSq->wtm=POS->wtm; O=wBitboardOcc|bBitboardOcc; PiSq->Occupied=O;
 // if (POPCNT(O)>4 &&  ONLY_FLAT) return FALSE; if (POPCNT(O)>5) return FALSE;
 PiSq->n=0; PiSq->pawn=FALSE; PiSq->Blocked=0;
 while (O)
 {sq=BSF(O); O&=(O-1); pi=POSITION->sq[sq]; PiSq->pi[n]=pi; PiSq->sq[n++]=sq;
  if (pi==wEnumP || pi==bEnumP) PiSq->pawn=TRUE;}
 PiSq->n=n; return TB_PiSq_score(PiSq,va);}
