#ifndef OBB_H
#define OBB_H

#include <unzip.h>
#include <nme/Object.h>
#include <string>

namespace nme
{

class OBB: public Object
{
	public:
        static OBB* OpenOBB(const char* pathName);
        
		OBB() {
			_buff = NULL;
			_buffSize = 0;
		}
		bool SetCurrentFile(const char *fileName);
		int GetCurrentFileSize();
		unsigned char* GetDataFromCurrentFile(int length);
		
	private:
        unzFile _zipFile;
		std::string _currentFile;
		
		unsigned char* _buff;
		unsigned int _buffSize;
};

}

#endif