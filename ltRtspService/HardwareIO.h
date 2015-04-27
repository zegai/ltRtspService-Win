#include "MediaBuffer.h"
#include <iostream>
#include <map>
class HardwareIO
{
public:
	static HardwareIO* GetInstance();
	int MakeFileNode(std::string& filename);
	buf_share_ptr GetBufferFormFile(int index, uint64_t pos, unsigned sizeget);
protected:
	typedef struct _FileNode
	{
		std::string _filename;
		FILE*  _filefd;
		uint64_t _curpos;
		~_FileNode(){fclose(_filefd);}
	}FileNode;
	
	HardwareIO();
	~HardwareIO();
private:
	std::map<int, FileNode*> FileManager;
};