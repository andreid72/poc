#!/usr/bin/python

import sys

tmpl_fname = sys.argv[1]
out_fname = sys.argv[2]
nstores = int(sys.argv[3])
nnoops = int(sys.argv[4])

def write_asm_body(out) :
	out.write("        uint64_t array[" + str(nstores) + "];\n");
	out.write("        asm volatile (\n");
	i = 0
	while (i < nstores):
		line = "            "
		line += "\"movq $" + str(i) + ", "
		line += str(i * 8) + "(%[array])\\n\"\n"
		out.write(line)
		i += 1
	i = 0
	while (i < nnoops):
		line = "            "
		line += "\"nop\\n\"\n"
		out.write(line)
		i += 1
	out.write("            : // No output\n")
	out.write("            : [array] \"r\" (array)\n")
	out.write("            : \"memory\");\n")


with open(tmpl_fname, 'r') as f:
	tmpl = f.readlines()

out = open(out_fname, 'w');

for l in tmpl:
	if "asm_code" in l:
		write_asm_body(out)
	elif "count_init" in l:
		out.write("    int store_cnt = " + str(nstores) + ";\n")
	else:
		out.write(l)