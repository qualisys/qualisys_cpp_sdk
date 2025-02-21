# TinyXML-2
## Source
- Repository: [TinyXML2 GitHub](https://github.com/leethomason/tinyxml2)  
- Version: 10.0.0  
  
## Modification
TinyXML2 requires only two files for compilation without tests:  
- `tinyxml2.cpp`  
- `tinyxml2.h`  

The following files have been copied to `External/tinyxml2`:  
- `tinyxml2.cpp`  
- `tinyxml2.h`  
- `LICENSE.txt`  
- `readme.txt`  

## Usage
`tinyxml2` is referenced by source and built together with the target.

### Used by:
- qualisys_cpp_sdk
- qualisys_cpp_sdk_tests

## Updating to a Newer Version  
To update TinyXML2, download the latest `tinyxml2.cpp` and `tinyxml2.h` from  
[GitHub Releases](https://github.com/leethomason/tinyxml2/releases) and replace  
the existing files in `External/tinyxml2`. Ensure that no breaking changes affect  
dependent projects.  

## License  
The `LICENSE.txt` file remains unchanged and must be included to comply with TinyXML2’s licensing terms. 
