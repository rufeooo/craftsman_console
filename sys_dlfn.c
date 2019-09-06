#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define __USE_GNU
#include <dlfcn.h>
#include <link.h>

#include "sys_dlfn.h"

typedef struct {
  const char *name;
  int (*func)();
} Symbol_t;

static const char *dlname = "feature.so";
static void *dlhandle;
static const int MAX_SYMBOLS = 32;
static Symbol_t symbols[MAX_SYMBOLS];
static int usedSymbols;

static void
resetSymbols()
{
  memset(symbols, 0, sizeof(symbols));
  usedSymbols = 0;
}

static void
addSymbol(Symbol_t sym)
{
  if (usedSymbols >= MAX_SYMBOLS)
    return;
  symbols[usedSymbols++] = sym;
}

void
sys_dlfnPrintSymbols()
{
  for (int i = 0; i < usedSymbols; ++i) {
    printf("%s: %p\n", symbols[i].name, (void *) symbols[i].func);
  }
}

void
sys_dlfnCall(const char *name)
{
  for (int i = 0; i < usedSymbols; ++i) {
    if (strcmp(name, symbols[i].name) == 0) {
      int r = symbols[i].func();
      printf("%s returns %d\n", name, r);
    }
  }
}

static void
parseSymtab(void *addr, void *symtab, void *strtab)
{
  ElfW(Sym *) iter = symtab;
  for (; (void *) iter < strtab; ++iter) {
    if (ELF64_ST_TYPE(iter->st_info) != STT_FUNC)
      continue;
    Symbol_t s = { .name = &strtab[iter->st_name],
                   .func = (void *) (addr + iter->st_value) };
    addSymbol(s);
    printf("Symbol %s addr %p\n", s.name, s.func);
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
  dlhandle = dlopen("feature.so", RTLD_NOW);
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
