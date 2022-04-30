Pinger

C++ package manager:
  git clone https://github.com/microsoft/vcpkg
  bootstrap-vcpkg.bat

dependencies: 
	- boost-asio
		vcpkg install boost-asio:x64-windows
        vcpkg integrate install
	- libpqxx
		vcpkg install libpqxx:x64-windows
        vcpkg integrate install
    - wxwidgets
 	   vcpkg install wxwidgets:x64-windows
	   vcpkg integrate install
