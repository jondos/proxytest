#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#if defined(_WIN32)
#include <io.h>
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
		_write(file, in, strlen(in));
		_write(file, "\n", 1);
		if (in[0] == 'c')
			break;
	}
	_close(file);
    return 0;
}

