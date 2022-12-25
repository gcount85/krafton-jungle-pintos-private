#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {

	
	// P2 arg passing: 파일명 파싱
	// totototodo
	char file_name[] = "String to tokenize.";
	char *token;
	char *save_ptr;
	char **parse = calloc(2, sizeof file_name);
	int argc = 0;

	for (token = strtok_r(file_name, " ", &save_ptr); token != NULL;
		 token = strtok_r(NULL, " ", &save_ptr))
	{
		parse[argc] = token;
		for (int j = 0; j < strlen(token); j++)
		{
			parse[argc][j] = token[j];
		}
		argc++;

		printf("'%s'\n", token);
		printf("%i\n", argc);
	}
	printf("%s\n", *parse);


}