#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#if defined(_WIN32)
#include <io.h>
#define myclose _close
#define myopen _open
#define mywrite _write
#else
#include <unistd.h>
#define myclose close
#define myopen open
#define mywrite write
#endif // 


int main()
{
	char* in = new char[0xFFFF];
	int file;
	_sopen_s(&file, "test.log", O_APPEND | O_CREAT | _O_WRONLY | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
	for (;;)
	{
		scanf_s("%s",in,0xFFFF);
		printf("%s\n", in);
		mywrite(file, in, strlen(in));
		mywrite(file, "\n", 1);
		if (in[0] == 'c')
			break;
	}
	myclose(file);
    return 0;
}

