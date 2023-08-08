#include<abstraction_create/abstraction_create.h>
#include <fstream>

project_path::project_path abstraction_create::Abs_create::set_project_path(std::string _file)
{

	ProjectPath.set_path(_file, "CMakeLists.txt");

	ProjectPath.print_path();

	return ProjectPath;
}


module_path::ModulePath abstraction_create::Abs_create::set_module_Path(std::string _file_path)
{

	ModulePath.set_path(_file_path, "CMakeLists.txt");

	ModulePath.print_path();

	return ModulePath;
}

project_path::project_path abstraction_create::Abs_create::get_project_path()
{
	return ProjectPath;
}

module_path::ModulePath abstraction_create::Abs_create::get_module_Path()
{
	return ModulePath;
}


bool abstraction_create::Abs_create::MkDir(const std::string& strPath)
{
	int i = 0;
	int nDirLen = strPath.length();
	if (nDirLen <= 0)
		return false;
	char* pDirTemp = new char[nDirLen + 4];
	strPath.copy(pDirTemp, nDirLen + 1, 0);// +1 to copy '\0'
	pDirTemp[nDirLen] = '\0';
	//��ĩβ��'/'
	if (pDirTemp[nDirLen - 1] != '\\' && pDirTemp[nDirLen - 1] != '/')
	{
		pDirTemp[nDirLen] = '/';
		pDirTemp[nDirLen + 1] = '\0';
		nDirLen++;
	}
	// ����Ŀ¼
	for (i = 0; i < nDirLen; i++)
	{
		if (pDirTemp[i] == '\\' || pDirTemp[i] == '/')
		{
			pDirTemp[i] = '\0';//�ضϺ������Ŀ¼���𼶲鿴Ŀ¼�Ƿ���ڣ����������򴴽�
			//���������,����
			int statu;
			statu = ACCESS(pDirTemp, 0);
			if (statu != 0)//���ܴ���ͬ���ļ�����û�д���
			{
				statu = MKDIR(pDirTemp);
				if (statu != 0)//�����ϼ������ļ��ж���ͬ���ļ����´���ʧ��
				{
					return false;
				}
			}
			//֧��linux,������\����/
			pDirTemp[i] = '/';
		}
	}
	delete[] pDirTemp;
	return true;
}
int abstraction_create::Abs_create::create_cmaketxt(std::string _cmakePath, std::string _txt)
{
	std::ofstream oFile;
	//���������½��ļ�
	oFile.open(_cmakePath, std::ios::app);
	if (!oFile)  //true��˵���ļ��򿪳���
		std::cout << "error 1" << std::endl;
	else
		oFile << _txt;

	oFile.close();

	return 0;

}