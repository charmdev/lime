#include <stdlib.h>
#include <unzip.h>
#include <OBB.h>
#include <sys/stat.h>

#ifdef HX_WINDOWS
#include <direct.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif
#include <Utils.h>

#define FILE_ATTRIBUTE_DIRECTORY 0x10

namespace nme
{
    
OBB* OBB::OpenOBB(const char* pathname)
{
	unzFile file = unzOpen(pathname);
	if(file != NULL)
	{
		OBB* obb = new OBB();
		obb -> _zipFile = file;
		
		bool error = !(obb -> GoToFirstFile());
		if(!error)
			do
			{
				unz_file_pos* pos = &(obb -> _map[ obb -> _currentFile ]);
				unzGetFilePos( obb -> _zipFile, pos);
				pos = &(obb -> _map[ obb -> _currentFile ]);
			}
			while ( obb -> GoToNextFile() );
		
		return obb;
	}

	return NULL;
}   
bool OBB::UnzipOBB(const char* pathname) //.zip
{
	/*
	std::string assetsDir(pathname);
	int dotPos = assetsDir.find_last_of('.');
	assetsDir = assetsDir.substr(0, dotPos);
	
	if(!rmdir(assetsDir.c_str()))
	{
		//__android_log_print(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "Remove directory error: %s", assetsDir.c_str());
		std::cout << "Remove directory error: " << assetsDir.c_str() << std::endl;
	}
	*/
	OBB* obb = new OBB();
	
	unzFile file = unzOpen(pathname);
	if(file != NULL)
	{		
		obb -> _zipFile = file;		
		if(!(obb -> GoToFirstFile()))
		{			
			//__android_log_write(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "GoToFirstFile error");
			std::cout << "GoToFirstFile error " << std::endl;
			return false;
		}
	}
	else 
	{		
		//__android_log_write(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "file == NULL");
		std::cout << "file == NULL" << std::endl;
		return false;
	}
	
	if (obb -> GetFileCount() == 0)
	{				
		//__android_log_write(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "GetFileCount() == 0");
		std::cout << "GetFileCount() == 0"<< std::endl;
		return false;	
	}
	
	do
	{
		//__android_log_print(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "unzipping file: %s", (obb->_currentFile).c_str());
		//std::cout << "unzipping file: " << obb->_currentFile << std::endl;
		if (!obb -> UnzipFile(pathname))
		{
			//__android_log_print(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "UnzipFile returned false for: %s", pathname);
			std::cout << "UnzipFile returned false for" << pathname<< std::endl<< std::endl;
			return false;
		}
	}
	while (obb -> GoToNextFile());
	
	return true;
}
bool OBB::UnzipFile(const char* zipPath)
{	
	if (! this -> _zipFile)
	{
		//__android_log_write(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "_zipFile == null");
		std::cout << "_zipFile == null"<< std::endl;
		return false;
	}

	std::string pathToFile(zipPath, strlen(zipPath));

	int slashPos = pathToFile.find_last_of('/');
	if(slashPos < 0)
		slashPos = pathToFile.find_last_of('\\');
	if(slashPos < 0)
	{
		//__android_log_write(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "Parsing file pathname error");
		std::cout << "Parsing file pathname error" << std::endl;		
		return false;
	}
	
	pathToFile = pathToFile.substr(0, slashPos+1);
	
	//__android_log_print(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "pathToFile: %s", pathToFile.c_str());
	//std::cout << "pathToFile: " << pathToFile << std::endl;	
	
	char szFileName[MAX_PATH + 1];
	char szComment[MAX_COMMENT + 1];
	
	unz_file_info* pfile_info = (unz_file_info*) _file_info;
	if (UNZ_OK != unzGetCurrentFileInfo(_zipFile, pfile_info, szFileName, MAX_PATH, NULL, 0, szComment, MAX_COMMENT))
	{
		//__android_log_write(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "unzGetCurrentFileInfo");
		std::cout << "unzGetCurrentFileInfo "<< std::endl;
		return false;
	}

	//__android_log_print(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "\nUnzipFile szFileName: %s", szFileName);
	//std::cout << "\nUnzipFile szFileName: " << szFileName << std::endl;
	
	pathToFile += szFileName;
	
	//__android_log_print(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "\nFull pathToFile: %s",  pathToFile.c_str());
	//std::cout << "Full pathToFile " << pathToFile.c_str() << std::endl;
	
	bool bFolder = ((pfile_info->external_fa & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
	
	// if the item is a folder then create it and return 'TRUE'
	if (bFolder)
	{				
		//__android_log_print(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "\nbFolder TRUE for : %s",  pathToFile.c_str());
		//std::cout << "bFolder TRUE for " << pathToFile.c_str() << std::endl;
		return CreateFolder(pathToFile.c_str()/*szFolderPath*/);
	}

	// open the input and output files
	if (!CreateFilePath(pathToFile.c_str()/*szFilePath*/))
	{
		//__android_log_print(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "CreateFilePath error for: %s", pathToFile.c_str());
		std::cout << "CreateFilePath error for " << pathToFile.c_str()/*szFilePath*/<< std::endl;
		return false;
	}

	FILE *hOutputFile = fopen(pathToFile.c_str(), "wb");
	if (!hOutputFile)
	{
		errno = 0;
		//__android_log_print(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "\nINVALID_HANDLE_VALUE for: %s WITH error: %d", pathToFile.c_str(), errno);
		std::cout << "INVALID_HANDLE_VALUE for "<< pathToFile.c_str()/*szFilePath*/ << std::endl;
		return false;
	}

	if (unzOpenCurrentFile(this -> _zipFile) != UNZ_OK)
	{
		//__android_log_write(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "unzOpenCurrentFile != UNZ_OK");
		std::cout << "unzOpenCurrentFile != UNZ_OK"<< std::endl;
		return false;
	}

	// read the file and output
	int nRet = UNZ_OK;
	char pBuffer[BUFFERSIZE];

	do
	{
		nRet = unzReadCurrentFile(this -> _zipFile, pBuffer, BUFFERSIZE);

		if (nRet > 0)
		{
			// output
			long dwBytesWritten = 0;
			dwBytesWritten = fwrite(pBuffer, 1, nRet, hOutputFile);
			if(dwBytesWritten != nRet)
			{
				//__android_log_print(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "WriteFile error: nRet, dwBytesWritten: %i, %i", nRet, dwBytesWritten);
				std::cout << "WriteFile error: nRet, dwBytesWritten: " << nRet << ", " << dwBytesWritten << std::endl;
				nRet = UNZ_ERRNO;
				break;
			}
		}
	}
	while (nRet > 0);

	fclose(hOutputFile);
	
	unzCloseCurrentFile(this -> _zipFile);

	//if (nRet == UNZ_OK)
	//	SetFileModTime(szFilePath, info.dwDosDate);

	return (nRet == UNZ_OK);
}
bool OBB::CreateFilePath(const char* szFilePath)
{
	bool bRes = false;
	
	std::string path(szFilePath);
	int slashPos = path.find_last_of('\\');
	if(slashPos < 0)
	{
		slashPos = path.find_last_of('/');
	}
	if(slashPos >= 0)
	{
		path = path.substr(0, slashPos);	
		bRes = CreateFolder(path.c_str());
	}
	return bRes;
}
bool OBB::CreateFolder(const char* szFolder)
{
	if (!szFolder || !strlen(szFolder))
	{
		//__android_log_write(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "CreateFolder check error");
		std::cout << "CreateFolder check error" << szFolder<< std::endl;
		return false;
	}

	struct stat st;
    bool isExist = (stat(szFolder,&st) == 0);
	if(isExist)
	{
		//std::cout << "exists, isDir = " << ((st.st_mode & S_IFDIR) == S_IFDIR) << std::endl;
		return ((st.st_mode & S_IFDIR) == S_IFDIR); //is directory
	}

	// recursively create from the top down
	std::string path(szFolder);
	int slashPos = path.find_last_of('\\');
	if(slashPos < 0)
	{
		slashPos = path.find_last_of('/');
	}
	if(slashPos >= 0)
	{
		path = path.substr(0, slashPos);	
	
		// if can't create parent		
		if (!CreateFolder(path.c_str()))
		{
			//__android_log_write(ANDROID_LOG_VERBOSE, "trace OBB.cpp", "can't create parent");
			std::cout << "can't create parent"<< std::endl;

			return false;
		}
		
		#ifdef HX_WINDOWS
			mkdir(szFolder);
		#else
			mkdir(szFolder, 493);
		#endif
	}
	
	return true;
}
int OBB::GetFileCount()
{
	if (!_zipFile)
		return 0;

	unz_global_info info;

	if (unzGetGlobalInfo(_zipFile, &info) == UNZ_OK)
	{
		return (int)info.number_entry;
	}

	return 0;
}
bool OBB::GoToFirstFile()
{
	int result = unzGoToFirstFile(_zipFile);
	if(result == UNZ_OK)
	{
		syncCurrentFile();
		return true;
	}
	
	return false;
}

bool OBB::GoToNextFile()
{
	int result = unzGoToNextFile(_zipFile);
	if(result == UNZ_OK)
	{
		syncCurrentFile();
		return true;
	}
	
	return false;
}

bool OBB::SetCurrentFile(const char* fileName)
{
	if(_currentFile.compare(fileName) == 0)
		return true;
	
	auto search = _map.find( fileName );
	if( search != _map.end() )
	{
		unz_file_pos* pos = &(search->second);
		int result = unzGoToFilePos( _zipFile, pos);
		if(result == UNZ_OK)
		{
			_currentFile.assign(fileName);
			return true;
		}
	}
	
	
	return false;
} 

int OBB::GetCurrentFileSize()
{
	int size = -1;
	
	if( _file_info == NULL )
		_file_info = (unsigned char*) malloc(sizeof(unz_file_info));
	
	unz_file_info* pfile_info = (unz_file_info*) _file_info;
	int result = unzGetCurrentFileInfo(_zipFile, pfile_info, NULL, 0, NULL, 0, NULL, 0);
	if(result == UNZ_OK)
		size = (int) pfile_info->uncompressed_size;
		
	return size;
}

unsigned char* OBB::GetDataFromCurrentFile(int length)
{
	int result = unzOpenCurrentFile2(_zipFile, NULL, NULL, 1);
	if(result == UNZ_OK)
	{
		if(_buffSize < length)
		{
			if( _buff )
				free(_buff);
				
			_buff = (unsigned char*) malloc( sizeof(unsigned char) * length );
			_buffSize = length;
		}
		
		unzReadCurrentFile(_zipFile, _buff, length);
		unzCloseCurrentFile(_zipFile);
		
		return _buff;
	}
	
	return NULL;
}

void OBB::syncCurrentFile()
{
	if( !_file_info )
		_file_info = (unsigned char*) malloc(sizeof(unz_file_info));
		
	unzGetCurrentFileInfo(_zipFile, (unz_file_info*) _file_info, _file_name, _file_name_length, NULL, 0, NULL, 0);
	_currentFile.assign( (const char*) _file_name);
}

}

