Pinger

dependencies: 
	- boost_1_79_0
	- libpqxx
	    Install vcpkg by running the below commands in cmd from a folder called C:\src (to avoid issues. Create folder if necessary):
   		 git clone https://github.com/microsoft/vcpkg
		.\vcpkg\bootstrap-vcpkg.bat
	       Install libpqxx by entering in cmd from the same folder:
		    .\vcpkg\vcpkg install libpqxx:x64-windows
		    .\vcpkg\vcpkg integrate install
		    .\vcpkg\vcpkg install libpqxx

