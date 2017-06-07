set ErrorActionPreference Stop
Set-StrictMode -Version 2.0

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

$files = @(
  "ICSharpCode.SharpZipLib.dll",
  "7za920.zip",
  "lessmsi-v1.6.1.zip",
  "boost_1_61_0.zip",
  "hdf5-1.10.0-patch1-win64-vs2015-shared.zip",
  "cmake-2.8.12.2-win32-x86.zip",
  "expat-2.1.0.tar.gz",
  "dbus-1.8.2.tar.gz",

  "python-3.5.3-embed-amd64.zip",
  "Python-3.5.3.tgz", # For the license file
  "numpy-1.12.1-cp35-none-win_amd64.whl",
  "numpy-1.12.1.zip", # For the license file
  "dbus-1.2.4-cp35-none-win_amd64.zip"
  "Pillow-4.1.1-cp35-cp35m-win_amd64.whl",
  "Pillow-4.1.1.tar.gz" # For the license file
)

$URL_EXPAT_SRC = GetUrl("expat-2.1.0.tar.gz")
$URL_DBUS_SRC = GetUrl("dbus-1.8.2.tar.gz")
$URL_HDF5 = GetUrl("hdf5-1.10.0-patch1.tar.bz2")
$URL_QT = GetUrl("qt-everywhere-opensource-src-5.6.0.tar.gz")

$URL_PYTHON_SRC = GetUrl("Python-3.5.3.tgz")
$URL_NUMPY_SRC = GetUrl("numpy-1.12.1.zip")
$URL_PYTHON_DBUS_SRC = GetUrl("dbus-python-1.2.4.tar.gz")
$URL_PILLOW_SRC = GetUrl("Pillow-4.1.1.tar.gz")

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
CheckEnvVar PATH_VISUAL_STUDIO

# Qt
# http://download.qt.io/official_releases/qt/5.6/5.6.0/qt-opensource-windows-x86-msvc2015_64-5.6.0.exe
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
  $REV = git describe --always
  CheckErrorCode
  $REV = $REV.Trim()
  if ($REV.StartsWith("voxie-")) {
      $REV = $REV.Substring(6);
  }
  $TARGET_NAME = "voxie-" + $REV
} else {
  $TARGET_NAME = "voxie"
}
$TARGET = "build/release/install/" + $TARGET_NAME

New-Item -type directory $TARGET | Out-Null
$SrcDir = [System.IO.Path]::GetFullPath(".")

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

echo "Unpacking 7z..."
New-Item -type directory build/dep | Out-Null
[System.Reflection.Assembly]::LoadFile($SrcDir + "/tools/build-dep/ICSharpCode.SharpZipLib.dll") | Out-Null
(New-Object ICSharpCode.SharpZipLib.Zip.FastZip).ExtractZip("tools/build-dep/7za920.zip", "build/dep", "7za.exe")
#New-Item -type directory build/dep/7z | Out-Null
#External build/dep/7za -obuild/dep/7z x "tools/build-dep/7z1604-extra.7z" | Out-Null

echo "Unpacking boost..."
External build/dep/7za -obuild/dep x "tools/build-dep/boost_1_61_0.zip" boost_1_61_0/boost | Out-Null
$BOOST_INCLUDE = $SrcDir + "/build/dep/boost_1_61_0"

echo "Unpacking lessmsi..."
External build/dep/7za -obuild/dep/lessmsi x "tools/build-dep/lessmsi-v1.6.1.zip" | Out-Null

echo "Unpacking HDF5..."
External build/dep/7za -obuild/dep x "tools/build-dep/hdf5-1.10.0-patch1-win64-vs2015-shared.zip" | Out-Null
External build/dep/lessmsi/lessmsi x "build\dep\hdf5\HDF5-1.10.0-win64.msi" "build\dep\hdf5\"
$PATH_HDF5 = "$SrcDir/build/dep/hdf5/SourceDir/HDF_Group/HDF5/1.10.0"

# Voxie
echo "Building Voxie..."
# New-Item -type directory build | Out-Null
cd build
& $QMAKE "BOOST_PATH=$BOOST_INCLUDE" "HDF5_PATH=$PATH_HDF5" ../src
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
Copy-Item build/Ext*/*/*.exe $TARGET/scripts
Copy-Item src/Ext*/*.conf $TARGET/scripts
cp scripts/Example*.js $TARGET/scripts
cp scripts/voxie.py $TARGET/scripts
cp scripts/createChessboard.py $TARGET/scripts
cp scripts/getAverage.py $TARGET/scripts
cp scripts/shmemTest.py $TARGET/scripts
cp scripts/de.uni_stuttgart.Voxie.xml $TARGET

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
AddCr $ExpatDir/COPYING $TARGET/COPYING.expat.txt

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
AddCr $DBusDir/COPYING $TARGET/COPYING.dbus.txt
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
AddCr lib/COPYING.hdf5 $TARGET/COPYING.hdf5.txt

echo "Copying Qt files..."
& ${env:VOXIEBUILD_PATH_QT}/bin/windeployqt.exe $TARGET/voxie.exe
CheckErrorCode
Copy-Item -recurse $TARGET/platforms $TARGET/scripts # Needed for ScriptGetAverage
rm $TARGET/vcredist*.exe
AddCr $env:VOXIEBUILD_PATH_QT/../../Licenses/LICENSE $TARGET/COPYING.qt.txt

echo "Extracting python files"
New-Item -type directory $TARGET/python | Out-Null
External build/dep/7za "-o$TARGET/python" x "tools/build-dep/python-3.5.3-embed-amd64.zip" | Out-Null
External build/dep/7za "-o$TARGET/python" x "tools/build-dep/numpy-1.12.1-cp35-none-win_amd64.whl" | Out-Null
External build/dep/7za "-o$TARGET/python" x "tools/build-dep/dbus-1.2.4-cp35-none-win_amd64.zip" | Out-Null
External build/dep/7za "-o$TARGET/python" x "tools/build-dep/Pillow-4.1.1-cp35-cp35m-win_amd64.whl" | Out-Null
Copy-Item $TARGET/python/COPYING.dbus-python.txt $TARGET/
External build/dep/7za "-obuild/dep/python-src" x "tools/build-dep/Python-3.5.3.tgz" | Out-Null
External build/dep/7za "-obuild/dep/python-src" x "build/dep/python-src/Python-3.5.3.tar" Python-3.5.3/LICENSE | Out-Null
AddCr build/dep/python-src/Python-3.5.3/LICENSE $TARGET/COPYING.python.txt
External build/dep/7za "-obuild/dep/numpy-src" x "tools/build-dep/numpy-1.12.1.zip" numpy-1.12.1/LICENSE.txt | Out-Null
AddCr build/dep/numpy-src/numpy-1.12.1/LICENSE.txt $TARGET/COPYING.numpy.txt
External build/dep/7za "-obuild/dep/pillow-src" x "tools/build-dep/Pillow-4.1.1.tar.gz" | Out-Null
External build/dep/7za "-obuild/dep/pillow-src" x "build/dep/pillow-src/dist/Pillow-4.1.1.tar" Pillow-4.1.1/LICENSE | Out-Null
AddCr build/dep/pillow-src/Pillow-4.1.1/LICENSE $TARGET/COPYING.pillow.txt

echo "Copying manual..."
Copy-Item manual.pdf $TARGET

echo "Creating README..."
$text = [IO.File]::ReadAllText("tools/README-windows.tmpl") -replace "`n", "`r`n"
$text = $text -replace "%QT_VERSION%", $QT_VERSION
$text = $text -replace "%OPENCL_ICD_LOADER_COMMIT%", $OPENCL_ICD_LOADER_COMMIT
$text = $text -replace "%EXPAT_URL%", $URL_EXPAT_SRC
$text = $text -replace "%DBUS_URL%", $URL_DBUS_SRC
$text = $text -replace "%HDF5_URL%", $URL_HDF5
$text = $text -replace "%URL_PYTHON_SRC%", $URL_PYTHON_SRC
$text = $text -replace "%URL_NUMPY_SRC%", $URL_NUMPY_SRC
$text = $text -replace "%URL_PYTHON_DBUS_SRC%", $URL_PYTHON_DBUS_SRC
$text = $text -replace "%URL_PILLOW_SRC%", $URL_PILLOW_SRC
$text = $text -replace "%QT_URL%", $URL_QT
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
