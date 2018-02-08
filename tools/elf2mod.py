import struct

def pack_struct(struct_spec, data):
	fmt_str = "<"
	paramters = []
	for name, fmt in struct_spec:
		fmt_str += fmt
		paramters.append(data.get(name, 0))
	return struct.pack(fmt_str, *paramters)

struct_mod_header = (
	("mod_magic",   "I"),
	("dyn_off",     "I"),
	("bss_off",     "I"),
	("bss_end",     "I"),
	("unwind_off",  "I"),
	("unwind_end",  "I"),
	("mod_object",  "I"),
)

struct_dyn_entry = (
	("tag",         "Q"),
	("value",       "Q"),
)

dynamic_entry_type = {
	"DT_NULL": 0x0,
	"DT_NEEDED": 0x1,
	"DT_PLTRELSZ": 0x2,
	"DT_PLTGOT": 0x3,
	"DT_HASH": 0x4,
	"DT_STRTAB": 0x5,
	"DT_SYMTAB": 0x6,
	"DT_RELA": 0x7,
	"DT_RELASZ": 0x8,
	"DT_RELAENT": 0x9,
	"DT_STRSZ": 0xa,
	"DT_SYMENT": 0xb,
	"DT_INIT": 0xc,
	"DT_FINI": 0xd,
	"DT_SONAME": 0xe,
	"DT_RPATH": 0xf,
	"DT_SYMBOLIC": 0x10,
	"DT_REL": 0x11,
	"DT_RELSZ": 0x12,
	"DT_RELENT": 0x13,
	"DT_PLTREL": 0x14,
	"DT_DEBUG": 0x15,
	"DT_TEXTREL": 0x16,
	"DT_JMPREL": 0x17,
	"DT_BIND_NOW": 0x18,
	"DT_INIT_ARRAY": 0x19,
	"DT_FINI_ARRAY": 0x1a,
	"DT_INIT_ARRAYSZ": 0x1b,
	"DT_FINI_ARRAYSZ": 0x1c,
	"DT_GNU_HASH": 0x6ffffef5,
	"DT_RELACOUNT": 0x6ffffff9,
	"DT_LOPROC": 0x70000000,
	"DT_HIPROC": 0x7fffffff
}

def generate_mod(elffile, nro_size):
	dynamic = elffile.get_section_by_name('.dynamic')
	
	dynamic_data = b""
	
	for dynamic_entry in dynamic.iter_tags():
		value = {"tag": dynamic_entry_type.get(dynamic_entry.entry['d_tag'], 0),
			"value": dynamic_entry.entry['d_val']}
		dynamic_data += pack_struct(struct_dyn_entry, value)
	
	dyn_off = 0x100
	
	total_size = dyn_off + len(dynamic_data) + nro_size
	padding_size = 0x1000 - ((dyn_off + len(dynamic_data) + nro_size) % 0x1000)
	
	mod_header = {
		"mod_magic": 0x30444f4d,
		"dyn_off": dyn_off,
		"bss_off": dyn_off + len(dynamic_data),
		"bss_end": total_size,
		"mod_object": dyn_off + len(dynamic_data),
	}
	
	mod = pack_struct(struct_mod_header, mod_header)
	mod += b"\x00" * (dyn_off - len(mod))
	mod += dynamic_data
	mod += b"\x00" * padding_size
	
	return mod
