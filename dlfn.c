#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define __USE_GNU
#include <dlfcn.h>
#include <link.h>

#include "dlfn.h"
#include "macro.h"

static const int MAX_PATH = 128;
static char dlname[MAX_PATH];
static void *dlhandle;

// visible
Symbol_t dlfnSymbols[MAX_SYMBOLS];
int dlfnUsedSymbols;
Object_t dlfnObjects[MAX_SYMBOLS];
int dlfnUsedObjects;

static void
reset_symbols()
{
  memset(dlfnSymbols, 0, sizeof(dlfnSymbols));
  dlfnUsedSymbols = 0;
}

static void
add_symbol(Symbol_t sym)
{
  if (dlfnUsedSymbols >= MAX_SYMBOLS)
    return;
  dlfnSymbols[dlfnUsedSymbols++] = sym;
}

static void
add_object(Object_t obj)
{
  if (dlfnUsedObjects >= MAX_SYMBOLS)
    return;
  dlfnObjects[dlfnUsedObjects++] = obj;
}

int
symbol_order(const void *lhs, const void *rhs)
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
sort_symbols()
{
  qsort(dlfnSymbols, dlfnUsedSymbols, sizeof(dlfnSymbols[0]), symbol_order);
}

void
dlfn_print_symbols()
{
  for (int i = 0; i < dlfnUsedSymbols; ++i) {
    printf("%s: %p\n", dlfnSymbols[i].name,
           (void *) dlfnSymbols[i].fnctor.call);
  }
}

void
dlfn_call(const char *name)
{
  for (int i = 0; i < dlfnUsedSymbols; ++i) {
    if (strcmp(name, dlfnSymbols[i].name) == 0) {
      int r = dlfnSymbols[i].fnctor.call();
      printf("%s returns %d\n", name, r);
    }
  }
}

Functor_t *
dlfn_get_symbol(const char *name)
{
  for (int i = 0; i < dlfnUsedSymbols; ++i) {
    if (strcmp(name, dlfnSymbols[i].name) == 0) {
      return &dlfnSymbols[i].fnctor;
    }
  }

  return 0;
}

void *
dlfn_get_object(const char *name)
{
  for (int i = 0; i < dlfnUsedObjects; ++i) {
    if (strcmp(name, dlfnObjects[i].name) == 0) {
      return dlfnObjects[i].address;
    }
  }

  return 0;
}

static void
parse_symtab(void *addr, void *symtab, void *strtab)
{
  ElfW(Sym *) iter = symtab;
  for (; (void *) iter < strtab; ++iter) {
    if (ELF64_ST_TYPE(iter->st_info) != STT_FUNC)
      continue;
    if (ELF64_ST_BIND(iter->st_info) != STB_GLOBAL)
      continue;
    if (iter->st_size == 0)
      continue;
    Symbol_t s = { .name = &strtab[iter->st_name],
                   .fnctor = functor_init((void *) (addr + iter->st_value)) };
    add_symbol(s);
    printf("Symbol %s addr %p\n", s.name, s.fnctor.call);
  }

  iter = symtab;
  for (; (void *) iter < strtab; ++iter) {
    if (ELF64_ST_TYPE(iter->st_info) != STT_OBJECT)
      continue;
    if (ELF64_ST_BIND(iter->st_info) != STB_GLOBAL)
      continue;
    if (strcmp(&strtab[iter->st_name], "tick"))
      continue;
    Object_t o = { .name = &strtab[iter->st_name],
                   .address = addr + iter->st_value,
                   .bytes = iter->st_size };

    add_object(o);
    printf("global object %s addr %p\n", o.name, o.address);
  }
}

static void
parse_dynamic_header(void *addr, const ElfW(Phdr) * phdr)
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
    parse_symtab(addr, symtab, strtab);
}

static void
parse_elf_header(void *dlAddr)
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
    parse_dynamic_header(dlAddr, dynHeader);
}

bool
dlfn_open()
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
      parse_elf_header((void *) iter->l_addr);
    }
    iter = iter->l_next;
  }

  // Functors follow call ordering of the address space
  sort_symbols();

  return true;
}

bool
dlfn_close()
{
  if (!dlhandle)
    return false;

  reset_symbols();
  int result = dlclose(dlhandle);
  if (result != 0)
    puts(dlerror());
  dlhandle = NULL;

  return true;
}

bool
dlfn_init(const char *filepath)
{
  if (!strstr(filepath, ".so"))
    return false;

  strncpy(dlname, filepath, sizeof(dlname));
  NULLTERM(dlname);

  return true;
}

void
dlfn_shutdown()
{
  dlfn_close();
  dlname[0] = 0;
}

