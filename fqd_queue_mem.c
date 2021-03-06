/*
 * Copyright (c) 2013 OmniTI Computer Consulting, Inc.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "fqd.h"
#include <stdlib.h>
#include <ck_fifo.h>

struct queue_mem {
  uint32_t              qlen;
  ck_fifo_mpmc_t        q;
  ck_fifo_mpmc_entry_t *qhead;
};

static void queue_mem_enqueue(fqd_queue_impl_data f, fq_msg *m) {
  struct queue_mem *d = (struct queue_mem *)f;
  ck_fifo_mpmc_entry_t *fifo_entry;
  fifo_entry = malloc(sizeof(ck_fifo_mpmc_entry_t));
  fq_msg_ref(m);
  ck_fifo_mpmc_enqueue(&d->q, fifo_entry, m);
  ck_pr_inc_uint(&d->qlen);
}
static fq_msg *queue_mem_dequeue(fqd_queue_impl_data f) {
  struct queue_mem *d = (struct queue_mem *)f;
  ck_fifo_mpmc_entry_t *garbage;
  fq_msg *m;
  if(ck_fifo_mpmc_dequeue(&d->q, &m, &garbage) == true) {
    ck_pr_dec_uint(&d->qlen);
    /* We can free this only because this fifo is used as a
     * multi-producer and *single* consumer */
    if(garbage != d->qhead) free(garbage);
    return m;
  }
  return NULL;
}
static fqd_queue_impl_data queue_mem_setup(fq_rk *qname, uint32_t *count) {
  struct queue_mem *d;
  d = calloc(1, sizeof(*d));
  d->qhead = malloc(sizeof(ck_fifo_mpmc_entry_t));
  *count = 0;
  ck_fifo_mpmc_init(&d->q, d->qhead);
  (void)qname;
  return d;
}
static void queue_mem_dispose(fq_rk *qname, fqd_queue_impl_data f) {
  struct queue_mem *d = (struct queue_mem *)f;
  fq_msg *m;
  (void)qname;
  while(NULL != (m = queue_mem_dequeue(d))) {
    fq_msg_deref(m);
  }
  if(d->qhead) free(d->qhead);
  free(d);
}

fqd_queue_impl fqd_queue_mem_impl = {
  .name = "mem",
  .setup = queue_mem_setup,
  .enqueue = queue_mem_enqueue,
  .dequeue = queue_mem_dequeue,
  .dispose = queue_mem_dispose
};
