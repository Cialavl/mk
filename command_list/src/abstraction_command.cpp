#include<abstraction_command/abstraction_command.h>

int AbsCommand::Abs_Command::run_cmd(const char* cmd)
{
	char MsgBuff[1024];
	int MsgLen = 1020;
	FILE* fp;
	if (cmd == NULL)
	{
		return -1;
	}
	if ((fp = _popen(cmd, "r")) == NULL)
	{
		return -2;
	}
	else
	{
		memset(MsgBuff, 0, MsgLen);

		//��ȡ����ִ�й����е����
		while (fgets(MsgBuff, MsgLen, fp) != NULL)
		{
			printf("MsgBuff: %s\n", MsgBuff);
		}

		//�ر�ִ�еĽ���
		if (_pclose(fp) == -1)
		{
			return -3;
		}
	}
	return 0;
}
