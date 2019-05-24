#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <elf.h>

const char *g_buf;			// global buf that holds each file contents
const char *g_filename;		

void print_usage();

typedef int (*proc)(void);

// procedure declarations
int display_file_header();
int display_section_headers();
int display_symbol_table();
int dummy_proc();
int unimplemented_proc();

// procedure indexes in the following 'option_xxx' arrays
#define OPT_h		0
#define OPT_S		1
#define OPT__DEC	2
#define OPT_s		3

static const char *option_chars = "hS s";
static const char *option_strs[] = {"--file-header", "--section-headers", "--dec", "--symbols", NULL};
static const proc option_procs[] = {display_file_header, display_section_headers, dummy_proc, display_symbol_table, NULL};

#define PROC_COUNT (sizeof(option_procs) / sizeof(proc))
// invoke produces in 'procs' if not NULL, after options and files have been parsed
proc procs[PROC_COUNT] = {NULL}; 

int main(const int argc, const char *argv[])
{
	if (argc < 3 ){ 
		print_usage();
		return 0;
	}
	
	
	int arg_i;

	// parse options
	for(arg_i = 1; arg_i < argc; arg_i ++) {
		const char *arg = argv[arg_i];
		int option_idx = 0;
		
		if (arg[0] != '-') break;
	
		// arg: -h -S ...
		if (arg[1] != '-') {
			char *p_char; // points to the matched char in option_chars
			
			if (strlen(arg) != 2 || (p_char = strchr(option_chars, arg[1])) == NULL) {
				printf("ELF-reader: invalid option '%s' \n", arg);
				print_usage();
				return 0;
			}
			
			option_idx = p_char - option_chars;
		} // arg: --file-header --section-headers ...		
		else if (arg[1] == '-') {

			int i = 0;
			const char *option;
			
			while((option = option_strs[i++]) != NULL) {
				if (strcmp(option, arg) == 0) break;
				option_idx ++;
			}
			
			if (option == NULL) {
				printf("ELF-reader: invalid option '%s' \n", arg);
				print_usage();
				return 0;		
			}
		}
		
		procs[option_idx] = option_procs[option_idx];
	}
	
	if (arg_i == argc) {
		printf("ELF-reader: No input files \n");
		print_usage();
		return 0;
	}
	
	// read and process input files one by one
	while (arg_i < argc) {
		g_filename = argv[arg_i];
		
		// check file accessibility
		if (access(g_filename, F_OK | R_OK) != 0) {
			if (errno == ENOENT) {
				printf("ELF-reader: Error: '%s': No such file \n", g_filename);					
			} else if (errno == EACCES) {
				printf("ELF-reader: Error: '%s': %s \n", g_filename, strerror(errno));			
			}
			arg_i ++;
			continue; // process other files
		}

		// open file for reading
		FILE *fp = fopen(g_filename, "r");
		if (fp == NULL) {
			printf("ELF-reader: Error: '%s': %s \n", g_filename, strerror(errno));
			return 1;
		}
		
		// get file size and read the whole file
		fseek(fp, 0, SEEK_END);
		size_t file_size = ftell(fp);
		rewind(fp);
		
		if ((g_buf = malloc(file_size)) == NULL || fread((void*)g_buf, 1, file_size, fp) != file_size) {
			printf("ELF-reader: Error: '%s': %s \n", g_filename, strerror(errno)); 
			return 1;
		}
	
		// check for ELF magic number
		Elf32_Ehdr *p_header = (Elf32_Ehdr *)g_buf;
		if (strncmp((void*)p_header, ELFMAG, SELFMAG) != 0) {
			printf("ELF-reader: Notice: file '%s' is not an ELF. \n", g_filename);
			return 0;
		}

		// check for elf class. now only ELF32 is supported. TODO we shall support ELF64 header later
		switch (p_header->e_ident[EI_CLASS]){
			case ELFCLASS32:
				// ok
			break;
		case ELFCLASS64:
			printf("ELF-reader: Notice: sorry, ELF64 file '%s' is currently not supported \n", g_filename);
			arg_i ++;
			continue;
		case ELFCLASSNONE:
		default:
			printf("ELF-reader: Notice: file '%s' is an invalid ELF. \n", g_filename);
			arg_i ++;
			continue;
		}
		
		// now invoke each procedure in proc array
		for(int i = 0; i < PROC_COUNT; i ++) {
			if (procs[i] == NULL) continue;
			
			if (procs[i]() == 1) {
				printf("ELF-reader: Error: failed to process file '%s', skip it \n", g_filename);
				break;
			}
		}
		free((void*)g_buf);
		arg_i ++;
	}
	
	return 0;	
}

void print_usage()
{
	puts("Usage: ELF-reader <option(s)> elf-file(s)");
	puts(" Display information about the contents of ELF format files");
	puts(" Options are:");
	puts(" -h --file-header       Display the ELF file header");
	puts(" -S --section-headers   Display the sections' header");
	puts(" -s --symbols           Display the symbol table");

}

// procedures for each ELF-reader options
int display_file_header()
{
	const Elf32_Ehdr *header = (Elf32_Ehdr*)g_buf;

	// Strings for meanings for some of the header fields.
	static char *elf_classes[ELFCLASSNUM] = {"invalid", "ELF32", "ELF64"};
	static char *elf_endians[ELFDATANUM] = {"invalid", 
			"2's complement, little endian", "2's complement, big endian"};
			
	static char *elf_os[256] = {NULL};
	elf_os[ELFOSABI_NONE] = "Unix System V ABI";
	elf_os[ELFOSABI_HPUX] = "HP-UX";
	elf_os[ELFOSABI_NETBSD] = "NetBSD";
	elf_os[ELFOSABI_GNU] = "Object uses GNU ELF extensions";
	elf_os[ELFOSABI_SOLARIS] = "Sun Solaris";
	elf_os[ELFOSABI_AIX] = "IBM AIX";
	elf_os[ELFOSABI_IRIX] = "SGI Irix";
	elf_os[ELFOSABI_FREEBSD] = "FreeBSD";
	elf_os[ELFOSABI_TRU64] = "Compaq TRU64 UNIX";
	elf_os[ELFOSABI_MODESTO] = "Novell Modesto";
	elf_os[ELFOSABI_OPENBSD] = "OpenBSD";
	elf_os[ELFOSABI_ARM_AEABI] = "ARM EABI";
	elf_os[ELFOSABI_ARM] = "ARM";			
	elf_os[ELFOSABI_STANDALONE] = "Standalone (embedded) application";

	static char *elf_types[ET_NUM] = { "NO TYPE", "REL", "EXE", "SHARED", "CORE"};
	static char *elf_machines[EM_NUM] = {NULL};
	elf_machines[EM_NONE] = "No machine";
	elf_machines[EM_SPARC] = "SUN SPARC";
	elf_machines[EM_386] = "Intel 80386";
	elf_machines[EM_860] = "Intel 80860";
	elf_machines[EM_PPC] = "PowerPC";
	elf_machines[EM_ARM] = "ARM";
	elf_machines[EM_X86_64] = "AMD x86-64 architecture";
	// the list goes on... See elf.h for more info
	
	// All ok. now print header info
	printf("Elf Header: \n");
	// Header identification. Magic number and other iWarningnfo
	printf(" off len  field: value \n");
	printf("  %02d -%2d  Magic: .%3.3s \n", EI_MAG0, SELFMAG, (char*)header + 1); 
	printf("  %02d -%2d  Class: %s \n", EI_CLASS, 1, elf_classes[header->e_ident[EI_CLASS]]);
	printf("  %02d -%2d  Data : %s \n", EI_DATA, 1, elf_endians[header->e_ident[EI_DATA]]);
	printf("  %02d -%2d  Version: %d (current) \n", EI_VERSION, 1, header->e_ident[EI_VERSION]);
	printf("  %02d -%2d  OS/ABI: %s \n", EI_OSABI, 1, elf_os[header->e_ident[EI_OSABI]] ? : "unknown");
	printf("  %02d -%2d  ABI Version: %d \n", EI_ABIVERSION, 1, header->e_ident[EI_ABIVERSION]);
	printf("  %02d -%2d  Padding... \n", EI_PAD, EI_NIDENT - 1 - EI_PAD);
	
	size_t idx = EI_NIDENT;
	size_t field_size;

	// Object file type
	field_size = sizeof(header->e_type);
	printf("  %02ld -%2ld  Type: %s \n", idx, field_size, elf_types[header->e_type]);
	idx += field_size;
	
	// Architecture
	field_size = sizeof(header->e_machine);
	printf("  %02ld -%2ld  Arch: %s \n", idx, field_size, elf_machines[header->e_machine] ? : "unknown");
	idx += field_size;
	
	// Object file version
	field_size = sizeof(header->e_version);
	printf("  %02ld -%2ld  Version: %d (same as the above)\n", idx, field_size, header->e_version);
	idx += field_size;

	// Entry point virtual address
	field_size = sizeof(header->e_entry);
	printf("  %02ld -%2ld  Entry: 0x%08x \n", idx, field_size, header->e_entry);
	idx += field_size;
	
	// Program header table file offset
	field_size = sizeof(header->e_phoff);
	printf("  %02ld -%2ld  Start of program header: %d (bytes into file) \n", idx, field_size, header->e_phoff);
	idx += field_size;
	
	// Section header table file offset
	printf("  %02ld -%2ld  Start of section header: %d (bytes into file) \n", idx, field_size, header->e_shoff);
	idx += field_size;
	
	// Processor-specific flags
	printf("  %02ld -%2ld  Flags: 0x%x \n", idx, field_size, header->e_flags);
	idx += field_size;
	
	// ELF header size in bytes
	printf("  %02ld -%2ld  Size of this header: %d (bytes) \n", idx, field_size, header->e_ehsize);
	idx += field_size;
	
	// Program header table entry size
	printf("  %02ld -%2ld  Size of program headers: %d (bytes) \n", idx, field_size, header->e_phentsize);
	idx += field_size;
	
	// Program header table entry count
	printf("  %02ld -%2ld  Number of program headers: %d \n", idx, field_size, header->e_phnum);
	idx += field_size;
	
	// Section header table entry count
	printf("  %02ld -%2ld  Size of section headers: %d (bytes) \n", idx, field_size, header->e_shentsize);
	idx += field_size;
	
	// Section header table
	printf("  %02ld -%2ld  Number of section headers: %d \n", idx, field_size, header->e_shnum);
	idx += field_size;
	
	// Section header string table index
	printf("  %02ld -%2ld  Section number string table index: %d \n", idx, field_size, header->e_shstrndx);
	idx += field_size;

	return 0;
}

int display_section_headers()
{
	Elf32_Ehdr *eh = (Elf32_Ehdr *)g_buf;
	Elf32_Shdr *sh_base = (Elf32_Shdr *)(g_buf + eh->e_shoff);
	Elf32_Shdr *str_sh = sh_base + eh->e_shstrndx;

	char *str_table = (char*)(g_buf + str_sh->sh_offset);

	// type string table
	static char *sh_types[SHT_NUM] = {"NULL", "PROGBITS", "SYMTAB", "STRTAB", "RELA", "HASH",
		"DYNAMIC", "NOTE", "NOBITS", "REL", "SHLIB", "DYNSYM", "NULL", "NULL", "INI_ARRAY", 
		"FINI_ARRAY", "PREINIT_ARRAY", "GROUP", "SYMTAB_SHNDX"};

	// flag characters set
	static char *sh_flags = "WAX MSILOGTCx"; // TODO support o, E,p flags

	// check section header size
	if (eh->e_shentsize != sizeof(Elf32_Shdr)) {
		printf("ELF-reader: Error: section header in '%s' should be %ld but is %d bytes. skip it.", 
			g_filename, sizeof(Elf32_Shdr), eh->e_shentsize);
		return 1;
	}

	// display section header infos
	if (procs[OPT_h] == NULL){
		printf("There are %d section headers, starting at offset 0x%x \n", eh->e_shnum, eh->e_shoff);
	}
	printf("\nSection Headers:\n");    // Note 'ES' is entry size for some sections
	printf(" [Nr] %-17s %-15s %-8s %-6s %-6s %s %s %s %s %s \n", 
		"Name", "Type", "Addr", "Off", "Size", "ES", "Flg", "LK", "Inf", "Al");

	for(int i = 0; i < eh->e_shnum; i ++) {
		Elf32_Shdr *sh = sh_base + i;

		const char *name = str_table + sh->sh_name;
		const char *type = sh_types[sh->sh_type];
		unsigned int addr = sh->sh_addr;
		unsigned int off = sh->sh_offset;
		unsigned int size = sh->sh_size;
		char flags[strlen(sh_flags)]; 
		{	// parse flag characters
			unsigned int flag_bits = sh->sh_flags;
			int flag_num = strlen(sh_flags);
			int flag_cnt = 0;
					
			int flag_i = 0;
			while(flag_i < flag_num) {
				if (flag_bits & 1) {
					flags[flag_cnt ++] = sh_flags[flag_i];
				}
				flag_bits = flag_bits >> 1;
				flag_i ++;
			}
			flags[flag_cnt] = '\0';
		}
		unsigned int entry_size = sh->sh_entsize;		
		unsigned int link = sh->sh_link;
		unsigned int info = sh->sh_info;
		unsigned int align = sh->sh_addralign;

		if (procs[OPT__DEC]) {
			printf(" [%2d] %-17.17s %-15s %-8.8x %6.6d %6.6d %2.2x %3s %2d %3d %2d \n",
				i, name, type, addr, off, size, entry_size, flags, link, info, align);
		} else {
			printf(" [%2d] %-17.17s %-15s %-8.8x %6.6x %6.6x %2.2x %3s %2d %3d %2d \n", 
				i, name, type, addr, off, size, entry_size, flags, link, info, align);
		}
	}

	// print flags explanation
	printf("Key to Flags: \n \
W (write), A (alloc), X (execute), M (merge), S (strings), I (info), \n \
L (link order), O (extra OS processing required), G (group), T (TLS), \n \
C (compressed), x (unknown), o (OS specific), E (exclude) \n \
p (processor specific) \n\n");

	return 0;
}

int display_symbol_table()
{
	Elf32_Ehdr *eh = (Elf32_Ehdr *)g_buf;
	// section header base
	Elf32_Shdr *sh_base = (Elf32_Shdr *)(g_buf + eh->e_shoff);
	// section string section header
	Elf32_Shdr *shstr_sh = sh_base + eh->e_shstrndx;	
	// symbol string section header
	Elf32_Shdr *str_sh = NULL;
	// symbol table section header
	Elf32_Shdr *sym_sh = NULL;	

	// get symbol table 
	for(int i = 0; i < eh->e_shnum; i ++) {
		Elf32_Shdr *sh = sh_base + i;
		if (sh->sh_type & SHT_SYMTAB) {
			sym_sh = sh;
			break;
		}
	}

	// check
	if (sym_sh == NULL) {
		printf("ELF-reader: Notice: there is no symbol table in '%s' \n", g_filename);
		return 0;
	}
	// check symbol entry size in the table
	if (sym_sh->sh_entsize != sizeof(Elf32_Sym)) {
		printf("ELF-reader: Error: symbol table entry size should be %ld but is %d \n",
			sizeof(Elf32_Sym), (unsigned int)sym_sh->sh_entsize);
			return 0;
	}

	Elf32_Sym *symbol_table = (Elf32_Sym *)(g_buf + sym_sh->sh_offset);
	Elf32_Sym *symbol = NULL;

	// prepare string tables
	str_sh = sh_base + sym_sh->sh_link;
	char *str_table = (char*)(g_buf + str_sh->sh_offset);
	char *shstr_table = (char*)(g_buf + shstr_sh->sh_offset);

	// print symbol table header
	const int symbol_cnt = sym_sh->sh_size / sizeof(Elf32_Sym);	
	printf("Symbol table '%s' contains %d entries: \n", shstr_table + sym_sh->sh_name, symbol_cnt);
	printf("%7.4s %8.8s %5.4s %-7.7s %-6.6s %-8.8s %3.3s %s\n", 
		"Num:", "Value", "Size", "Type", "Bind", "Vis", "Ndx", "Name");

	// print each symbol in symbol table
	static char *symbol_types[STT_NUM] = {"NOTYPE", "OBJECT", "FUNC", "SECTION", "FILE", "COMMON", "TLS"};
	static char *symbol_binds[STB_NUM] = {"LOCAL", "GLOBAL", "WEAK"};
	static char *symbol_vis[] = {"DEFAULT", "INTERNAL", "HIDDEN", "PROTECTED"};

	for (int i = 0; i < symbol_cnt; i ++) {
		symbol = symbol_table + i;

		unsigned int value = symbol->st_value;
		unsigned int size = symbol->st_size;
		char *type = symbol_types[ELF32_ST_TYPE(symbol->st_info)];
		char *bind = symbol_binds[ELF32_ST_BIND(symbol->st_info)];
		char *visibility = symbol_vis[ELF32_ST_VISIBILITY(symbol->st_other)];
		char *index = "";
		{
			unsigned int idx = symbol->st_shndx;
			if (idx >= SHN_LORESERVE && idx <= SHN_HIRESERVE) {
				if (idx == SHN_ABS) {
					index = "ABS";
				} else if (idx == SHN_COMMON) {
					index = "COM";
				} else {
					index = "BAD";
				}

			} else if (idx == SHN_UNDEF) { // = 0
				index = "UND";
			} else {
				char buf[3]; // enough
				snprintf(buf, 3, "%d", idx);
				index = buf;
			}
		}
		const char *name = str_table + symbol->st_name;
		
		printf("%6d: %08d %5d %-7s %-6s %-8s %3s %s \n",
			 i, value, size, type, bind ,visibility, index, name);
	}

	return 0;
}

int dummy_proc()
{
	return 0;
}

int unimplemented_proc()
{
	printf("ELF-reader: Fatal: unimplemented procedure for this option \n");
	return 0;
}