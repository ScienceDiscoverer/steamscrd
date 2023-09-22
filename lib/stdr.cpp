// SLIB
#include "stdr"

HANDLE StdReader::oh = GetStdHandle(STD_OUTPUT_HANDLE);
HANDLE StdReader::ih = GetStdHandle(STD_INPUT_HANDLE);
CONSOLE_SCREEN_BUFFER_INFO StdReader::csbi;

StdReader r;

ui64 StdReader::ReadNum(ui64 max_is)
{
	GetConsoleScreenBufferInfo(oh, &csbi);
	bool hex_mode = false;
	
	while(1)
	{
		ReadInput();
		
		DWORD ch_rmed = 0;
		if(inp[1] == 'x')
		{
			txtd(inp, 0, 2);
			hex_mode = true;
			ch_rmed = 2;
		}
		
		//txti(inp, 0, '[');
		//inp += ']';
		//DWORD d;
		//WriteFile(oh, inp, (DWORD)~inp, &d, NULL);
		
		if(
		~inp != 0						&&
		~inp <= max_is					&&
		txtf(inp, 0, 0, ',')	== NFND	&&
		txtf(inp, 0, '.', '/')	== NFND	&&
		txtf(inp, 0, ':', '@')	== NFND &&
		txtf(inp, 0, 'G', '`')	== NFND &&
		txtf(inp, 0, 'g', 0x7F)	== NFND )
		{
			if(~inp == max_is && NumOverflowed(inp, max_is))
			{
				goto retry_input;
			}
			
			break;
		}
		
	retry_input:
		
		SetConsoleCursorPosition(oh, csbi.dwCursorPosition);
		memset(inp, ' ', ~inp);
		WriteFile(oh, inp, (DWORD)~inp + ch_rmed, *inp, NULL);
		SetConsoleCursorPosition(oh, csbi.dwCursorPosition);
		
		memset(inp, 0, ~inp);
		txtssz(inp, 0);
	}
	
	ui64 ui;

	if(txtf(inp, 0, 'A', 'F') != NFND || txtf(inp, 0, 'a', 'f') != NFND || hex_mode)
	{
		ui = h2i(inp);
	}
	else
	{
		ui = t2i(inp);
	}
	
	memset(inp, 0, ~inp);
	txtssz(inp, 0);
	return ui;
}

bool StdReader::NumOverflowed(const txt &num, ui64 max_is)
{
	const char *mn = NULL;
	
	switch(max_is)
	{
	case 20:
		if(num[0] == '-')
		{
			mn = "-9223372036854775809";
		}
		else
		{
			mn = "18446744073709551616";
		}
		break;
	case 11:
		mn = "-2147483649";
		break;
	case 10:
		mn = "4294967296";
		break;
	case 6:
		mn = "-32769";
		break;
	case 5:
		mn = "65536";
		break;
	default:
		break;
	}
	
	const char *tb = num;
	--mn, --tb;
	while(*++mn != 0)
	{
		if(*++tb < *mn)
		{
			return false;
		}
	}
	
	return true;
}

// For testing:
//	p|"ih: "|ih|N;
//	p|"oh: "|oh|N;
//	
//	
//	p|0xFFFFFFFFFFFFFFFF|N;
//	
//	txt t;
//	r > t;
//	
//	p|I|t|I|N;
//	
//	while(1)
//	{
//		ui64 i;
//		r > i;
//		
//		p|I|i|I|S|I|(i64)i|I|N;
//	}