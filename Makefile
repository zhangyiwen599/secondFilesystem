Initialize:Initialize.cpp
	g++ -o Initialize File.cpp OpenFileManager.cpp  Kernel.cpp FileSystem.cpp INode.cpp Utility.cpp  BufferManager.cpp  Initialize.cpp -std=c++11

SecondFileSys: SecondFileSys.cpp
	g++ -o SecondFileSys File.cpp OpenFileManager.cpp  Kernel.cpp FileSystem.cpp INode.cpp Utility.cpp  BufferManager.cpp FileManager.cpp SecondFileSys.cpp -std=c++11


clean:
	rm SecondFileSys
	rm Initialize
	
