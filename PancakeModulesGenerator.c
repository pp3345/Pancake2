#include <stdio.h>

int main(int argc, char *argv[]) {
	unsigned int i;
	FILE *modulesFile = fopen(argv[1], "w");

	if(!modulesFile) {
		printf("Unable to open module file %s", argv[1]);
		return 1;
	}

	fprintf(modulesFile, "#include <Pancake.h>\n");

	for(i = 0; i < (int) ((argc - 2) / 2); i++) {
		fprintf(modulesFile, "#include \"%s\"\n", argv[i + 2]);
	}

	fprintf(modulesFile, "\nPancakeModule *PancakeModules[%i] = {\n", (int) ((argc - 2) / 2 + 1));
	for(i = 0; i < (int) ((argc - 2) / 2); i++) {
		fprintf(modulesFile, "&%s,\n", argv[i + 2 + (int) ((argc - 2) / 2)]);
	}

	fprintf(modulesFile, "NULL};\n");
	fclose(modulesFile);

	return 0;
}
