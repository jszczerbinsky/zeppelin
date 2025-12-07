#include "main.h"

#define IN_IDXW(c, sqr, p) (PIECE_MAX * 64 * (c) + PIECE_MAX * (sqr) + p)
#define IN_IDXB(c, sqr, p) (IN_IDXW(!(c), 63 - (sqr), p))

extern const unsigned char _binary_weights_bin_start[];

void nnue_add(NNUE *nnue, int idx) {
  for (int i = 0; i < NNUE_H1_SIZE; i++) {
    //    nnue->h1[i] += nnue->inw[idx]; // TODO weights biases
  }
}

void nnue_load_in(NNUE *nnue) {
  nnue->out = 0;
  memset(nnue->h1, 0, NNUE_H1_SIZE * sizeof(float));
  memset(nnue->h2, 0, NNUE_H2_SIZE * sizeof(float));

  for (int sqr = 0; sqr < 64; sqr++) {
    for (int p = 0; p < PIECE_MAX; p++) {
      int isw = (g_game.pieces[WHITE][p] & sqr2bbrd(sqr)) == sqr2bbrd(sqr);
      int isb = (g_game.pieces[BLACK][p] & sqr2bbrd(sqr)) == sqr2bbrd(sqr);

      if (isw) {
        nnue->in[WHITE][IN_IDXW(WHITE, sqr, p)] = 1;
        nnue->in[BLACK][IN_IDXB(WHITE, sqr, p)] = 1;
        nnue->in[WHITE][IN_IDXW(BLACK, sqr, p)] = 0;
        nnue->in[BLACK][IN_IDXB(BLACK, sqr, p)] = 0;

      } else if (isb) {
        nnue->in[WHITE][IN_IDXW(BLACK, sqr, p)] = 1;
        nnue->in[BLACK][IN_IDXB(BLACK, sqr, p)] = 1;
        nnue->in[WHITE][IN_IDXW(WHITE, sqr, p)] = 0;
        nnue->in[BLACK][IN_IDXB(WHITE, sqr, p)] = 0;

      } else {
        nnue->in[WHITE][IN_IDXW(WHITE, sqr, p)] = 0;
        nnue->in[BLACK][IN_IDXB(WHITE, sqr, p)] = 0;

        nnue->in[WHITE][IN_IDXW(BLACK, sqr, p)] = 0;
        nnue->in[BLACK][IN_IDXB(BLACK, sqr, p)] = 0;
      }

      /*if (val) {
        nnue_add(nnue, IN_IDX(c, sqr, p));
      }*/
    }
  }
}
