# Note: When using meson "vctip.exe" seems to be started using some meson dll, which prevents the automatic cleanup by gitlab. Can be worked around by renaming "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64\vctip.exe" to "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64\old_vctip.exe"

# TODO: Add vcruntime140_1.dll needed for scikit-image 0.18.1? (Currently windows uses version 0.17.2 because of this)

set ErrorActionPreference Stop
Set-StrictMode -Version 2.0

$DoVoxieBuild = 1

function CheckErrorCode {
  if (-Not $?) {
    throw "Command execution failed: " + $args
  }
}

function RMrf {
  ForEach ($arg in $args) {
    if (Test-Path $arg) {
      rmdir -recurse -force $arg
      CheckErrorCode
    }
  }
}

function External {
  $args2 = $($args | % {$_})
  $par = $args2[1..($args2.Length-1)]
  &$args2[0] $par
  CheckErrorCode ($args2 -join ' ')
}

# Hack to execute code with .NET 4
# https://blog.stangroome.com/2011/03/23/executing-individual-powershell-commands-using-net-4/
# https://gist.github.com/jstangroome/882528
function Invoke-CLR4PowerShellCommand {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory=$true)]
        [ScriptBlock]
        $ScriptBlock,
        [Parameter(ValueFromRemainingArguments=$true)]
        [Alias('Args')]
        [object[]]
        $ArgumentList
    )
    if ($PSVersionTable.CLRVersion.Major -eq 4) {
        Invoke-Command -ScriptBlock $ScriptBlock -ArgumentList $ArgumentList
        return
    }
    $RunActivationConfigPath = $Env:TEMP | Join-Path -ChildPath ([Guid]::NewGuid())
    New-Item -Path $RunActivationConfigPath -ItemType Container | Out-Null
@"
<?xml version="1.0" encoding="utf-8" ?>
<configuration>
  <startup useLegacyV2RuntimeActivationPolicy="true">
    <supportedRuntime version="v4.0"/>
  </startup>
</configuration>
"@ | Set-Content -Path $RunActivationConfigPath\powershell.exe.activation_config -Encoding UTF8
    $EnvVarName = 'COMPLUS_ApplicationMigrationRuntimeActivationConfigPath'
    $EnvVarOld = [Environment]::GetEnvironmentVariable($EnvVarName)
    [Environment]::SetEnvironmentVariable($EnvVarName, $RunActivationConfigPath)
    try {
        & powershell.exe -inputformat text -outputformat text -command $ScriptBlock -args $ArgumentList
        CheckErrorCode
    } finally {
        [Environment]::SetEnvironmentVariable($EnvVarName, $EnvVarOld)
        $RunActivationConfigPath | Remove-Item -Recurse
    }
}

$global_args = $args
function CheckEnvVar ([string]$name) {
  ForEach ($arg in $global_args) {
    $arg = ($arg -as [string])
    if ($arg.StartsWith($name + "=")) {
      [Environment]::SetEnvironmentVariable("VOXIEBUILD_" + $name, $arg.Substring(($name + "=").Length))
    }
  }
  $val = [Environment]::GetEnvironmentVariable("VOXIEBUILD_" + $name)
  if (($val -eq $null) -or ($val -eq "")) {
    throw "Environment variable " + $name + " not found"
  }
}

function AddCr([string]$source, [string]$target) {
  $text = [IO.File]::ReadAllText($source)
  $text = $text -replace "`r", ""
  $text = $text -replace "`n", "`r`n"
  [IO.File]::WriteAllText($target, $text)
}

# http://stackoverflow.com/questions/2124753/how-i-can-use-powershell-with-the-visual-studio-command-prompt
function SetVsCmd {
    pushd $env:VOXIEBUILD_PATH_VISUAL_STUDIO_VC
    cmd /c "vcvarsall.bat x64&set" |
    foreach {
      if ($_ -match "(.*?)=(.*)") {
        Set-Item -force -path "ENV:\$($matches[1])" -value "$($matches[2])"
      }
    }
    popd
    CheckErrorCode
}


$URL_BASE = $env:VOXIEBUILD_URL_BASE
ForEach ($arg in $args) {
  $arg = ($arg -as [string])
  if ($arg.StartsWith("URL_BASE=")) {
    $URL_BASE=$arg.Substring("URL_BASE=".Length)
  }
}

function GetUrl {
  $file = $args[0]
  $url = [IO.File]::ReadAllText("tools/build-dep/$file.url").Trim()
  return $url
}


# $VERSION_PYTHON = "3.5.3"
# $VERSION_PYTHON_SHORT = "cp35"

$VERSION_PYTHON = "3.7.9"
$VERSION_PYTHON_SHORT = "cp37"

$files = @(
  "ICSharpCode.SharpZipLib.dll",
  "7za920.zip",
  "lessmsi-v1.6.1.zip",
  # Note: Meson 0.56.2 seems to fail with the error message: '..\src\meson.build:1:0: ERROR: Requested C runtime based on buildtype, but buildtype is "custom".'
  # Seems to be fixed by using an optimization level of 3, see commit aeb5982d207035f62ba7ad8fc3d1e78e641eedb5
  #"meson-0.56.2-64.msi",
  "meson-0.57.0-64.msi",
  "boost_1_61_0.zip",

  "lapack/3.7.0-win64/liblapacke.lib",
  "lapack/3.7.0-win64/libblas.dll",
  "lapack/3.7.0-win64/liblapack.dll",
  "lapack/3.7.0-win64/liblapacke.dll",
  "lapack/3.7.0-win64/libtmglib.dll",

  "cmake-2.8.12.2-win32-x86.zip"
)
$files_lic = @(
  "hdf5-1.10.0-patch1-win64-vs2015-shared.zip",
  "expat-2.1.0.tar.gz",
  "dbus-1.8.2.tar.gz",

  #"LAPACKE_examples.zip",
  #"lapack-3.5.0.tgz",
  #"LAPACKE-3.5.0.lib",
  "lapack-3.7.0.tgz",

  "python-$VERSION_PYTHON-embed-amd64.zip"
)
$files_lic_only = @(
  "qt-everywhere-opensource-src-5.6.3.tar.xz"
)
$files = $files + $files_lic
$files_lic = $files_lic + $files_lic_only

$URL_EXPAT_SRC = GetUrl("expat-2.1.0.tar.gz")
$URL_DBUS_SRC = GetUrl("dbus-1.8.2.tar.gz")
$URL_HDF5 = GetUrl("hdf5-1.10.0-patch1.tar.bz2")
$URL_QT = GetUrl("qt-everywhere-opensource-src-5.6.3.tar.xz")

$URL_PYTHON_SRC = GetUrl("Python-$VERSION_PYTHON.tgz")

echo "Checking build dependencies..."
$algorithm = [Security.Cryptography.HashAlgorithm]::Create("SHA512")
ForEach ($file in $files) {
  #echo $file
  $expSum = [IO.File]::ReadAllText("tools/build-dep/$file.sha512sum").Split(" ")[0]
  if ($expSum.Length -ne 128) {
    throw "Checksum in tools/build-dep/$file.sha512sum has length $($expSum.Length)"
  }
  if (Test-Path "tools/build-dep/$file") {
    $fileBytes = [IO.File]::ReadAllBytes("tools/build-dep/$file")
    $bytes = $algorithm.ComputeHash($fileBytes)
    $sum = -Join ($bytes | ForEach {"{0:x2}" -f $_})
    if ($sum -ne $expSum) {
      throw "Checksum for existing file tools/build-dep/$file does not match: Expected $expSum, got $sum"
    }
  } else {
    if ( $URL_BASE -eq "" -or $URL_BASE -eq $null ) {
      throw "URL_BASE not set and file $file not found"
    }
    Write-Host "Getting $file..." -NoNewLine
    if ( $URL_BASE -eq "origin" ) {
      $url = [IO.File]::ReadAllText("tools/build-dep/$file.url").Trim()
    } else {
      $url = "$URL_BASE/$file"
    }
    (New-Object System.Net.WebClient).DownloadFile($url, "tools/build-dep/$file.new")
    Write-Host " done"
    $fileBytes = [IO.File]::ReadAllBytes("tools/build-dep/$file.new")
    $bytes = $algorithm.ComputeHash($fileBytes)
    $sum = -Join ($bytes | ForEach {"{0:x2}" -f $_})
    if ($sum -ne $expSum) {
      throw "Checksum for downloaded file tools/build-dep/$file does not match: Expected $expSum, got $sum"
    }
    [IO.File]::Move("tools/build-dep/$file.new", "tools/build-dep/$file")
  }
}
echo "Done checking build dependencies."


echo "Checking environment variables..."

# Visual Studio
# https://www.visualstudio.com/downloads/download-visual-studio-vs (Visual Studio Community 2015 with Update 2)
CheckEnvVar PATH_VISUAL_STUDIO_VC

# Qt
# http://download.qt.io/archive/qt/5.6/5.6.3/qt-opensource-windows-x86-msvc2015_64-5.6.3.exe
CheckEnvVar PATH_QT
CheckEnvVar URL_QT_COPY

CheckEnvVar USE_GIT

echo "Cleaning up build directory..."

RMrf build

rm voxie*.zip

echo "Checking git config..."

if ($env:VOXIEBUILD_USE_GIT -ne "0") {
  $AutoCrlf = (External git config core.autocrlf) | Out-String
  if ($AutoCrlf -eq $null) {
    $AutoCrlf = ""
  }
  
  #echo $AutoCrlf
  if ($AutoCrlf.Trim() -ne "false") {
    throw "core.autocrlf is not set to false"
  }
  
  #$TARGET_NAME = "voxie"
  # $REV = git describe --always
  # CheckErrorCode
  # $REV = $REV.Trim()
  # if ($REV.StartsWith("voxie-")) {
  #     $REV = $REV.Substring(6);
  # }

  $VOXIE_REF = git rev-parse HEAD
  CheckErrorCode
  $VOXIE_REF_SHORT = git rev-parse --short=10 $VOXIE_REF
  CheckErrorCode
} else {
  $VOXIE_REF = "unknown"
  $VOXIE_REF_SHORT = "unknown"
}

# $VOXIE_TAG = $env:CI_BUILD_REF
# if (($env:CI_BUILD_TAG -ne $null) -and ($env:CI_BUILD_TAG -ne "")) {
#     $VOXIE_TAG = $env:CI_BUILD_TAG
# }

$VOXIE_TAG = $VOXIE_REF

$VOXIE_VERSION = [IO.File]::ReadAllText("src/version.txt").Trim() -replace "`"", ""
$VOXIE_VERSION_SUFFIX = ""
if (Test-Path "intern/version-suffix.txt") {
  $VOXIE_VERSION_SUFFIX = [IO.File]::ReadAllText("intern/version-suffix.txt").Trim() -replace "`"", ""
}

$TAG = "voxie-" + $VOXIE_VERSION + $VOXIE_VERSION_SUFFIX + "-" + $VOXIE_REF_SHORT

echo "Voxie version is: '$TAG'"

$TARGET_NAME = $TAG
$TARGET = "build/release/install/" + $TARGET_NAME

New-Item -type directory $TARGET | Out-Null
$SrcDir = [System.IO.Path]::GetFullPath(".")

$QMAKE = $env:VOXIEBUILD_PATH_QT + "/bin/qmake"
# Meson needs qmake in path to find Qt
$env:PATH = $env:VOXIEBUILD_PATH_QT + "/bin;" + $env:PATH

#$MAKE = "nmake"
#$MAKE_ARGS = @()

# Use jom from Qt to allow a parallel build
$MAKE = $env:VOXIEBUILD_PATH_QT + "/../../Tools/QtCreator/bin/jom"
$MAKE_ARGS = @( "-j5" )


$QT_VERSION = [System.IO.Path]::GetFilename([System.IO.Path]::GetFullPath($env:VOXIEBUILD_PATH_QT + "/../..")) -replace "Qt", ""

SetVsCmd

echo "Unpacking 7z..."
New-Item -type directory build/dep | Out-Null
[System.Reflection.Assembly]::LoadFile($SrcDir + "/tools/build-dep/ICSharpCode.SharpZipLib.dll") | Out-Null
(New-Object ICSharpCode.SharpZipLib.Zip.FastZip).ExtractZip("tools/build-dep/7za920.zip", "build/dep", "7za.exe")
#New-Item -type directory build/dep/7z | Out-Null
#External build/dep/7za -obuild/dep/7z x "tools/build-dep/7z1604-extra.7z" | Out-Null

echo "Unpacking lessmsi..."
External build/dep/7za -obuild/dep/lessmsi x "tools/build-dep/lessmsi-v1.6.1.zip" | Out-Null

echo "Unpacking meson/ninja..."
#External build/dep/lessmsi/lessmsi x "tools\build-dep\meson-0.56.2-64.msi" "build\dep\meson\"
External build/dep/lessmsi/lessmsi x "tools\build-dep\meson-0.57.0-64.msi" "build\dep\meson\"
$MESON = $SrcDir + "/build/dep/meson/SourceDir/Meson/meson.exe"
$NINJA = $SrcDir + "/build/dep/meson/SourceDir/Meson/ninja.exe"
# Meson seems to need ninja in path
$env:PATH = $SrcDir + "/build/dep/meson/SourceDir/Meson;" + $env:PATH

echo "Unpacking boost..."
# TODO: Should boost be listed as bundeled?
External build/dep/7za -obuild/dep x "tools/build-dep/boost_1_61_0.zip" boost_1_61_0/boost | Out-Null
$BOOST_INCLUDE = "build/dep/boost_1_61_0"

echo "Unpacking HDF5..."
External build/dep/7za -obuild/dep x "tools/build-dep/hdf5-1.10.0-patch1-win64-vs2015-shared.zip" | Out-Null
External build/dep/lessmsi/lessmsi x "build\dep\hdf5\HDF5-1.10.0-win64.msi" "build\dep\hdf5\"
$PATH_HDF5 = "build/dep/hdf5/SourceDir/HDF_Group/HDF5/1.10.0"

echo "Unpacking LAPACKE..."
#External build/dep/7za -obuild/dep x "tools/build-dep/LAPACKE_examples.zip" | Out-Null
#Rename-Item build/dep/LAPACKE_examples/lib build/dep/LAPACKE_examples/lib.old
#New-Item -type directory build/dep/LAPACKE_examples/lib | Out-Null
#Copy-Item tools/build-dep/LAPACKE-3.5.0.lib build/dep/LAPACKE_examples/lib/LAPACKE.lib
#$PATH_LAPACKE = "$SrcDir/build/dep/LAPACKE_examples"

#External build/dep/7za -obuild/dep x "tools/build-dep/lapack-3.5.0.tgz" | Out-Null
#External build/dep/7za -obuild/dep x "build/dep/lapack-3.5.0.tar" | Out-Null
#Copy-Item build/dep/lapack-3.5.0/lapacke/include/lapacke_mangling_with_flags.h build/dep/lapack-3.5.0/lapacke/include/lapacke_mangling.h
#New-Item -type directory build/dep/lapack-3.5.0/lapacke/lib | Out-Null
#Copy-Item tools/build-dep/LAPACKE-3.5.0.lib build/dep/lapack-3.5.0/lapacke/lib/LAPACKE.lib
#$PATH_LAPACKE = "$SrcDir/build/dep/lapack-3.5.0/lapacke"

External build/dep/7za -obuild/dep x "tools/build-dep/lapack-3.7.0.tgz" | Out-Null
External build/dep/7za -obuild/dep x "build/dep/lapack-3.7.0.tar" | Out-Null
New-Item -type directory build/dep/lapack-3.7.0/LAPACKE/lib | Out-Null
Copy-Item tools/build-dep/lapack/3.7.0-win64/liblapacke.lib build/dep/lapack-3.7.0/LAPACKE/lib/LAPACKE.lib
$PATH_LAPACKE = "build/dep/lapack-3.7.0/LAPACKE"

echo "Extracting python files"
New-Item -type directory $TARGET/python | Out-Null
External build/dep/7za "-o$TARGET/python" x "tools/build-dep/python-$VERSION_PYTHON-embed-amd64.zip" | Out-Null

echo "Getting license files"
if (Test-Path -Path "build/licenses") {
   Remove-Item -recurse "build/licenses"
}
New-Item -type directory "build/licenses" | Out-Null
External "$TARGET/python/python.exe" -c "import sys; sys.path.insert(0, 'tools'); import download_dep; download_dep.main()" --license-only "--license-output-dir=build/licenses" "--unpack-program-7z=build/dep/7za" $files_lic

# Extract python packages
# Note: This has to be done before building voxie to get the license files
# TODO: Do more stuff in python, unpack build-time python somewhere else
# External "$TARGET/python/python.exe" "tools/python_packages_unpack.py" --python-tag=$VERSION_PYTHON_SHORT --abi-tag=${VERSION_PYTHON_SHORT}m --platform-tag=win_amd64 "$TARGET/python" "--license-output-dir=$TARGET"
External "$TARGET/python/python.exe" -c "import sys; sys.path.insert(0, 'tools'); import python_packages_unpack; python_packages_unpack.main()" --python-tag=$VERSION_PYTHON_SHORT --abi-tag=${VERSION_PYTHON_SHORT}m --platform-tag=win_amd64 "$TARGET/python" "--license-output-dir=build/licenses"

# Voxie
if ($DoVoxieBuild) {
echo "Building Voxie..."
# New-Item -type directory build | Out-Null
cd build
# -Dbuildtype=release seems to be needed to prevent meson from using the debug version of Qt
& $MESON "-Dboost_include_path=$BOOST_INCLUDE" "-Dhdf5_path=$PATH_HDF5" "-Dadditional_licenses_file=build/licenses/list.jsonl" "-Dlapacke_path=$PATH_LAPACKE" "-Ddebug=false" "-Doptimization=3" "-Dbuildtype=release" ..
CheckErrorCode
# & $NINJA --verbose -k0
& $NINJA --verbose
CheckErrorCode
# TODO: Use ninja install

cd $SrcDir
echo "Copying files..."
AddCr LICENSE $TARGET/LICENSE.txt
# Copy-Item C:/Windows/System32/ucrtbased.dll $TARGET/ # Needed for debug build, not sure where it should come from
Copy-Item build/src/Main/*.exe $TARGET/
Copy-Item build/src/VoxieClient/*.dll $TARGET/
Copy-Item build/src/VoxieBackend/*.dll $TARGET/
Copy-Item build/src/Voxie/*.dll $TARGET/
New-Item -type directory $TARGET/plugins | Out-Null
Copy-Item build/src/Plugin*/*.dll $TARGET/plugins
Copy-Item -recurse build/src/extra/licenses $TARGET/licenses
Copy-Item src/Main/voxie-path-install-win.json $TARGET/voxie-path.json
New-Item -type directory $TARGET/pythonlib/voxie | Out-Null
cp pythonlib/voxie/*.py $TARGET/pythonlib/voxie
cp pythonlib/voxie/*.json $TARGET/pythonlib/voxie
cp pythonlib/voxie/*.xml $TARGET/pythonlib/voxie
if (Test-Path -Path intern/pythonlib/voxie_full) {
  New-Item -type directory $TARGET/pythonlib/voxie_full | Out-Null
  cp intern/pythonlib/voxie_full/*.py $TARGET/pythonlib/voxie_full
  cp intern/pythonlib/voxie_full/*.json $TARGET/pythonlib/voxie_full
  cp intern/pythonlib/voxie_full/*.xml $TARGET/pythonlib/voxie_full
}
cp de.uni_stuttgart.Voxie.xml $TARGET/pythonlib/voxie
cp de.uni_stuttgart.Voxie.xml $TARGET
if (Test-Path -Path intern) {
  Copy-Item intern/*/de.uni_stuttgart.Voxie.*.xml $TARGET/pythonlib/voxie
  Copy-Item intern/*/de.uni_stuttgart.Voxie.*.xml $TARGET
}

if (Test-Path "lib/python/") {
  New-Item -type directory $TARGET/python-extra | Out-Null
  ForEach ($a in [System.IO.Directory]::GetDirectories("lib/python/")) {
    Copy-Item -recurse $a/* $TARGET/python-extra
  }
}

}

New-Item -type directory $TARGET/scripts | Out-Null
Copy-Item build/src/Script*/*.exe $TARGET/scripts
cp scripts/createChessboard.py $TARGET/scripts
cp scripts/getAverage.py $TARGET/scripts
cp scripts/shmemTest.py $TARGET/scripts

New-Item -type directory $TARGET/ext | Out-Null
# TODO: Copy Ext* except ExtFilter*
Copy-Item build/src/ExtFile*/*.exe $TARGET/ext
Copy-Item src/ExtFile*/*.json $TARGET/ext
Copy-Item ext/*.py $TARGET/ext
Copy-Item ext/*.json $TARGET/ext

New-Item -type directory $TARGET/filters | Out-Null
Copy-Item build/src/ExtFilter*/*.exe $TARGET/filters
Copy-Item src/ExtFilter*/*.json $TARGET/filters
Copy-Item filters/*.py $TARGET/filters
Copy-Item filters/*.json $TARGET/filters
Copy-Item filters/*.png $TARGET/filters
Copy-Item filters/*.md $TARGET/filters

New-Item -type directory $TARGET/doc | Out-Null
Copy-Item -recurse doc/topic $TARGET/doc
Copy-Item -recurse doc/prototype $TARGET/doc

New-Item -type directory $TARGET/lib | Out-Null
Copy-Item -recurse lib/katex-0.11.1 $TARGET/lib

# CMake (needed for building Expat and DBus)
echo "Unpacking CMake..."
External build/dep/7za -obuild/dep x "tools/build-dep/cmake-2.8.12.2-win32-x86.zip" | Out-Null
$CMAKE = $SrcDir + "/build/dep/cmake-2.8.12.2-win32-x86/bin/cmake"

# Expat (needed for DBus)
echo "Building Expat..."
New-Item -type directory "build/library/expat" | Out-Null
External build/dep/7za -obuild/library/expat/ x "tools/build-dep/expat-2.1.0.tar.gz" | Out-Null
External build/dep/7za -obuild/library/expat/ x "build/library/expat/expat-2.1.0.tar" | Out-Null
rm build/library/expat/expat-2.1.0.tar
cd build/library/expat/*
$ExpatDir = $PWD
New-Item -type directory build | Out-Null
cd build
& $CMAKE -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
CheckErrorCode
& $MAKE $MAKE_ARGS
CheckErrorCode
cd $SrcDir

# DBUS
echo "Building DBus..."
New-Item -type directory "build/library/dbus" | Out-Null
External build/dep/7za -obuild/library/dbus/ x "tools/build-dep/dbus-1.8.2.tar.gz" | Out-Null
External build/dep/7za -obuild/library/dbus/ x "build/library/dbus/dbus-1.8.2.tar" | Out-Null
rm build/library/dbus/dbus-1.8.2.tar
cd build/library/dbus/*
$DBusDir = $PWD
New-Item -type directory build | Out-Null
cd build
& $CMAKE -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release `-DEXPAT_INCLUDE_DIR=$ExpatDir/lib `-DEXPAT_LIBRARY=$ExpatDir/build/expat.lib -DDBUS_BUILD_TESTS=OFF ../cmake
CheckErrorCode
& $MAKE $MAKE_ARGS
CheckErrorCode
cd $SrcDir
Copy-Item $ExpatDir/build/expat.dll $TARGET
Copy-Item $DBusDir/build/bin/dbus-1.dll $TARGET
Copy-Item $DBusDir/build/bin/dbus-daemon.exe $TARGET
New-Item -type directory $TARGET/bus | Out-Null
Copy-Item $DBusDir/build/bus/*.conf $TARGET/bus
New-Item -type directory $TARGET/bus/session.d | Out-Null
New-Item -type directory $TARGET/bus/system.d | Out-Null

# HDF5
echo "Copying HDF5 files..."
Copy-Item $PATH_HDF5/bin/hdf5.dll $TARGET
Copy-Item $PATH_HDF5/bin/zlib.dll $TARGET
Copy-Item $PATH_HDF5/bin/szip.dll $TARGET
Copy-Item $PATH_HDF5/bin/vcruntime*.dll $TARGET
Copy-Item $PATH_HDF5/bin/msvcp*.dll $TARGET

# LAPACK
echo "Copying LAPACK files..."
Copy-Item tools/build-dep/lapack/3.7.0-win64/libblas.dll $TARGET
Copy-Item tools/build-dep/lapack/3.7.0-win64/liblapack.dll $TARGET
Copy-Item tools/build-dep/lapack/3.7.0-win64/liblapacke.dll $TARGET
#Copy-Item tools/build-dep/lapack/3.7.0-win64/libtmglib.dll $TARGET

if ($DoVoxieBuild) {
echo "Copying Qt files..."
& ${env:VOXIEBUILD_PATH_QT}/bin/windeployqt.exe $TARGET/voxie.exe $TARGET/Voxie.dll $TARGET/VoxieClient.dll $TARGET/VoxieBackend.dll (gci $TARGET/plugins/*.dll | % { "$_" })
CheckErrorCode
Copy-Item -recurse $TARGET/platforms $TARGET/scripts # Needed for Qt executables
Copy-Item -recurse $TARGET/platforms $TARGET/ext # Needed for Qt executables
Copy-Item -recurse $TARGET/platforms $TARGET/filters # Needed for Qt executables
rm $TARGET/vcredist*.exe
# QtWebEngineCore... is too large, artifact is rejected by build server => Works when increasing the setting in the gitlab CI admin panel https://stackoverflow.com/questions/57367224/how-to-increase-maximum-artifacts-size-for-gitlab-on-premises
#rm $TARGET/Qt5WebEngine*.dll
}

if (1) {
  Move-Item "$TARGET/python/dbus-python-windows/Lib/dbus" "$TARGET/python"
  Move-Item "$TARGET/python/dbus-python-windows/DLLs/_dbus_bindings.pyd" "$TARGET/python"
  Move-Item "$TARGET/python/dbus-python-windows/DLLs/libpython3.7m.dll" "$TARGET/python" # TODO: ?
  Copy-Item "$TARGET/dbus-1.dll" "$TARGET/python/libdbus-1-3.dll" # TODO: ?
  Remove-Item -recurse "$TARGET/python/dbus-python-windows"
}

echo "Creating README..."
$text = [IO.File]::ReadAllText("tools/README-windows.tmpl") -replace "`n", "`r`n"
$text = $text -replace "%QT_VERSION%", $QT_VERSION
$text = $text -replace "%VOXIE_TAG%", $VOXIE_TAG
$text = $text -replace "%VOXIE_COMMIT%", $VOXIE_REF
[IO.File]::WriteAllText("build/README", $text)
AddCr build/README $TARGET/README.txt

echo "Creating .zip file..."
$ScriptCode = {
    param($TARGET_NAME)
    set ErrorActionPreference Stop
    Set-StrictMode -Version 2.0
    #Add-Type -Assembly System.IO.Compression.FileSystem
    [System.Reflection.Assembly]::LoadFile($env:WINDIR + "/Microsoft.NET/assembly/GAC_MSIL/System.IO.Compression.FileSystem/v4.0_4.0.0.0__b77a5c561934e089/System.IO.Compression.FileSystem.dll")
    $compressionLevel = [System.IO.Compression.CompressionLevel]::Optimal
    [System.IO.Compression.ZipFile]::CreateFromDirectory("build/release/install", "build/release/" + $TARGET_NAME + "-win64.zip", $compressionLevel, $false)
}

Invoke-CLR4PowerShellCommand -ScriptBlock $ScriptCode -ArgumentList $TARGET_NAME

Copy-Item build/release/voxie*.zip .

echo "Done."
