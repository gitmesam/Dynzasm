#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "arch/x86/x86.h"
#include "arch/x86/x86load.h"
#include "common/trie.h"
#include "common/db.h"
#include "common/disk.h"
#include "common/ser.h"

void disassemble(struct trie_node *root, unsigned char *out, long max)
{
	long bit = 0;
	long addr = 0;

	while (bit < max) {
		printf("%#08lx: ", addr);
		struct dis disas;
		int oldbit = bit;
		bit+=x86_disassemble(root, out+bit, max-bit, &disas);
		addr = bit;
		int w = 20;
		for (int i = oldbit; w>=2 && i < bit; i++) {
			printf("%02x", out[i]);
			w-=2;
		}
		while (w-->0) printf(" ");
		char buf[256];
		w = 7;
		printf("%s ", disas.mnemonic);
		w -= strlen(disas.mnemonic);
		while (w-- > 0) printf(" ");
		for (int i = 0; i < disas.num_operands; i++) {
			operand_squash(buf, 256, disas.operands[i]);
			printf("%s%c ", buf, (i+1)<disas.num_operands?',':' ');
			operand_tree_free(disas.operands[i]);
		}
		free(disas.operands);
		printf("\n");
	}
}

void disas_stdin(struct trie_node *root)
{
	char ch;
	long allocd=100;
	char *bbuf = malloc(allocd);
	int iter = 0;
	while (read(STDIN_FILENO, &ch, 1) > 0) {
		if ((iter + 1) >= allocd) {
			allocd += 100;
			bbuf = realloc(bbuf, allocd);
			memset(bbuf+iter, 0, allocd-iter-1);
		}
		if ((ch>='a'&&ch<='f') || (ch >= 0x30 && ch <= 0x39)) {
			bbuf[iter++]=ch;
		}
	}
	int maxout = iter/2 + 1;
	unsigned char *out = malloc(maxout);
	memset(out, 0, maxout);
	long max = ascii_to_hex(out, bbuf, iter);

	disassemble(root, out, max);

	free(bbuf);
	free(out);
}

int main(int argc, char ** argv)
{
	struct trie_node *root = trie_init(0, NULL);
	x86_parse(root);

	if (argc < 2) {
		disas_stdin(root);
	} else {
		FILE *fp = fopen(argv[1], "r");
		if (fp) {
			fseek(fp, 0, SEEK_END);
			long size = ftell(fp);
			if (size > 0) {
				rewind(fp);
				unsigned char *buffer = malloc(size+1);
				memset(buffer, 0, size);
				if (fread(buffer, 1, size, fp) != (unsigned long)size) {
					printf("Error reading bytes from file\n");
					exit(1);
				}
				disassemble(root, buffer, size);
				free(buffer);
			} else {
				printf("Error ftell returns negative number\n");
				exit(1);
			}
		}
	}
	trie_destroy(root);
	return 0;
}
