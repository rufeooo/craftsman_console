#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define __USE_GNU
#include <dlfcn.h>
#include <link.h>

#include "functor.c"
#include "macro.h"

// extern
typedef int (*__compar_fn_t)(const void *, const void *);
extern void qsort(void *__base, size_t __nmemb, size_t __size,
                  __compar_fn_t __compar) __nonnull((1, 4));

typedef struct {
  const char *name;
  Functor_t fnctor;
} Function_t;

typedef struct {
  const char *name;
  void *address;
  size_t bytes;
} Object_t;

#define MAX_SYMBOLS 32
#define MAX_PATH 128
static char dlname[MAX_PATH];
static void *dlhandle;

// visible
Function_t dlfn_function[MAX_SYMBOLS];
int dlfn_used_function;
Object_t dlfn_object[MAX_SYMBOLS];
int dlfn_used_object;

int
symbol_order(const void *lhs, const void *rhs)
{
  const Function_t *lhv = lhs;
  const Function_t *rhv = rhs;
  size_t lhfn = (size_t)lhv->fnctor.call;
  size_t rhfn = (size_t)rhv->fnctor.call;

  if (lhfn < rhfn)
    return -1;
  else if (lhfn > rhfn)
    return 1;
  else
    return 0;
}

void
sort_functions()
{
  qsort(dlfn_function, dlfn_used_function, ARRAY_MEMBER_SIZE(dlfn_function),
        symbol_order);
}

int
object_order(const void *lhs, const void *rhs)
{
  const Object_t *lhv = lhs;
  const Object_t *rhv = rhs;

  if (lhv->address < rhv->address)
    return -1;
  else if (lhv->address > rhv->address)
    return 1;
  else
    return 0;
}

void
sort_objects()
{
  qsort(dlfn_object, dlfn_used_object, ARRAY_MEMBER_SIZE(dlfn_object),
        object_order);
}

void
dlfn_print_functions()
{
  printf("--Functions--\n");
  for (int i = 0; i < dlfn_used_function; ++i) {
    printf("[ %p ] %s\n", dlfn_function[i].fnctor.call, dlfn_function[i].name);
  }
}

void
dlfn_print_objects()
{
  printf("--Objects--\n");
  for (int i = 0; i < dlfn_used_object; ++i) {
    Object_t *obj = &dlfn_object[i];
    printf("[ %zu bytes ] %s: %p\n", obj->bytes, obj->name, obj->address);
  }
}

void
dlfn_call(const char *name)
{
  for (int i = 0; i < dlfn_used_function; ++i) {
    if (strcmp(name, dlfn_function[i].name) == 0) {
      int r = dlfn_function[i].fnctor.call();
      printf("%s returns %d\n", name, r);
    }
  }
}

Function_t *
dlfn_get_symbol(const char *name)
{
  for (int i = 0; i < dlfn_used_function; ++i) {
    if (strcmp(name, dlfn_function[i].name) == 0) {
      return &dlfn_function[i];
    }
  }

  return 0;
}

Object_t *
dlfn_get_object(const char *name)
{
  for (int i = 0; i < dlfn_used_object; ++i) {
    if (strcmp(name, dlfn_object[i].name) == 0) {
      return &dlfn_object[i];
    }
  }

  return 0;
}

static void
parse_symtab(void *addr, void *symtab, void *strtab)
{
  const char *read_strtab = strtab;
  ElfW(Sym *) iter = symtab;
  for (; (void *)iter < strtab; ++iter) {
    if (ELF64_ST_TYPE(iter->st_info) != STT_FUNC) continue;
    if (ELF64_ST_BIND(iter->st_info) != STB_GLOBAL) continue;
    if (iter->st_size == 0) continue;
    Function_t s = {.name = &read_strtab[iter->st_name],
                    .fnctor = functor_init(addr + iter->st_value)};

    if (dlfn_used_function >= MAX_SYMBOLS) break;
    dlfn_function[dlfn_used_function++] = s;
  }

  iter = symtab;
  for (; (void *)iter < strtab; ++iter) {
    if (ELF64_ST_TYPE(iter->st_info) != STT_OBJECT) continue;
    if (ELF64_ST_BIND(iter->st_info) != STB_GLOBAL) continue;
    if (iter->st_size == 0) continue;
    Object_t o = {.name = &read_strtab[iter->st_name],
                  .address = addr + iter->st_value,
                  .bytes = iter->st_size};

    if (dlfn_used_object >= MAX_SYMBOLS) break;
    dlfn_object[dlfn_used_object++] = o;
  }
}

static void
parse_dynamic_header(void *base, const ElfW(Phdr) * phdr)
{
  ElfW(Addr) addr = (ElfW(Addr))base;
  ElfW(Dyn) *dyn = (base + phdr->p_vaddr);

  void *strtab = NULL;
  void *symtab = NULL;
  while (dyn->d_tag != 0) {
    switch (dyn->d_tag) {
      case DT_STRTAB:
        strtab = (void *)dyn->d_un.d_ptr;
        break;
      case DT_SYMTAB:
        symtab = (void *)dyn->d_un.d_ptr;
        break;
    }
    ++dyn;
  }

  if (symtab) parse_symtab(base, symtab, strtab);
}

static void
parse_elf_header(void *base)
{
  ElfW(Ehdr) *elfHeader = base;
  ElfW(Phdr) *pHeader = base + elfHeader->e_phoff;
  void *dynHeader = NULL;
  for (size_t i = 0; i < elfHeader->e_phnum; ++i) {
    printf("\t\t header %zu: address=%10p type=0x%x\n", i,
           base + pHeader->p_vaddr, pHeader->p_type);
    if (pHeader->p_type == PT_DYNAMIC) {
      dynHeader = pHeader;
    }
    pHeader = (void *)pHeader + elfHeader->e_phentsize;
  }
  if (dynHeader) parse_dynamic_header(base, dynHeader);
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
    printf("l_name %s l_addr %p\n", iter->l_name, (void *)iter->l_addr);
    if (strstr(iter->l_name, dlname)) {
      parse_elf_header((void *)iter->l_addr);
    }
    iter = iter->l_next;
  }

  // Functors follow call ordering of the address space
  sort_functions();
  sort_objects();

  return true;
}

bool
dlfn_close()
{
  if (!dlhandle) return false;

  memset(dlfn_function, 0, sizeof(dlfn_function));
  dlfn_used_function = 0;
  memset(dlfn_object, 0, sizeof(dlfn_object));
  dlfn_used_object = 0;
  int result = dlclose(dlhandle);
  if (result != 0) puts(dlerror());
  dlhandle = NULL;

  return true;
}

bool
dlfn_init(const char *filepath)
{
  if (!strstr(filepath, ".so")) return false;

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
