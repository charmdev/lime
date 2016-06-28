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

