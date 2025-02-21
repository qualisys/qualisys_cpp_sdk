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

### Applied the following patch to hide leaking consts

```
diff --git a/External/tinyxml2/tinyxml2.cpp b/External/tinyxml2/tinyxml2.cpp
index 083f54b..db7b53b 100644
--- a/External/tinyxml2/tinyxml2.cpp
+++ b/External/tinyxml2/tinyxml2.cpp
@@ -119,6 +119,8 @@ distribution.
        #define TIXML_FTELL ftell
 #endif

+namespace
+{

 static const char LINE_FEED                            = static_cast<char>(0x0a);                      // all line endings are normalized to LF
 static const char LF = LINE_FEED;
@@ -135,6 +137,8 @@ static const unsigned char TIXML_UTF_LEAD_0 = 0xefU;
 static const unsigned char TIXML_UTF_LEAD_1 = 0xbbU;
 static const unsigned char TIXML_UTF_LEAD_2 = 0xbfU;

+}
+
 namespace tinyxml2
 {
```
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
