:: Set paths for the "include" and "lib" directories of the toolchain and the
:: Windows SDK. Note that "link" has a dependency on "common7\ide\mspdb*.dll".

set PATH=c:\bin\msvs\vc\bin;c:\program files\microsoft sdks\windows\v7.1\bin;c:\bin\msvs\common7\ide;%PATH%
set INCLUDE=c:\bin\msvs\vc\include;c:\program files\microsoft sdks\windows\v7.1\include;%INCLUDE%
set LIB=c:\bin\msvs\vc\lib;c:\program files\microsoft sdks\windows\v7.1\lib;%LIB%
