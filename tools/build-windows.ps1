set ErrorActionPreference Stop
Set-StrictMode -Version 2.0

function CheckErrorCode {
  if (-Not $?) {
    throw "Command execution failed"
  }
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

function CheckEnvVar ([string]$name) {
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
    pushd $env:VOXIEBUILD_PATH_VISUAL_STUDIO/VC
    cmd /c "vcvarsall.bat x64&set" |
    foreach {
      if ($_ -match "(.*?)=(.*)") {
        Set-Item -force -path "ENV:\$($matches[1])" -value "$($matches[2])"
      }
    }
    popd
    CheckErrorCode
}


echo "Checking environment variables..."

# CMake path
# http://www.cmake.org/files/v2.8/cmake-2.8.12.2-win32-x86.exe
CheckEnvVar PATH_CMAKE

# Visual Studio
# https://www.visualstudio.com/downloads/download-visual-studio-vs (Visual Studio Community 2015 with Update 2)
CheckEnvVar PATH_VISUAL_STUDIO

# Qt
# http://download.qt.io/official_releases/qt/5.6/5.6.0/qt-opensource-windows-x86-msvc2015_64-5.6.0.exe
CheckEnvVar PATH_QT
CheckEnvVar URL_QT
CheckEnvVar URL_QT_COPY

# HDF5
# http://www.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.0-patch1/bin/windows/extra/hdf5-1.10.0-patch1-win64-vs2015-shared.zip
CheckEnvVar PATH_HDF5
CheckEnvVar URL_HDF5

# Boost include directory (or source directory)
# https://sourceforge.net/projects/boost/files/boost/1.61.0/boost_1_61_0.zip/download
CheckEnvVar PATH_BOOST_INCLUDE

# Expat source directory
# http://sourceforge.net/projects/expat/files/expat/2.1.0/expat-2.1.0.tar.gz/download
CheckEnvVar PATH_EXPAT_SRC
CheckEnvVar URL_EXPAT_SRC

# DBus source directory
# http://cgit.freedesktop.org/dbus/dbus/snapshot/dbus-1.8.2.zip
CheckEnvVar PATH_DBUS_SRC
CheckEnvVar URL_DBUS_SRC


echo "Cleaning up build directory..."

if (Test-Path build) {
  rmdir -recurse build
  CheckErrorCode
}

rm voxie*.zip

echo "Checking git config..."

$AutoCrlf = (git config core.autocrlf) | Out-String
CheckErrorCode

#echo $AutoCrlf
if ($AutoCrlf.Trim() -ne "false") {
  throw "core.autocrlf is not set to false"
}

#$TARGET_NAME = "voxie"
$REV = git describe --always
CheckErrorCode
$REV = $REV.Trim()
if ($REV.StartsWith("voxie-")) {
    $REV = $REV.Substring(6);
}
$TARGET_NAME = "voxie-" + $REV
$TARGET = "build/release/install/" + $TARGET_NAME

New-Item -type directory $TARGET | Out-Null
$SrcDir = [System.IO.Path]::GetFullPath(".")

$CMAKE = $env:VOXIEBUILD_PATH_CMAKE + "/bin/cmake"
$QMAKE = $env:VOXIEBUILD_PATH_QT + "/bin/qmake"


#$MAKE = "nmake"
#$MAKE_ARGS = @()

# Use jom from Qt to allow a parallel build
$MAKE = $env:VOXIEBUILD_PATH_QT + "/../../Tools/QtCreator/bin/jom"
$MAKE_ARGS = @( "-j5" )


$QT_VERSION = [System.IO.Path]::GetFilename([System.IO.Path]::GetFullPath($env:VOXIEBUILD_PATH_QT + "/../..")) -replace "Qt", ""
$VOXIE_TAG = $env:CI_BUILD_REF
if (($env:CI_BUILD_TAG -ne $null) -and ($env:CI_BUILD_TAG -ne "")) {
    $VOXIE_TAG = $env:CI_BUILD_TAG
}

$OPENCL_ICD_LOADER_COMMIT = [IO.File]::ReadAllText("lib/opencl-icd-loader.txt")
$OPENCL_ICD_LOADER_COMMIT = $OPENCL_ICD_LOADER_COMMIT.SubString($OPENCL_ICD_LOADER_COMMIT.IndexOf("commit "))
$OPENCL_ICD_LOADER_COMMIT = $OPENCL_ICD_LOADER_COMMIT.SubString(7)
$OPENCL_ICD_LOADER_COMMIT = $OPENCL_ICD_LOADER_COMMIT.SubString(0, 40)

SetVsCmd

# Voxie
echo "Building Voxie..."
# New-Item -type directory build | Out-Null
cd build
& $QMAKE "BOOST_PATH=$env:VOXIEBUILD_PATH_BOOST_INCLUDE" "HDF5_PATH=$env:VOXIEBUILD_PATH_HDF5" ../src
CheckErrorCode
& $MAKE $MAKE_ARGS
CheckErrorCode
cd $SrcDir
echo "Copying files..."
AddCr COPYING $TARGET/COPYING.txt
AddCr lib/opencl-icd-loader/LICENSE.txt $TARGET/COPYING.opencl-icd-loader.txt
Copy-Item build/Main/*/*.exe $TARGET/
Copy-Item build/Voxie/*/*.dll $TARGET/
Copy-Item build/lib/OpenCLICDLoader/*/OpenCL.dll $TARGET/
New-Item -type directory $TARGET/plugins | Out-Null
Copy-Item build/Plugin*/*/*.dll $TARGET/plugins
New-Item -type directory $TARGET/scripts | Out-Null
Copy-Item build/Script*/*/*.exe $TARGET/scripts
cp scripts/Example*.js $TARGET/scripts
cp scripts/de.uni_stuttgart.Voxie.xml $TARGET

# Expat (needed for DBus)
echo "Building Expat..."
New-Item -type directory "build/library/expat" | Out-Null
Copy-Item -recurse $env:VOXIEBUILD_PATH_EXPAT_SRC "build/library/expat/"
cd build/library/expat/*
$ExpatDir = $PWD
New-Item -type directory build | Out-Null
cd build
& $CMAKE -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
CheckErrorCode
& $MAKE $MAKE_ARGS
CheckErrorCode
cd $SrcDir
AddCr $ExpatDir/COPYING $TARGET/COPYING.expat.txt

# DBUS
echo "Building DBus..."
#ExpandZIPFile $env:VOXIEBUILD_PATH_DBUS_SRC "build/library/dbus"
New-Item -type directory "build/library/dbus" | Out-Null
Copy-Item -recurse $env:VOXIEBUILD_PATH_DBUS_SRC "build/library/dbus/"
cd build/library/dbus/*
$DBusDir = $PWD
New-Item -type directory build | Out-Null
cd build
& $CMAKE -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release `-DEXPAT_INCLUDE_DIR=$ExpatDir/lib `-DEXPAT_LIBRARY=$ExpatDir/build/expat.lib -DDBUS_BUILD_TESTS=OFF ../cmake
CheckErrorCode
& $MAKE $MAKE_ARGS
CheckErrorCode
cd $SrcDir
AddCr $DBusDir/COPYING $TARGET/COPYING.dbus.txt
Copy-Item $ExpatDir/build/expat.dll $TARGET
Copy-Item $DBusDir/build/bin/dbus-1.dll $TARGET
Copy-Item $DBusDir/build/bin/dbus-daemon.exe $TARGET
New-Item -type directory $TARGET/bus | Out-Null
Copy-Item $DBusDir/build/bus/*.conf $TARGET/bus
New-Item -type directory $TARGET/bus/session.d | Out-Null
New-Item -type directory $TARGET/bus/system.d | Out-Null

$env:VOXIEBUILD_PATH_HDF5
# HDF5
echo "Copying HDF5 files..."
Copy-Item $env:VOXIEBUILD_PATH_HDF5/bin/hdf5.dll $TARGET
Copy-Item $env:VOXIEBUILD_PATH_HDF5/bin/zlib.dll $TARGET
Copy-Item $env:VOXIEBUILD_PATH_HDF5/bin/szip.dll $TARGET
Copy-Item $env:VOXIEBUILD_PATH_HDF5/bin/vcruntime*.dll $TARGET
Copy-Item $env:VOXIEBUILD_PATH_HDF5/bin/msvcp*.dll $TARGET
AddCr lib/COPYING.hdf5 $TARGET/COPYING.hdf5.txt

echo "Copying Qt files..."
& ${env:VOXIEBUILD_PATH_QT}/bin/windeployqt.exe $TARGET/voxie.exe
CheckErrorCode
Copy-Item -recurse $TARGET/platforms $TARGET/scripts # Needed for ScriptGetAverage
rm $TARGET/vcredist*.exe
AddCr $env:VOXIEBUILD_PATH_QT/../../Licenses/LICENSE COPYING.qt.txt

echo "Copying manual..."
Copy-Item manual.pdf $TARGET

echo "Creating README..."
$text = [IO.File]::ReadAllText("tools/README-windows.tmpl") -replace "`n", "`r`n"
$text = $text -replace "%QT_VERSION%", $QT_VERSION
$text = $text -replace "%OPENCL_ICD_LOADER_COMMIT%", $OPENCL_ICD_LOADER_COMMIT
$text = $text -replace "%EXPAT_URL%", $env:VOXIEBUILD_URL_EXPAT_SRC
$text = $text -replace "%DBUS_URL%", $env:VOXIEBUILD_URL_DBUS_SRC
$text = $text -replace "%HDF5_URL%", $env:VOXIEBUILD_URL_HDF5
$text = $text -replace "%QT_URL%", $env:VOXIEBUILD_URL_QT
$text = $text -replace "%QT_URL_COPY%", $env:VOXIEBUILD_URL_QT_COPY
$text = $text -replace "%VOXIE_TAG%", $VOXIE_TAG
$text = $text -replace "%VOXIE_COMMIT%", $env:CI_BUILD_REF
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
