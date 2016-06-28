#ifndef OBB_H
#define OBB_H

#include <stdlib.h>
#include <unzip.h>
#include <nme/Object.h>
#include <string>
#include <map>

namespace nme
{

class OBB: public Object
{
	public:
        static OBB* OpenOBB(const char* pathName);
        
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