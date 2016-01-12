// baidupushsdk.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "baidupush.h"



int _tmain(int argc, _TCHAR* argv[])
{
	baidupush_init();
	while(1){
		baidupush_android_signle("4523264398674826448","This is tittle","This is mesage.");
		//baidupush_ios_signle("4523264398674826448","This is tittle","This is mesage.");
		Sleep(10000);
	}
	return 0;
}

