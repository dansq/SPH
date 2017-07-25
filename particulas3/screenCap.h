#include <glad\glad.h>
#include <fstream>
#include <ios>

class screenCap{

public:
	void screenshot(char filename[160], int x, int y)
	{// get the image data
		long imageSize = x * y * 3;
		unsigned char *data = new unsigned char[imageSize];
		glReadPixels(0, 0, x, y, GL_BGR, GL_UNSIGNED_BYTE, data);// split x and y sizes into bytes
		int xa = x % 256;
		int xb = (x - xa) / 256; int ya = y % 256;
		int yb = (y - ya) / 256;//assemble the header
		unsigned char header[18] = { 0,0,2,0,0,0,0,0,0,0,0,0,(char)xa,(char)xb,(char)ya,(char)yb,24,0 };
		// write header and data to file
		std::fstream File(filename, std::ios::out | std::ios::binary);
		File.write(reinterpret_cast<char *>(header), sizeof(char) * 18);
		File.write(reinterpret_cast<char *>(data), sizeof(char)*imageSize);
		File.close();

		delete[] data;
		data = NULL;
	}

};