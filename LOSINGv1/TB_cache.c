
#include "LCTB.h"

#define BLOCK_SIZE 0x10000
static uint8 ***TB_cache = NULL;
uint8 *BLOCK_PTR; /* SMP ? */ /* need 1MB+128 for hiperindici */
static volatile uint64 *CACHE_INFO[4]; /* volatile? */
static uint64 TB_CACHE_COUNT = 0;
//static MUTEX_TYPE CACHE_LOCK[1];
static int CURRENT_TOTAL_BASE_CACHE=0;
#define MAXIMAL_FOPEN 64

int SetTBcache (int mb)
{
  int i, j; mb = 1 << BSR (mb); TB_CACHE_COUNT = mb << 2;
  for (i = 0; i < 4; i++) if (CACHE_INFO[i]) free ((void*) CACHE_INFO[i]);
  for (i = 0; i < 4; i++) if (TB_cache[i])
      {
	for (j = 0; j < CURRENT_TOTAL_BASE_CACHE << 2; j++)
	  if (TB_cache[i][j]) free (TB_cache[i][j]);
	free (TB_cache[i]);
      }
  CACHE_INFO[0] = malloc (TB_CACHE_COUNT * sizeof (uint64));
  CACHE_INFO[1] = malloc (TB_CACHE_COUNT * sizeof (uint64));
  CACHE_INFO[2] = malloc (TB_CACHE_COUNT * sizeof (uint64));
  CACHE_INFO[3] = malloc (TB_CACHE_COUNT * sizeof (uint64));
  TB_cache[0] = malloc (TB_CACHE_COUNT * sizeof (uint8 *));
  TB_cache[1] = malloc (TB_CACHE_COUNT * sizeof (uint8 *));
  TB_cache[2] = malloc (TB_CACHE_COUNT * sizeof (uint8 *));
  TB_cache[3] = malloc (TB_CACHE_COUNT * sizeof (uint8 *));
  for (j = 0; j < 4; j++)
    for (i = 0; i < TB_CACHE_COUNT; i++) TB_cache[j][i] = NULL;
  for (j = 0; j < 4; j++)
    for (i = 0; i < TB_CACHE_COUNT; i++) CACHE_INFO[j][i] = 0xffffffff;
  printf ("TB_Cache is %dmb + (1mb)\n", mb);
  CURRENT_TOTAL_BASE_CACHE = mb;
  return mb;
}

void InitInitTotalBaseCache (uint64 mb)
{ int i; // LOCK_INIT (CACHE_LOCK);
  TB_cache = malloc (4 * sizeof (uint8 **));
  BLOCK_PTR = malloc (128 + 16 * BLOCK_SIZE); /* for hiperindici */
  for (i = 0; i < 4; i++) { CACHE_INFO[i] = NULL; TB_cache[i] = NULL; }
  SetTBcache (mb);}

#define GetCache0(a, b) ( (123 * (a) + 321 * (b) ) & ( TB_CACHE_COUNT - 1) )
#define GetCache1(a, b) ( (179 * (a) + 557 * (b) ) & ( TB_CACHE_COUNT - 1) )
#define GetCache2(a, b) ( (671 * (a) + 409 * (b) ) & ( TB_CACHE_COUNT - 1) )
#define GetCache3(a, b) ( (773 * (a) + 187 * (b) ) & ( TB_CACHE_COUNT - 1) )
#define GetCache(x ,a, b) \
  ((x) >= 2) ? (((x) == 2) ? GetCache2 (a, b) : GetCache3 (a, b)) \
             : (((x) == 1) ? GetCache1 (a, b) : GetCache0 (a, b))
static uint32 espul_conto = 0;
static int fc = 0;
static volatile uint64 CURRENT_XOR = 0;

static uint8 CachePosition (TableBase *TB, uint64 index)
{uint64 n = TB->num; uint64 zo = index / BLOCK_SIZE, ci = (n << 32) | zo;
 int h[4], e, i; uint8* BPTR; uint8 u; uint64 SAVE_XOR;
 PRIM:
  SAVE_XOR = CURRENT_XOR;
  for (i = 0; i < 4; i++)
    {
      h[i] = GetCache (i, n, zo);
      if (CACHE_INFO[i][h[i]] == ci)
	{
	  u = TB_cache[i][h[i]][index % BLOCK_SIZE];
	  if (CACHE_INFO[i][h[i]] == ci) return u;
	}
    }
  //  LOCK (CACHE_LOCK);
  if (SAVE_XOR != CURRENT_XOR) { /* UNLOCK (SCOP_PILA_LOCK); */ goto PRIM; }
  espul_conto++; e = -1;
  for (i = 0; i < 4; i++) if (CACHE_INFO[i][h[i]] == 0xffffffff) e = i;
  if (e == -1)
    { e = 0; for (i = 0; i < 16; i++) e += (espul_conto >> i); e &= 3; }
  if (!TB_cache[e][h[e]]) TB_cache[e][h[e]] = malloc (BLOCK_SIZE);
  // this can crash, so there's a memory foobar somewhere... (spinloop?)
  CACHE_INFO[e][h[e]] = 0xfffeefff; /* invalidate */
  {
    uint32 ba = TB->indici[zo];
    sint32 lun = TB->indici[zo + 1] - ba; /* 32-bit plus under Windows ? */
    uint64 pr = ba;
    char STR[1024];
    if (!TB->Fdata)
      { fc++;
	if (fc == MAXIMAL_FOPEN) /* FULL */
	  {
	    for (i = 0; i < 0x4000; i++) if ((TB_TABLE + i)->Fdata)
		{fclose ((TB_TABLE + i)->Fdata); (TB_TABLE + i)->Fdata = NULL;}
	    fc = 1;
	  }
	sprintf(STR, "%s%s%s", TB->DIR_NOME, COMPRESSION_PREFIX+1, TB->string);
	TB->Fdata = fopen (STR, "rb");
	//	printf("open %s %s %s index:%lld pr:%lld lun:%d handle:%d\n",
	//     STR,TB->DIR_NOME,TB->string,index,pr,lun,TB->Fdata);
      }
      fseek (TB->Fdata, pr, SEEK_SET);
      fread (BLOCK_PTR, 1, lun, TB->Fdata);
      BPTR = BLOCK_PTR;
      DecompressBlock (TB_cache[e][h[e]], BPTR, lun);
    }
  CACHE_INFO[e][h[e]] = ci; /* ritard a tiempo */
  CURRENT_XOR ^= ci | (((uint64) h[e]) << 44) | (((uint64) (e)) << 62);
  u = TB_cache[e][h[e]][index % BLOCK_SIZE]; return u;}

uint8 TB_value (TableBase* tb, uint64 ind) /* SMP the this */
{ uint8 u; u = CachePosition (tb, ind); return u; }
