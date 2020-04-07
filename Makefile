all: Initialize SecondFileSys

Initialize:Initialize.cpp
	g++ -o  Initialize -m32 File.cpp OpenFileManager.cpp  Kernel.cpp FileSystem.cpp INode.cpp Utility.cpp  BufferManager.cpp FileManager.cpp  SystemCall.cpp Initialize.cpp -std=c++11

SecondFileSys: SecondFileSys.cpp
	g++ -o  SecondFileSys -m32 File.cpp OpenFileManager.cpp  Kernel.cpp FileSystem.cpp INode.cpp Utility.cpp  BufferManager.cpp FileManager.cpp SystemCall.cpp SecondFileSys.cpp -std=c++11


clean:
	rm SecondFileSys
	rm Initialize
	
