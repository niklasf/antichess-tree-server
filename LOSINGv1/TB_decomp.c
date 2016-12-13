
#include <stdio.h>
#include <stdlib.h>

#define UINT64 unsigned long long int

typedef struct
{
  int byte;
  int bit;
  unsigned char* data;
} bitstring;

static void pila_fare
  (unsigned int* PILA, unsigned int* l, unsigned int* va, UINT64* b, int ns)
{
  int c, HOP = 0, SALTO_DUE, prSALTO;
  unsigned char PREFISSO[256], PREFISSO2[256];
  unsigned int SA[256];
  unsigned int j, k, p, j2, i, pr;
  for (c = 0; c < 256; c++)
    PILA[c] = (1 << 24);
  for (c = 0; c < 256; c++)
    PREFISSO[c] = 0;
  for (c = 0; c < ns; c++)
    {
      if (l[c] <= 8)
	for (k = b[c]; k < 256; k += 1 << l[c])
	  PILA[k] = va[c] | (l[c] << 24);
      else
	PREFISSO[b[c] & 255] = 1;
    }
  for (p = 0; p < 256; p++)
    if (PREFISSO[p])
      {
	SA[HOP] = p;
	PILA[p] = HOP++;
      }
  SALTO_DUE = HOP;
  for (j = 0; j < HOP; j++)
    {
      pr = SA[j];
      for (i = 0; i < 256; i++)
	PREFISSO2[i] = 0;
      prSALTO = SALTO_DUE;
      for (c = 0; c < ns; c++)
	if (pr == (b[c] & 255))
	  {
	    if (l[c] <= 16)
	      for (k = (b[c] >> 8); k < 256; k += (1 << (l[c] - 8)))
		PILA[256 * (j + 1) + k] = va[c] | (l[c] << 24);
	    else
	      PREFISSO2[(b[c] >> 8) & 255] = 1;
	  }
      for (p = 0; p < 256; p++)
	if (PREFISSO2[p])
	  {
	    SA[SALTO_DUE] = p;
	    PILA[256 * (j + 1) + p] = SALTO_DUE++;
	  }
      for (j2 = prSALTO; j2 < SALTO_DUE; j2++)
	{
	  p = SA[j2];
	  for (c = 0; c < ns; c++)
	    if (p == ((b[c] >> 8) & 255) && pr == (b[c] & 255))
	      for (k = (b[c] >> 16); k < 256; k += (1 << (l[c] - 16)))
		PILA[256 * (j2 + 1) + k] = va[c] | (l[c] << 24);
	}
    }
}

static UINT64 bits_fare (int n, bitstring* BS)
{
  UINT64 r = 0;
  int l = n + BS->bit, bc = 0;
  while (l >= 8)
    {
      r |= (BS->data[BS->byte++] >> BS->bit) << bc;
      bc += (8 - BS->bit);
      BS->bit = 0;
      l -= 8;
    }
  if (l)
    r |= ((BS->data[BS->byte] >> BS->bit) & ((1 << (l - BS->bit)) - 1)) << bc;
  BS->bit = l;
  return r;
}

static void bytes_fare
  (unsigned char* DATA, unsigned int* PILA, int pro, unsigned char* B)
{
  int b = 0, vb = 0, d = 0;
  unsigned int va = 0, t;
  while (b < pro)
    {
      if (vb < 8)
	{
	  va |= (((unsigned int) DATA[d++]) << vb);
	  vb += 8;
	}
      if (vb < 16)
	{
	  va |= (((unsigned int) DATA[d++]) << vb);
	  vb += 8;
	}
      if (vb < 24)
	{
	  va |= (((unsigned int) DATA[d++]) << vb);
	  vb += 8;
	}
      t = PILA[va & 255];
      if (t >= (1 << 24))
	{
	  B[b++] = t & 255;
	  va >>= t >> 24;
	  vb -= t >> 24;
	  continue;
	}
      t = PILA[256 * (t + 1) + ((va >> 8) & 255)];
      if (t >= (1 << 24))
	{
	  B[b++] = t & 255;
	  va >>= t >> 24;
	  vb -= t >> 24;
	  continue;
	}
      t = PILA[256 * (t + 1) + ((va >> 16) & 255)];
      B[b++] = t & 255;
      va >>= t >> 24;
      vb -= t >> 24;
    }
}

static void bwt_annul (unsigned char* I, int pro, unsigned char* O, int ind)
{
  int B[257], *L, i, j, w = 0, *b;
  unsigned char *S;
  L = malloc (sizeof (int) * pro);
  S = malloc (pro);
  b = B + 1;
  for (i = 0; i <= 256; i++)
    B[i] = 0;
  for (i = 0; i < pro; i++)
    b[I[i]]++;
  for (i = 0; i < 256; i++)
    for (j = 0; j < b[i]; j++)
      S[w++] = i;
  for (i = 1; i < 256; i++)
    b[i] += b[i - 1];
  for (i = 0; i < pro; i++)
    L[b[I[i] - 1]++] = i;
  for (i = 0; i < pro; i++)
    {
      O[i] = S[ind];
      ind = L[ind];
    }
  free (L);
  free (S);
}

static unsigned int huffman_annul (unsigned char* I, unsigned char* O)
{
  long w = 0; int i, ns; bitstring BS[1];
  UINT64 pro = 0, bits[256]; unsigned int va[256], lun[256], *PILA;
  for (w = 0; w < 4; w++) pro += I[w] << (8 * w); /* size */
  ns = I[w++]; /* symbols */
  w++; w++;
  for (i = 0; i < ns; i++)
    {
      va[i] = I[w++]; /* value */
      lun[i] = I[w++]; /* bit length */
    }
  for (i = 0; i < ns; i++)
    if (lun[i] > 24)
      {
	printf ("huffman_annul lun %d %d\n", i, lun[i]);
	exit (1);
      }
  BS->data = (I + w);
  BS->bit = 0;
  BS->byte = 0;
  for (i = 0; i < ns; i++)
    bits[i] = bits_fare (lun[i], BS); /* symbol bits */

  PILA = malloc (65536 * sizeof (int)); /* HACK */
  if (BS->bit)
    BS->byte++;
  w += BS->byte;
  pila_fare (PILA, lun, va, bits, ns); /* build table */
  BS->data = (I + w);
  BS->bit = 0;
  BS->byte = 0;
  bytes_fare (BS->data, PILA, pro, O); /* data output */
  free (PILA);
  return pro;
}

#define SEG1 0xFB
#define SEG2 0xFC
static unsigned int rle_annul (unsigned char* I, int pro, unsigned char* O)
{
  int i = 0, o = 0, s = 0, rip = 1, b, j;
  while (i < pro)
    {
      b = I[i++];
      if (b == SEG1)
	rip += (1 << s++);
      else if (b == SEG2)
	rip += (2 << s++);
      else
	{
	  for (j = 0; j < rip; j++)
	    O[o++] = b;
	  rip = 1;
	  s = 0;
	}
    }
  return o;
}

#define RIP1 0xFD
#define RIP2 0xFE
static unsigned int mtf_annul (unsigned char* I, int pro, unsigned char* O)
{
  int b, j, w, i = 0, o = 0, c, rip = 0, s = 0;
  unsigned char sim[256];
  c = I[i++];
  for (j = 0; j < c; j++)
    sim[j] = I[i++];
  while (i < pro)
    {
      b = I[i++];
      if (b == RIP1)
	rip += (1 << s++);
      else if (b == RIP2)
	rip += (2 << s++);
      else
	{
	  w = sim[b];
	  for (j = 0; j < rip; j++)
	    O[o++] = sim[0];
	  rip = 1;
	  s = 0;
	  for (; b > 0; b--)
	    sim[b] = sim[b - 1];
	  sim[0] = w;
	}
    }
  if (rip)
    for (j = 0; j < rip; j++)
      O[o++] = sim[0];
  return o;
}

#include <string.h>

int DecompressBlock (unsigned char* O, unsigned char* I, int lun)
{ int i = 0, f = 0, b; unsigned char *B; B = malloc (65536);
  //  for (i=0;i<lun;i++) printf("%d ",I[i]); printf("\n");
  for (i = 0; i < 4; i++) f |= I[i] << (8 * i);
  b = huffman_annul (I + i, B); b = mtf_annul (B, b, O);
  bwt_annul (O, b, B, f); b = rle_annul (B, b, O); free (B); return b;}
