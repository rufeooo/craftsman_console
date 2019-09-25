#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define __USE_GNU
#include <dlfcn.h>
#include <link.h>

#include "macro.h"
#include "sys_dlfn.h"

static const int MAX_PATH = 128;
static char dlname[MAX_PATH];
static void *dlhandle;

// visible
Symbol_t dlfnSymbols[MAX_SYMBOLS];
int dlfnUsedSymbols;

static void
resetSymbols()
{
  memset(dlfnSymbols, 0, sizeof(dlfnSymbols));
  dlfnUsedSymbols = 0;
}

static void
addSymbol(Symbol_t sym)
{
  if (dlfnUsedSymbols >= MAX_SYMBOLS)
    return;
  dlfnSymbols[dlfnUsedSymbols++] = sym;
}

int
symbolOrdering(const void *lhs, const void *rhs)
{
  const Symbol_t *lhv = lhs;
  const Symbol_t *rhv = rhs;
  size_t lhfn = (size_t) lhv->fnctor.call;
  size_t rhfn = (size_t) rhv->fnctor.call;

  if (lhfn < rhfn)
    return -1;
  else if (lhfn > rhfn)
    return 1;
  else
    return 0;
}

void
sortSymbols()
{
  qsort(dlfnSymbols, dlfnUsedSymbols, sizeof(dlfnSymbols[0]), symbolOrdering);
}

void
sys_dlfnPrintSymbols()
{
  for (int i = 0; i < dlfnUsedSymbols; ++i) {
    printf("%s: %p\n", dlfnSymbols[i].name,
           (void *) dlfnSymbols[i].fnctor.call);
  }
}

void
sys_dlfnCall(const char *name)
{
  for (int i = 0; i < dlfnUsedSymbols; ++i) {
    if (strcmp(name, dlfnSymbols[i].name) == 0) {
      int r = dlfnSymbols[i].fnctor.call();
      printf("%s returns %d\n", name, r);
    }
  }
}

Functor_t
sys_dlfnGet(const char *name)
{
  for (int i = 0; i < dlfnUsedSymbols; ++i) {
    if (strcmp(name, dlfnSymbols[i].name) == 0) {
      return dlfnSymbols[i].fnctor;
    }
  }

  return (Functor_t){};
}

static void
parseSymtab(void *addr, void *symtab, void *strtab)
{
  ElfW(Sym *) iter = symtab;
  for (; (void *) iter < strtab; ++iter) {
    if (ELF64_ST_TYPE(iter->st_info) != STT_FUNC)
      continue;
    if (iter->st_size == 0)
      continue;
    Symbol_t s = { .name = &strtab[iter->st_name],
                   .fnctor = functor_init((void *) (addr + iter->st_value)) };
    addSymbol(s);
    printf("Symbol %s addr %p\n", s.name, s.fnctor.call);
  }
}

static void
parseDynamicHeader(void *addr, const ElfW(Phdr) * phdr)
{
  ElfW(Addr) base = (ElfW(Addr)) addr;
  ElfW(Dyn) *dyn = (void *) (base + phdr->p_vaddr);

  void *strtab = NULL;
  void *symtab = NULL;
  while (dyn->d_tag != 0) {
    switch (dyn->d_tag) {
    case DT_STRTAB:
      strtab = (void *) dyn->d_un.d_ptr;
      break;
    case DT_SYMTAB:
      symtab = (void *) dyn->d_un.d_ptr;
      break;
    }
    ++dyn;
  }

  if (symtab)
    parseSymtab(addr, symtab, strtab);
}

static void
parseElfHeader(void *dlAddr)
{
  ElfW(Ehdr) *elfHeader = dlAddr;
  ElfW(Phdr) *pHeader = (void *) (dlAddr + elfHeader->e_phoff);
  void *dynHeader = NULL;
  for (size_t i = 0; i < elfHeader->e_phnum; ++i) {
    printf("\t\t header %zu: address=%10p type=0x%x\n", i,
           (void *) (dlAddr + pHeader->p_vaddr), pHeader->p_type);
    if (pHeader->p_type == PT_DYNAMIC) {
      dynHeader = pHeader;
    }
    pHeader = (void *) pHeader + elfHeader->e_phentsize;
  }
  if (dynHeader)
    parseDynamicHeader(dlAddr, dynHeader);
}

bool
sys_dlfnOpen()
{
  dlhandle = dlopen(dlname, RTLD_NOW);
  Dl_info info;
  struct link_map *dllm;
  if (!dlhandle) {
    puts(dlerror());
    return false;
  }

  printf("dlhandle %p\n", dlhandle);
  if (dlinfo(dlhandle, RTLD_DI_LINKMAP, &dllm) || !dllm) {
    puts(dlerror());
    return false;
  }

  puts("link_map populated\n");
  struct link_map *iter = dllm;
  while (iter) {
    printf("l_name %s l_addr %p\n", iter->l_name, (void *) iter->l_addr);
    if (strstr(iter->l_name, dlname)) {
      parseElfHeader((void *) iter->l_addr);
    }
    iter = iter->l_next;
  }

  // Functors follow call ordering of the address space
  sortSymbols();

  return true;
}

bool
sys_dlfnClose()
{
  if (!dlhandle)
    return false;

  resetSymbols();
  int result = dlclose(dlhandle);
  if (result != 0)
    puts(dlerror());
  dlhandle = NULL;

  return true;
}

bool
sys_dlfnInit(const char *filepath)
{
  if (!strstr(filepath, ".so"))
    return false;

  strncpy(dlname, filepath, sizeof(dlname));
  NULLTERM(dlname);

  return true;
}

void
sys_dlfnShutdown()
{
  sys_dlfnClose();
  dlname[0] = 0;
}

