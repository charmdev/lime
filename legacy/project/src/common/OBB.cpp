#include <stdlib.h>
#include <unzip.h>
#include <OBB.h>

namespace nme
{
    
OBB* OBB::OpenOBB(const char* pathname)
{
	unzFile file = unzOpen(pathname);
	if(file != NULL)
	{
		OBB* obb = new OBB();
		obb -> _zipFile = file;

		return obb;
	}

	return NULL;
}   
  
bool OBB::SetCurrentFile(const char* fileName)
{
	if(_currentFile.compare(fileName) == 0)
		return true;
	
	int result = unzLocateFile(_zipFile, fileName, 1);
	if(result == UNZ_OK)
	{
		_currentFile.assign(fileName);
		return true;
	}
	
	return false;
} 

int OBB::GetCurrentFileSize()
{
	int size = -1;
	
	unz_file_info* pfile_info = (unz_file_info*) malloc(sizeof(unz_file_info));
	int result = unzGetCurrentFileInfo(_zipFile, pfile_info, NULL, 0, NULL, 0, NULL, 0);
	if(result == UNZ_OK)
		size = (int) pfile_info->uncompressed_size;
		
	free(pfile_info);
	
	return size;
}

unsigned char* OBB::GetDataFromCurrentFile(int length)
{
	int result = unzOpenCurrentFile2(_zipFile, NULL, NULL, 1);
	if(result == UNZ_OK)
	{
		if(_buffSize < length)
		{
			if(_buff != NULL)
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

}

