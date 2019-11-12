#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "float.h"
#include "functor.h"
#include "macro.h"
#include "rdtsc.h"

#define DATA_COUNT 8

size_t
test(size_t a, size_t b, size_t c)
{
  getenv("USERFOO");

  return 0;
}

void
clear_parameters(Functor_t *fp)
{
  memset(fp->param, 0, sizeof(fp->param));
}

void
push_parameters(size_t offset, const char *type_info, Param_t *next_param)
{
  static char *example_p[] = {
    "RESULT1", "RESULT2", "RESULT3", "RESULT4",
    "RESULT5", "RESULT6", "RESULT7", "RESULT8",
  };
  static uint64_t example_i[] = {
    1231,      2323513,  35817,        472311,
    591203857, 62373363, 712375832891, 819239283,
  };
  static double example_d[] = {
    1.23412, 2.8123, 3.1415926, 4.5911, 5.5912, 6.1837, 7.8917822, 8.0111,
  };
  if (*type_info == 0)
    return;

  offset = MIN(DATA_COUNT - 1, offset);
  switch (*type_info) {
  case 'd':
    next_param->d = example_d[offset];
    break;
  case 'i':
    next_param->i = example_i[offset];
    break;
  case 'p':
    next_param->p = example_p[offset];
    break;
  }

  push_parameters(offset, type_info + 1, next_param + 1);
}

void
api_call(Functor_t f)
{
  functor_invoke(f);
}

void
manual_call(Functor_t f)
{
  f.call(f.param[0], f.param[1], f.param[2]);
}

static char global_result[8];
static Functor_t gf;
static size_t go;
void
clear_result()
{
  memset(global_result, 0, sizeof(global_result));
}
void
param_choose_n(const char *param_types, int n, char *result)
{
  if (!n) {
    *result = 0;
    printf("\nglobal_result %s\n", global_result);
    push_parameters(go, global_result, gf.param);
    functor_invoke(gf);
    return;
  }

  const char *read = param_types;
  while (*read) {
    *result = *read++;
    param_choose_n(param_types, n - 1, result + 1);
  }
}

void
check_count(int i)
{
  if (strlen(global_result) < i)
    printf("undercall\n");
  else if (strlen(global_result) > i)
    printf("overcall\n");
}

void
check_d(char c, ApiDouble_t d)
{
  if (c == 'd')
    printf("valid %f\n", d.value);
  else
    printf("mismatch d/%c: %zx\n", c, d._pdbr);
}

void
check_i(char c, int64_t i)
{
  if (c == 'i')
    printf("valid %zd\n", i);
  else
    printf("mismatch i/%c: %zx\n", c, i);
}

void
check_p(char c, const char *p)
{
  if (c == 'p')
    printf("valid \"%s\"\n", p);
  else
    printf("mismatch p/%c: %p\n", c, p);
}

#define CHECK(c, X) \
  _Generic((X), ApiDouble_t : check_d, int64_t : check_i, const char * : check_p)(c, X)

size_t
test_d(ApiDouble_t d)
{
  check_count(1);
  CHECK(global_result[0], d);

  return 1;
}

size_t
test_i(int64_t i)
{
  check_count(1);
  CHECK(global_result[0], i);

  return 1;
}

size_t
test_p(const char *p)
{
  check_count(1);
  CHECK(global_result[0], p);

  return 1;
}

size_t
test_dd(ApiDouble_t d, ApiDouble_t dd)
{
  check_count(2);

  CHECK(global_result[0], d);
  CHECK(global_result[1], dd);

  return 1;
}

size_t
test_di(ApiDouble_t d, int64_t ii)
{
  check_count(2);
  CHECK(global_result[0], d);
  CHECK(global_result[1], ii);

  return 1;
}

size_t
test_dp(ApiDouble_t d, const char *pp)
{
  check_count(2);
  CHECK(global_result[0], d);
  CHECK(global_result[1], pp);

  return 1;
}

size_t
test_id(int64_t i, ApiDouble_t dd)
{
  check_count(2);
  CHECK(global_result[0], i);
  CHECK(global_result[1], dd);

  return 1;
}

size_t
test_ii(int64_t i, int64_t ii)
{
  check_count(2);
  CHECK(global_result[0], i);
  CHECK(global_result[1], ii);

  return 1;
}

size_t
test_ip(int64_t i, const char *pp)
{
  check_count(2);
  CHECK(global_result[0], i);
  CHECK(global_result[1], pp);

  return 1;
}

size_t
test_pd(const char *p, ApiDouble_t dd)
{
  check_count(2);
  CHECK(global_result[0], p);
  CHECK(global_result[1], dd);

  return 1;
}

size_t
test_pi(const char *p, int64_t i)
{
  check_count(2);
  CHECK(global_result[0], p);
  CHECK(global_result[1], i);

  return 1;
}

size_t
test_pp(const char *p, const char *pp)
{
  check_count(2);
  CHECK(global_result[0], p);
  CHECK(global_result[1], pp);

  return 1;
}

size_t
test_ddd(ApiDouble_t d, ApiDouble_t dd, ApiDouble_t ddd)
{
  check_count(3);
  CHECK(global_result[0], d);
  CHECK(global_result[1], dd);
  CHECK(global_result[2], ddd);

  return 1;
}

size_t
test_ddi(ApiDouble_t d, ApiDouble_t dd, int64_t iii)
{
  check_count(3);
  CHECK(global_result[0], d);
  CHECK(global_result[1], dd);
  CHECK(global_result[2], iii);

  return 0;
}

size_t
test_ddp(ApiDouble_t d, ApiDouble_t dd, const char *ppp)
{
  check_count(3);
  CHECK(global_result[0], d);
  CHECK(global_result[1], dd);
  CHECK(global_result[2], ppp);

  return 1;
}

size_t
test_did(ApiDouble_t d, int64_t ii, ApiDouble_t ddd)
{
  check_count(3);
  CHECK(global_result[0], d);
  CHECK(global_result[1], ii);
  CHECK(global_result[2], ddd);

  return 1;
}

size_t
test_dii(ApiDouble_t d, int64_t ii, int64_t iii)
{
  check_count(3);
  CHECK(global_result[0], d);
  CHECK(global_result[1], ii);
  CHECK(global_result[2], iii);

  return 1;
}

size_t
test_dip(ApiDouble_t d, int64_t ii, const char *ppp)
{
  check_count(3);
  CHECK(global_result[0], d);
  CHECK(global_result[1], ii);
  CHECK(global_result[2], ppp);

  return 1;
}

size_t
test_dpd(ApiDouble_t d, const char *pp, ApiDouble_t ddd)
{
  check_count(3);

  CHECK(global_result[0], d);
  CHECK(global_result[1], pp);
  CHECK(global_result[2], ddd);

  return 1;
}

size_t
test_dpi(ApiDouble_t d, const char *pp, int64_t iii)
{
  check_count(3);
  CHECK(global_result[0], d);
  CHECK(global_result[1], pp);
  CHECK(global_result[2], iii);

  return 1;
}

size_t
test_dpp(ApiDouble_t d, const char *pp, const char *ppp)
{
  check_count(3);
  CHECK(global_result[0], d);
  CHECK(global_result[1], pp);
  CHECK(global_result[2], ppp);

  return 1;
}

size_t
test_idd(int64_t i, ApiDouble_t dd, ApiDouble_t ddd)
{
  check_count(3);
  CHECK(global_result[0], i);
  CHECK(global_result[1], dd);
  CHECK(global_result[2], ddd);

  return 1;
}

size_t
test_idi(int64_t i, ApiDouble_t dd, int64_t iii)
{
  check_count(3);
  CHECK(global_result[0], i);
  CHECK(global_result[1], dd);
  CHECK(global_result[2], iii);

  return 1;
}

size_t
test_idp(int64_t i, ApiDouble_t dd, const char *ppp)
{
  check_count(3);
  CHECK(global_result[0], i);
  CHECK(global_result[1], dd);
  CHECK(global_result[2], ppp);

  return 1;
}

size_t
test_iid(int64_t i, int64_t ii, ApiDouble_t ddd)
{
  check_count(3);
  CHECK(global_result[0], i);
  CHECK(global_result[1], ii);
  CHECK(global_result[2], ddd);

  return 1;
}

size_t
test_iii(int64_t i, int64_t ii, int64_t iii)
{
  check_count(3);
  CHECK(global_result[0], i);
  CHECK(global_result[1], ii);
  CHECK(global_result[2], iii);

  return 1;
}

size_t
test_iip(int64_t i, int64_t ii, const char *ppp)
{
  check_count(3);
  CHECK(global_result[0], i);
  CHECK(global_result[1], ii);
  CHECK(global_result[2], ppp);

  return 1;
}

size_t
test_ipd(int64_t i, const char *pp, ApiDouble_t ddd)
{
  check_count(3);
  CHECK(global_result[0], i);
  CHECK(global_result[1], pp);
  CHECK(global_result[2], ddd);

  return 1;
}

size_t
test_ipi(int64_t i, const char *pp, int64_t iii)
{
  check_count(3);
  CHECK(global_result[0], i);
  CHECK(global_result[1], pp);
  CHECK(global_result[2], iii);

  return 1;
}

size_t
test_ipp(int64_t i, const char *pp, const char *ppp)
{
  check_count(3);
  CHECK(global_result[0], i);
  CHECK(global_result[1], pp);
  CHECK(global_result[2], ppp);

  return 1;
}

size_t
test_pdd(const char *p, ApiDouble_t dd, ApiDouble_t ddd)
{
  check_count(3);
  CHECK(global_result[0], p);
  CHECK(global_result[1], dd);
  CHECK(global_result[2], ddd);

  return 1;
}

size_t
test_pdi(const char *p, ApiDouble_t dd, int64_t iii)
{
  check_count(3);
  CHECK(global_result[0], p);
  CHECK(global_result[1], dd);
  CHECK(global_result[2], iii);

  return 1;
}

size_t
test_pdp(const char *p, ApiDouble_t dd, const char *ppp)
{
  check_count(3);
  CHECK(global_result[0], p);
  CHECK(global_result[1], dd);
  CHECK(global_result[2], ppp);

  return 1;
}

size_t
test_pid(const char *p, int64_t ii, ApiDouble_t ddd)
{
  check_count(3);
  CHECK(global_result[0], p);
  CHECK(global_result[1], ii);
  CHECK(global_result[2], ddd);

  return 1;
}

size_t
test_pii(const char *p, int64_t ii, int64_t iii)
{
  check_count(3);
  CHECK(global_result[0], p);
  CHECK(global_result[1], ii);
  CHECK(global_result[2], iii);

  return 1;
}

size_t
test_pip(const char *p, int64_t ii, const char *ppp)
{
  check_count(3);
  CHECK(global_result[0], p);
  CHECK(global_result[1], ii);
  CHECK(global_result[2], ppp);

  return 1;
}

size_t
test_ppd(const char *p, const char *pp, ApiDouble_t ddd)
{
  check_count(3);
  CHECK(global_result[0], p);
  CHECK(global_result[1], pp);
  CHECK(global_result[2], ddd);

  return 1;
}

size_t
test_ppi(const char *p, const char *pp, int64_t iii)
{
  check_count(3);
  CHECK(global_result[0], p);
  CHECK(global_result[1], pp);
  CHECK(global_result[2], iii);

  return 1;
}

size_t
test_ppp(const char *p, const char *pp, const char *ppp)
{
  check_count(3);
  CHECK(global_result[0], p);
  CHECK(global_result[1], pp);
  CHECK(global_result[2], ppp);

  return 1;
}

int
main(int argc, char **argv)
{
  const unsigned OneMil = 1 * 1000 * 1000;
  uint64_t start = rdtsc();
  Functor_t f = functor_init(test);
  f.param[0].i = 35;
  f.param[2].i = 13;

  // asm review
  api_call(f);
  manual_call(f);

  // Tests
  for (int i = 0; i < OneMil; ++i) {
    functor_invoke(f);
  }
  uint64_t end1 = rdtsc();
  for (int i = 0; i < OneMil; ++i) {
    test(35, 0, 13);
  }
  uint64_t end2 = rdtsc();
  for (int i = 0; i < OneMil; ++i) {
    getenv("USERFOO");
  }
  uint64_t end3 = rdtsc();

  puts("Performance timing");
  printf("%lu functor %lu indirect %lu direct\n", end1 - start, end2 - end1,
         end3 - end2);

  FuncPointer call_address[] = {
    test_d,   test_i,   test_p,   test_dd,  test_di,  test_dp,  test_id,
    test_ii,  test_ip,  test_pd,  test_pi,  test_pp,  test_ddd, test_ddi,
    test_ddp, test_did, test_dii, test_dip, test_dpd, test_dpi, test_dpp,
    test_idd, test_idi, test_idp, test_iid, test_iii, test_iip, test_ipd,
    test_ipi, test_ipp, test_pdd, test_pdi, test_pdp, test_pid, test_pii,
    test_pip, test_ppd, test_ppi, test_ppp,
  };
  const char *call_name[] = {
    "test_d",   "test_i",   "test_p",   "test_dd",  "test_di",  "test_dp",
    "test_id",  "test_ii",  "test_ip",  "test_pd",  "test_pi",  "test_pp",
    "test_ddd", "test_ddi", "test_ddp", "test_did", "test_dii", "test_dip",
    "test_dpd", "test_dpi", "test_dpp", "test_idd", "test_idi", "test_idp",
    "test_iid", "test_iii", "test_iip", "test_ipd", "test_ipi", "test_ipp",
    "test_pdd", "test_pdi", "test_pdp", "test_pid", "test_pii", "test_pip",
    "test_ppd", "test_ppi", "test_ppp",
  };
  clear_result();
  for (int j = 0; j < ARRAY_LENGTH(call_address); ++j) {
    gf = (Functor_t){ .call = call_address[j] };
    printf("\n\nTesting call %s\n", call_name[j]);
    for (int i = 0; i <= PARAM_COUNT; ++i) {
      param_choose_n("dip", i, global_result);
    }
  }

  return 0;
}
