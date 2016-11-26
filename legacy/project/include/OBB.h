#ifndef OBB_H
#define OBB_H

#include <unzip.h>
#include <nme/Object.h>
#include <string>
#include <map>
#include <iostream>


namespace nme
{
	const unsigned int MAX_COMMENT = 255;
	const unsigned int BUFFERSIZE = 2048;
	const unsigned int MAX_PATH = 4096;
	const unsigned int MAX_DRIVE = 3;
	const unsigned int MAX_DIR = 256;
	const unsigned int MAX_FNAME = 256;
	const unsigned int MAX_EXT = 256;

class OBB: public Object
{
	public:
        static OBB* OpenOBB(const char* pathName);
        static bool UnzipOBB(const char* pathName);
		
		OBB() {
			_buff = NULL;
			_buffSize = 0;
			
			_file_info = NULL;
			
			_file_name_length = sizeof(char) * 500;
			_file_name = (char*) malloc(_file_name_length);
		}
		bool GoToFirstFile();
		bool GoToNextFile();
		bool SetCurrentFile(const char *fileName);
		int GetCurrentFileSize();
		unsigned char* GetDataFromCurrentFile(int length);
		bool UnzipFile(const char* pathname);
		int GetFileCount();
		bool CreateFilePath(const char* szFilePath);
		bool CreateFolder(const char* szFolder);
	private:
        std::map<std::string, unz_file_pos> _map;
		
		unzFile _zipFile;
		std::string _currentFile;
		
		unsigned char* _file_info;
		char* _file_name;
		long _file_name_length;
		
		unsigned char* _buff;
		unsigned int _buffSize;
		
		void syncCurrentFile();
};

}

#endif