// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>

// int main() {

// 	// P2 arg passing: 파일명 파싱
// 	// totototodo
// 	char file_name[] = "String to tokenize.";
// 	char *token;
// 	char *save_ptr;
// 	char **parse = calloc(2, sizeof file_name);
// 	int argc = 0;

// 	for (token = strtok_r(file_name, " ", &save_ptr); token != NULL;
// 		 token = strtok_r(NULL, " ", &save_ptr))
// 	{
// 		parse[argc] = token;
// 		for (int j = 0; j < strlen(token); j++)
// 		{
// 			parse[argc][j] = token[j];
// 		}
// 		argc++;

// 		printf("%s\n", token);
// 		printf("%i\n", argc);
// 	}
// 	printf("%s\n", *parse);

// }

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main()
{
	// P2 arg passing: 파일명 파싱
	char *file_name = "string to tokenize abara cadabra aleehandd.";
	char *token;
	char *save_ptr;
	char *argv[64]; // char형 포인터는 string으로 받는다
	int argc = 0;

	token = strtok_r(file_name, " ", &save_ptr);
	while (token)
	{
		argv[argc++] = token;
		token = strtok_r(NULL, " ", &save_ptr);
	}
}

// 재형
/* parsing the *f_name*/
int argc = 0;
char *argv[100]; /*몇개정도 세팅해야 할까?*/
char *ret_ptr, *next_ptr;

ret_ptr = strtok_r(copy_name, " ", &next_ptr);
char *token_name = ret_ptr;
argv[argc] = ret_ptr;
while (ret_ptr != NULL)
{
	ret_ptr = strtok_r(NULL, " ", &next_ptr);
	argc += 1;
	argv[argc] = ret_ptr;
}
/*출처: https://www.it-note.kr/86*/

// 동훈

int main()
{
	// P2 arg passing: 파일명 파싱
	char *file_name;
	char str[100] = "string to tokenize abara cadabra aleehandd.";
	file_name = str;
	char *token = NULL;
	char *save_ptr = NULL;
	char *argv[64]; // char형 포인터는 string으로 받는다
	int argc = 0;

	token = strtok_r(file_name, " ", &save_ptr);
	while (token)
	{
		argv[argc++] = token;
		token = strtok_r(NULL, " ", &save_ptr);
		printf("%s\n", token);
	}
}