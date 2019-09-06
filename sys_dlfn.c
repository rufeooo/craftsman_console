
typedef struct {
  const char *name;
  int (*func)();
} Symbol_t;

static const char *dlname = "feature.so";
static void *dlp;
static const int MAX_SYMBOLS = 32;
static Symbol_t symbols[MAX_SYMBOLS];
static int usedSymbols;

void
resetSymbols()
{
  memset(symbols, 0, sizeof(symbols));
  usedSymbols = 0;
}

void
addSymbol(Symbol_t sym)
{
  if (usedSymbols >= MAX_SYMBOLS)
    return;
  symbols[usedSymbols++] = sym;
}

void
printSymbols()
{
  for (int i = 0; i < usedSymbols; ++i) {
    printf("%s: %p\n", symbols[i].name, (void *) symbols[i].func);
  }
}

void
callSymbols()
{
  for (int i = 0; i < usedSymbols; ++i) {
    int r = symbols[i].func();
    printf("return %d\n", r);
  }
}

void
callSymbol(const char *name)
{
  for (int i = 0; i < usedSymbols; ++i) {
    if (strcmp(name, symbols[i].name) == 0) {
      int r = symbols[i].func();
      printf("%s returns %d\n", name, r);
    }
  }
}

void
dynamic_header(ElfW(Addr) base, const ElfW(Phdr) * phdr)
{
  ElfW(Dyn) *dyn = (void *) (base + phdr->p_vaddr);

  printf("dynamic offset 0x%lx\n", phdr->p_vaddr);
  void *strtab = NULL;
  while (dyn->d_tag != 0) {
    printf("0x%lx ", dyn->d_tag);
    if (dyn->d_tag == DT_STRTAB) {
      strtab = (void *) dyn->d_un.d_ptr;
    } else if (dyn->d_tag == DT_SYMTAB) {
      puts("\ndt_symtab\n");
      ElfW(Sym *) sym = (ElfW(Sym) *) dyn->d_un.d_ptr;
      printf("\nsymptr %p\n", (void *) sym);
      for (; (void *) sym < strtab; ++sym) {
        if (ELF64_ST_TYPE(sym->st_info) != STT_FUNC)
          continue;
        Symbol_t s = { .name = &strtab[sym->st_name],
                       .func = (void *) (base + sym->st_value) };
        addSymbol(s);

        printf("0x%lx [0x%x]", sym->st_value, ELF64_ST_TYPE(sym->st_info));
        if (strtab)
          printf("%s ", &strtab[sym->st_name]);
      }
      puts("\nend dt_symtab\n");
      printf("symptr %p\n", (void *) sym);
    }
    ++dyn;
  }
}

static int
phdr_capture(struct dl_phdr_info *info, size_t size, void *data)
{
  puts(info->dlpi_name);
  if (!info->dlpi_addr)
    return 0;
  if (!strstr(info->dlpi_name, dlname))
    return 0;
  printf("addr 0x%p dlpi_phdr 0x%p\n", (void *) info->dlpi_addr,
         info->dlpi_phdr);
  for (unsigned j = 0; j < info->dlpi_phnum; j++) {
    printf("\t\t header %2d: address=%10p type=0x%x\n", j,
           (void *) (info->dlpi_addr + info->dlpi_phdr[j].p_vaddr),
           info->dlpi_phdr[j].p_type);
    if (info->dlpi_phdr[j].p_type == PT_DYNAMIC)
      dynamic_header(info->dlpi_addr, &info->dlpi_phdr[j]);

    // dlinfo
  }
  return 0;
}

static int
ehdr_capture(struct dl_phdr_info *info, size_t size, void *data)
{
  if (!info->dlpi_addr)
    return 0;
  if (!strstr(info->dlpi_name, dlname))
    return 0;
  puts(info->dlpi_name);
  ElfW(Ehdr) *ehdr = (void *) info->dlpi_addr;
  printf("phnum %d shnum %d phoff 0x%p shoff 0x%p\n", ehdr->e_phnum,
         ehdr->e_shnum, (void *) ehdr->e_phoff, (void *) ehdr->e_shoff);
  ElfW(Shdr) *shdr = (void *) (info->dlpi_addr + ehdr->e_shoff);
  printf("addr 0x%p shdr 0x%p\n", (void *) info->dlpi_addr, shdr);
  for (int i = 0; i < ehdr->e_shnum; ++i, ++shdr) {
    printf("%d ", shdr->sh_type);
  }
  return 0;
}

static int
phdr_callback(struct dl_phdr_info *info, size_t size, void *data)
{
  if (!info->dlpi_addr)
    return 0;
  if (!strstr(info->dlpi_name, dlname))
    return 0;
  printf("0x%p %s \n", (void *) info->dlpi_addr, info->dlpi_name);
  return 0;
}

bool
openLibrary()
{
  dlp = dlopen("feature.so", RTLD_NOW);
  Dl_info info;
  struct link_map *dllm;
  if (dlp) {
    // void* baseaddr = dlsym(dlp, NULL);
    // printf("dlp baseaddr %p\n", baseaddr);
    if (dlinfo(dlp, RTLD_DI_LINKMAP, &dllm)) {
      puts(dlerror());
    }
    if (dllm) {
      puts("link_map populated\n");
      struct link_map *iter = dllm;
      while (iter) {
        printf("l_name %s l_addr %p\n", iter->l_name, (void *) iter->l_addr);
        iter = iter->l_next;
      }
    }

    /*if (0 == dladdr(dlp, &info)) {
      puts(dlerror());
    }
    else {
      puts("loaded\n");
      printf("dlp %s %p\n", info.dli_fname, info.dli_fbase);
    }*/

    // TODO: cast dlp to ELF file header
    // find section offset, and section count
    // iterate sections looking for dynsym
    printf("dlp addr %p\n", dlp);
    dl_iterate_phdr(phdr_capture, NULL);
  } else {
    puts(dlerror());
  }
  return dlp != 0;
}

bool
closeLibrary()
{
  if (!dlp)
    return false;

  resetSymbols();
  int result = dlclose(dlp);
  if (result != 0)
    puts(dlerror());
  dlp = NULL;

  return true;
}

static inline void
trySymbol()
{
  char buffer[128];
  if (fgets(buffer, sizeof(buffer), stdin)) {
    char *str = buffer;
    while (*str == ' ')
      ++str;
    char *end = str;
    while (*end != '\n' && *end != 0)
      ++end;
    *end = 0;
    callSymbol(str);
  }
}
