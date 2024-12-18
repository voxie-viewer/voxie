/*
 * Copyright (c) 2010-2012 Steffen Kieß
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "Exception.hpp"

#include <Core/StrError.h>
#include <Core/Memory.hpp>
#include <Core/Util.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>

#if OS_UNIX && !defined(__CYGWIN__)
#include <execinfo.h>
#endif

#if OS_UNIX
#include <dlfcn.h>
#include <link.h>
#elif OS_WIN
#include <windows.h>
// Note: windows.h has to be included before imagehlp.h
#include <imagehlp.h>
#endif

#ifndef _MSC_VER
#include <cxxabi.h>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#else
#define popen _popen
#define pclose _pclose
static inline int MY_vsnprintf(char* str, size_t size, const char* format,
                               va_list ap) {
  int count = -1;

  if (size != 0) count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
  if (count == -1) count = _vscprintf(format, ap);

  return count;
}
static inline int MY_snprintf(char* str, size_t size, const char* format, ...) {
  int count;
  va_list ap;

  va_start(ap, format);
  count = MY_vsnprintf(str, size, format, ap);
  va_end(ap);

  return count;
}
#define snprintf MY_snprintf
#endif

#if !OS_UNIX
#ifndef WEXITSTATUS
#define WEXITSTATUS(x) (x)
#endif
#endif

#if OS_WIN
// Define ptrint to be a pointer-size int
#ifdef _WIN64
typedef DWORD64 ptrint;
#else
typedef DWORD ptrint;
#endif
#endif

#if OS_UNIX
template <typename F>
inline void call_dl_iterate_phdr(const F& f) {
  std::exception_ptr error;
  auto lambda = [&](dl_phdr_info* info, size_t size) {
    try {
      return f(info, size);
    } catch (...) {
      error = std::current_exception();
      return 1;
    }
  };
  auto lambdaPtr = &lambda;
  dl_iterate_phdr(
      [](dl_phdr_info* info, size_t size, void* data) {
        auto lambdaPtr2 = reinterpret_cast<decltype(lambdaPtr)>(data);
        return (*lambdaPtr2)(info, size);
      },
      lambdaPtr);
  if (error) std::rethrow_exception(error);
}
#endif

namespace Core {
namespace {
#if OS_UNIX
template <class T>
inline T dlcheck(const char* name, T value) {
  if (!value) {
    std::string s = name;
    s += ": ";
    const char* msg = dlerror();
    if (msg) s += msg;
    throw SimpleStdException(s);
  }
  return value;
}
#endif

inline int check(const char* name, int value) {
  if (value < 0) {
    std::string s = name;
    s += ": ";
#if OS_UNIX
    int err = errno;
    errno = 0;
    char buf[2048];
    const char* msg;
    if (MY_XSI_strerror_r(err, buf, sizeof(buf)) == 0)
      msg = buf;
    else
      msg = "<strerror_r failed>";
#else
    const char* msg = strerror(errno);  // This will break with multiple threads
#endif
    if (msg) s += msg;
    throw SimpleStdException(s);
  }
  return value;
}

#if OS_WIN
inline ptrint checkWin(const char* name, ptrint value, DWORD ignore = 0) {
  if (!value) {
    DWORD err = GetLastError();
    if (err == ignore) return value;

    std::stringstream str;
    char* lpMsgBuf;
    if (!FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                        (char*)&lpMsgBuf, 0, NULL)) {
      DWORD err2 = GetLastError();
      str << "FormatMessage () for " << err << " returned " << err2;
      throw SimpleStdException(str.str());
    }
    WindowsLocalRefHolder<char> refHolder(lpMsgBuf);
    size_t len = strlen(lpMsgBuf);
    if (len && lpMsgBuf[len - 1] == '\n') {
      lpMsgBuf[len - 1] = 0;
      if ((len > 1) && lpMsgBuf[len - 2] == '\r') lpMsgBuf[len - 2] = 0;
    }
    str << name << ": " << lpMsgBuf << " (" << err << ")";
    throw SimpleStdException(str.str());
  }
  return value;
}

std::string getCallingFilename() {
  std::vector<char> filename(128);
  DWORD size;
  while ((size = GetModuleFileNameA(NULL, filename.data(), filename.size())) ==
         filename.size())
    filename.resize(filename.size() * 2);
  checkWin("GetModuleFileNameA", size);

  return std::string(filename.data(), size);
}
#endif

#define SIMPLE_ASSERT_L(x, l)                                                  \
  do {                                                                         \
    if (!(x)) {                                                                \
      throw SimpleStdException("Simple assertion `" #x "' failed at " __FILE__ \
                               ":" #l);                                        \
    }                                                                          \
  } while (0)
#define SIMPLE_ASSERT_L_(x, l) SIMPLE_ASSERT_L(x, l)
#define SIMPLE_ASSERT(x) SIMPLE_ASSERT_L_(x, __LINE__)

}  // namespace

SimpleStdException::SimpleStdException(std::string descr) : descr_(descr) {}

SimpleStdException::~SimpleStdException() throw() {}

const char* SimpleStdException::what() const throw() { return descr_.c_str(); }

InlineStackFrame::InlineStackFrame(const std::string& method,
                                   const std::string& sourceFile,
                                   uint64_t lineNumber)
    : _method(method), _sourceFile(sourceFile), _lineNumber(lineNumber) {}

StackFrame::StackFrame(void* ptr) : _ptr(ptr) {}

StackFrame::StackFrame(const StackFrame& o) : _ptr(o._ptr) {}

namespace {
inline std::string pad(const std::string str, size_t* size, bool left = false) {
  size_t len = str.length();
  size_t missing = 0;
  if (size) {
    if (*size < len)
      *size = len;
    else
      missing = *size - len;
  }
  if (!missing) return str;
  if (left)
    return std::string(missing, ' ') + str;
  else
    return str + std::string(missing, ' ');
}
}  // namespace

std::string StackFrame::toString(int* iPtr, size_t* addrSize, size_t* symbSize,
                                 size_t* locSize) const {
  std::stringstream str;

  if (false) {
    str << "-- " << (hasSharedObject() ? "SO" : "NOSO");
    str.setf(std::ios::hex, std::ios::basefield);
    if (hasSharedObject()) {
      str << " " << sharedObjectName();
      if (hasBuildID()) {
        str << " ";
        str.fill('0');
        for (const auto& val : buildID()) str << std::setw(2) << (int)val;
      }
      if (hasSharedObjectBase()) str << " +0x" << sharedObjectOffset();
    }
    str << std::endl;
  }

  const std::vector<InlineStackFrame> isfs = inlineStackFrames();
  size_t size = isfs.size();
  if (size == 0) size = 1;
  for (size_t j = 0; j < size; j++) {
    if (j > 0) str << std::endl;
    str.setf(std::ios::dec, std::ios::basefield);
    str.fill('0');
    if (iPtr) {
      str << " #" << std::setw(2) << (*iPtr)++;
    }
    str.setf(std::ios::hex, std::ios::basefield);
    str << " at";
    {
      std::stringstream str2;
      str2 << ptr();
      size_t len = str2.str().length();
      if (j > 0) {
        str << " [" << pad(std::string(len, ' '), addrSize, true) << "]";
      } else {
        str << " [" << pad(str2.str(), addrSize, true) << "]";
      }
    }

    /*
      if (hasSharedObject ())
        str << " " << sharedObjectName () << "+0x" << sharedObjectOffset ();
      else
        str << " no_shobj";
      */

    bool hasSymb = false;
    std::string symb;
    if (j < isfs.size()) {
      const InlineStackFrame& isf = isfs[j];
      symb = isf.method();
      if (symb != "??") {
        hasSymb = true;

        // demangle name
#ifndef _MSC_VER
        if (symb.substr(0, 2) == "_Z") {
          size_t len;
          int status;
          MallocRefHolder<char> demangled =
              abi::__cxa_demangle(symb.c_str(), NULL, &len, &status);
          SIMPLE_ASSERT(status == 0 || status == -2);
          if (status == 0) symb = demangled.p;
        }
#endif
      }
    }
    if (!hasSymb && hasSymbol()) {
      symb = symbolName();

      // demangle name
#ifndef _MSC_VER
      if (symb.substr(0, 2) == "_Z") {
        size_t len;
        int status;
        MallocRefHolder<char> demangled =
            abi::__cxa_demangle(symb.c_str(), NULL, &len, &status);
        SIMPLE_ASSERT(status == 0 || status == -2);
        if (status == 0) symb = demangled.p;
      }
#endif

      std::stringstream str2;
      str2 << symb << "+0x" << symbolOffset();
      symb = str2.str();
      hasSymb = true;
    }
    if (!hasSymb) {
      if (hasSharedObject() && sharedObjectName() != "" &&
          hasSharedObjectBase()) {
        std::stringstream str2;
        str2.setf(std::ios::hex, std::ios::basefield);
        str2 << "<nosymb " + sharedObjectName() + "+0x" << sharedObjectOffset()
             << ">";
        symb = str2.str();
      } else {
        symb = "<no symbol information>";
      }
    }
    str << " " << pad(symb, symbSize);

    str.setf(std::ios::dec, std::ios::basefield);
    if (j < isfs.size()) {
      const InlineStackFrame& isf = isfs[j];

      const int maxPathElements = 2;
      std::string fn = isf.sourceFile();
      size_t pos = fn.length();
      for (int i = 0;
           i < maxPathElements && pos != std::string::npos && pos != 0; i++)
        pos = fn.rfind('/', pos - 1);
      if (pos != std::string::npos && pos != 0 && pos != fn.length()) {
        fn = ".../" + fn.substr(pos + 1);
      }

      if (isf.lineNumber()) {
        std::stringstream str2;
        str2 << fn << ":" << isf.lineNumber();
        str << " (" << pad(str2.str(), locSize) << ")";
      }
    } else {
      //str << " no_addr2line";
    }
  }

  return str.str();
}

void StackFrame::doResolve(
    UNUSED const std::lock_guard<std::mutex>& guard) const {
  if (_isResolved) abort();

#if OS_UNIX && !defined(__CYGWIN__)
  Dl_info info;

  //dlerror ();
  //dlcheck ("dladdr", dladdr (ptr (), &info));
  if (!dladdr(ptr(), &info)) {
    _hasSharedObject = false;
    _sharedObjectBaseFirstSegment = NULL;
    _hasSharedObjectBase = false;
    _sharedObjectBase = NULL;
    _hasBuildID = false;
    _hasSymbol = false;
    _symbolAddr = NULL;
  } else {
    if (info.dli_fname) {
      _hasSharedObject = true;
      _sharedObjectName = info.dli_fname;
      _sharedObjectBaseFirstSegment = info.dli_fbase;

      _hasSharedObjectBase = false;
      _hasBuildID = false;

      // Look for information using dl_phdr_info
      // TODO: Cache this?
      call_dl_iterate_phdr([&](dl_phdr_info* phdrInfo, UNUSED size_t size) {
        bool found = false;
        for (size_t i = 0; i < phdrInfo->dlpi_phnum; i++) {
          if (phdrInfo->dlpi_phdr[i].p_type != PT_LOAD) continue;

          auto map_start =
              (void*)(phdrInfo->dlpi_addr + phdrInfo->dlpi_phdr[i].p_vaddr);

          if (map_start != _sharedObjectBaseFirstSegment) return 0;

          found = true;
          break;
        }
        if (!found) return 0;  // No PT_LOAD found, continue

        _sharedObjectBase = (void*)phdrInfo->dlpi_addr;
        _hasSharedObjectBase = true;

        // Note: This has to be done inside the dl_iterate_phdr() callback to make sure that the object is not unloaded while its PT_NOTE data is used

        // Look for build ID
        // https://git.mattst88.com/build-id/tree/build-id.c?id=5380624471e4ac22edd397cd91289844650ef5e5#n86
        // https://github.com/bminor/mesa-mesa/blob/dcba7731e6056b6cad03064f90a97cf206e68a75/src/util/build_id.c#L73
        for (size_t i = 0; i < phdrInfo->dlpi_phnum; i++) {
          if (phdrInfo->dlpi_phdr[i].p_type != PT_NOTE) continue;

          auto note = (ElfW(Nhdr)*)(phdrInfo->dlpi_addr +
                                    phdrInfo->dlpi_phdr[i].p_vaddr);
          ptrdiff_t len = phdrInfo->dlpi_phdr[i].p_filesz;

#define ALIGN(val, align) (((val) + (align)-1) & ~((align)-1))
          while (len >= (ptrdiff_t)sizeof(ElfW(Nhdr))) {
            char* noteName = (char*)(note + 1);
            if (note->n_type == NT_GNU_BUILD_ID && note->n_descsz != 0 &&
                note->n_namesz == 4 && memcmp(noteName, "GNU", 4) == 0) {
              char* noteValue = noteName + ALIGN(note->n_namesz, 4);
              _buildID.resize(note->n_descsz);
              memcpy(_buildID.data(), noteValue, note->n_descsz);
              _hasBuildID = true;
              break;
            }

            size_t offset = sizeof(ElfW(Nhdr)) + ALIGN(note->n_namesz, 4) +
                            ALIGN(note->n_descsz, 4);
            note = (ElfW(Nhdr)*)(void*)((char*)note + offset);
            len -= offset;
          }
          break;
        }
#undef ALIGN

        return 1;  // Finished
      });
    } else {
      _hasSharedObject = false;
      _sharedObjectBaseFirstSegment = NULL;
      _hasSharedObjectBase = false;
      _sharedObjectBase = NULL;
      _hasBuildID = false;
    }

    if (info.dli_sname) {
      _hasSymbol = true;
      _symbolName = info.dli_sname;
      _symbolAddr = info.dli_saddr;
    } else {
      _hasSymbol = false;
      _symbolAddr = NULL;
    }
  }

#elif OS_WIN
  // TODO: According to https://docs.microsoft.com/en-us/windows/desktop/api/dbghelp/nf-dbghelp-syminitialize Syminitialize() etc. are single threaded, do something about that?
  //checkWin ("SymInitialize", SymInitialize (GetCurrentProcess(), nullptr, TRUE));
  SymInitialize(
      GetCurrentProcess(), nullptr,
      TRUE);  // TODO: seems to fail with "SymInitialize: The parameter is incorrect. (87)", at least when it is called a second time?
  checkWin("SymInitialize", SymInitialize(GetCurrentProcess(), nullptr, FALSE));

  {
    _sharedObjectBase =
        (void*)checkWin("SymGetModuleBase",
                        SymGetModuleBase(GetCurrentProcess(), (ptrint)ptr()));
    _hasSharedObjectBase = true;
    _sharedObjectBaseFirstSegment = _sharedObjectBase;

    std::vector<char> filename(128);
    DWORD size;
    while ((size = GetModuleFileNameA((HINSTANCE)_sharedObjectBase,
                                      filename.data(), filename.size())) ==
           filename.size())
      filename.resize(filename.size() * 2);
    checkWin("GetModuleFileNameA", size);

    _hasSharedObject = true;
    _sharedObjectName = std::string(filename.data(), size);
  }

  const size_t nameLen = 4096;
  const size_t size = sizeof(IMAGEHLP_SYMBOL) + nameLen;
  MallocRefHolder<IMAGEHLP_SYMBOL> buffer((IMAGEHLP_SYMBOL*)malloc(size));
  buffer.p->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
  buffer.p->MaxNameLength = nameLen - 1;
  ptrint disp = 0;
  if (SymInitialize(GetCurrentProcess(), 0, TRUE) &&
      SymGetSymFromAddr(GetCurrentProcess(), (ptrint)ptr(), &disp, buffer.p)) {
    _hasSymbol = true;
    _symbolName = buffer.p->Name;
    _symbolAddr = (void*)buffer.p->Address;
    /*
      if (_symbolAddr != (void*) ((char*) ptr () - disp))
        abort ();
      */
  } else {
    _hasSymbol = false;
    _symbolAddr = NULL;
  }
#endif

  _isResolved = true;
}

static std::string escape(const std::string& s) {
  std::stringstream str;
#if OS_UNIX
  str << "'";
  for (size_t i = 0; i < s.length(); i++)
    if (s[i] == '\'')
      str << "'\\''";
    else
      str << s[i];
  str << "'";
#elif OS_WIN
  str << "\"";
  for (size_t i = 0; i < s.length(); i++) {
    SIMPLE_ASSERT(s[i] != '"');
    str << s[i];
  }
  str << "\"";
#else
#error
#endif
  return str.str();
}

void StackFrame::doAddr2line(
    UNUSED const std::lock_guard<std::mutex>& guard) const {
  if (_hasAddr2line) abort();
  if (_inlineStackFrames.size() != 0) abort();

#if OS_WIN
  // Not implemented
  _hasAddr2line = true;
  return;
#endif

  if (!hasSharedObject()) {
    // TODO: Are there cases when this is false?
    _hasAddr2line = true;
    return;
  }

  try {
    // Hack to avoid looking up JITted functions
    if (sharedObjectName().substr(0, 4) == "mono") {
      _hasAddr2line = true;
      return;
    }

    std::vector<InlineStackFrame> frames;

    bool noinfo = false;

    std::stringstream str;

    if (!hasSharedObjectBase())
      throw SimpleStdException("Unable to find shared object");

    str << "addr2line -ife ";
    if (sharedObjectName() != "") {
      //str << sharedObjectName ();
      str << escape(sharedObjectName());
    } else {
      // TODO: Should this ever be the case?
#if OS_UNIX
      str << "/proc/" << getpid() << "/exe";
#elif OS_WIN
      str << "\"" << getCallingFilename() << "\"";
#else
#error
#endif
    }

    // buffer[i] points to the return address, subtract 1 to get an address
    // in the call instruction
    str.setf(std::ios::hex, std::ios::basefield);
    str << " 0x" << sharedObjectOffset() - 1;
    std::string cmd = str.str();
    std::string cmdExpl = " (addr2line invocation was `" + cmd + "')";
    //std::cerr << cmd << std::endl;
    FILE* pipe = popen(cmd.c_str(), "r");
    check("popen", pipe == NULL ? -1 : 0);
    bool eof = false;
    bool r = false;
    while (!eof) {
      std::string line;
      int c;
      c = fgetc(pipe);
      if (c == EOF) {
        eof = true;
        if (!r) {
          int ret = check("pclose", pclose(pipe));
          if (ret > 0) {
            char buf[256] = "";
            snprintf(buf, sizeof(buf), "addr2line returned %d",
                     WEXITSTATUS(ret));
            throw SimpleStdException(buf + cmdExpl);
          }
          throw SimpleStdException(
              "unexpected EOF from addr2line (0 bytes read) + cmdExpl");
        }
      } else {
        if (noinfo) {
          int ret = check("pclose", pclose(pipe));
          if (ret > 0) {
            char buf[256] = "";
            snprintf(buf, sizeof(buf), "addr2line returned %d",
                     WEXITSTATUS(ret));
            throw SimpleStdException(buf + cmdExpl);
          }
          throw SimpleStdException("got line from addr2line after ?? ??:0" +
                                   cmdExpl);
        }

        r = true;
        while (c != EOF && c != '\n') {
          line += (char)c;
          c = fgetc(pipe);
        }
        if (c == EOF) {
          int ret = check("pclose", pclose(pipe));
          if (ret > 0) {
            char buf[256] = "";
            snprintf(buf, sizeof(buf), "addr2line returned %d (mid of line)",
                     WEXITSTATUS(ret));
            throw SimpleStdException(buf + cmdExpl);
          }
          throw SimpleStdException("unexpected EOF from addr2line" + cmdExpl);
        }

        std::string line2;
        c = fgetc(pipe);
        while (c != EOF && c != '\n') {
          line2 += (char)c;
          c = fgetc(pipe);
        }
        if (c == EOF) {
          int ret = check("pclose", pclose(pipe));
          if (ret > 0) {
            char buf[256] = "";
            snprintf(buf, sizeof(buf), "addr2line returned %d (second line)",
                     WEXITSTATUS(ret));
            throw SimpleStdException(buf + cmdExpl);
          }
          throw SimpleStdException("unexpected EOF from addr2line" + cmdExpl);
        }

        if (line == "??" && (line2 == "??:0" || line2 == "??:?")) {
          if (frames.size() != 0) {
            int ret = check("pclose", pclose(pipe));
            if (ret > 0) {
              char buf[256] = "";
              snprintf(buf, sizeof(buf), "addr2line returned %d",
                       WEXITSTATUS(ret));
              throw SimpleStdException(buf + cmdExpl);
            }
            throw SimpleStdException(
                "got ?? ??:0 line from addr2line after other lines" + cmdExpl);
          }
          noinfo = true;
        } else {
          long lineNr;
          std::string sourceFile;
          if (line2 == "??:0" || line2 == "??:?") {
            lineNr = 0;
            sourceFile = "";
          } else {
            size_t pos = line2.rfind(':');
            if (pos == std::string::npos) {
              int ret = check("pclose", pclose(pipe));
              if (ret > 0) {
                char buf[256] = "";
                snprintf(buf, sizeof(buf), "addr2line returned %d",
                         WEXITSTATUS(ret));
                throw SimpleStdException(buf + cmdExpl);
              }
              throw SimpleStdException(
                  "got no line number from addr2line in '" + line2 + "'" +
                  cmdExpl);
            }
            sourceFile = line2.substr(0, pos);
            std::string lineString = line2.substr(pos + 1);

            // Remove " (discriminator 1)" at end of string
            std::size_t lineSpacePos = lineString.find(' ');
            if (lineSpacePos != std::string::npos)
              lineString = lineString.substr(0, lineSpacePos);

            const char* lineStringC = lineString.c_str();
            char* end = NULL;
            lineNr = strtol(lineStringC, &end, 10);
            if (end != lineStringC + lineString.length() ||
                /*lineNr <= 0*/ lineNr < 0) {
              int ret = check("pclose", pclose(pipe));
              if (ret > 0) {
                char buf[256] = "";
                snprintf(buf, sizeof(buf), "addr2line returned %d",
                         WEXITSTATUS(ret));
                throw SimpleStdException(buf + cmdExpl);
              }
              throw SimpleStdException(
                  "got invalid line number from addr2line '" + lineString +
                  "'" + cmdExpl);
            }
          }
          frames.push_back(InlineStackFrame(line, sourceFile, lineNr));
        }
      }
    }
    int ret = check("pclose", pclose(pipe));
    if (ret > 0) {
      char buf[256] = "";
      snprintf(buf, sizeof(buf), "addr2line returned %d (beginning of line)",
               WEXITSTATUS(ret));
      throw SimpleStdException(buf);
    }

    if (!noinfo && frames.size() == 0) {
      throw SimpleStdException("got no data from addr2line\n");
    }

    swap(_inlineStackFrames, frames);
    _hasAddr2line = true;
  } catch (const SimpleStdException& e) {
    std::cerr << "Error while calling addr2line: " << e.what() << std::endl;
    _hasAddr2line = true;
  }
}

static bool lineIsAddress(const std::string& line, size_t addr) {
  // std::cerr << "'" << line << "'" << std::endl;
  if (line.length() < 3)
    return false;
  if (line[0] != '0')
    return false;
  if (line[1] != 'x' && line[1] != 'X')
    return false;

  std::size_t result = 0;
  for (size_t i = 2; i < line.length(); i++) {
    char c = line[i];
    int digit;
    if (c >= '0' && c <= '9') {
      digit = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      digit = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      digit = c - 'A' + 10;
    } else {
      return false;
    }
    std::size_t result2 = result << 4;
    if ((result2 >> 4) != result) {
      // Overflow
      return false;
    }
    result = result2 + (std::size_t)digit;
  }
  // std::cerr << "Got line '" << line << "' result " << result << " should be "
  //           << addr << std::endl;
  return result == addr;
}

// TODO: Merge partially with doAddr2line?
void StackFrame::doAddr2lineBatch(
    const std::string& sharedObjectName,
    const std::vector<const StackFrame*>& frames) {
  // std::cerr << "doAddr2lineBatch " << sharedObjectName << std::endl;
  if (!frames.size()) return;

  try {
    std::stringstream str;

    str << "addr2line -aife " << escape(sharedObjectName);

    std::vector<std::size_t> addresses;
    for (const auto& frame : frames) {
      // buffer[i] points to the return address, subtract 1 to get an address
      // in the call instruction
      str.setf(std::ios::hex, std::ios::basefield);
      std::size_t addr = frame->sharedObjectOffset() - 1;
      str << " 0x" << addr;
      addresses.push_back(addr);
    }
    std::string cmd = str.str();
    std::string cmdExpl = " (addr2line invocation was `" + cmd + "')";
    //std::cerr << cmd << std::endl;
    FILE* pipe = popen(cmd.c_str(), "r");
    check("popen", pipe == NULL ? -1 : 0);
    bool eof = false;
    bool r = false;
    std::vector<std::vector<InlineStackFrame>> inlineFramesRes;
    std::vector<InlineStackFrame> inlineFrames;
    ptrdiff_t current = -1;
    bool noinfo = false;
    while (!eof) {
      std::string line;
      int c;
      c = fgetc(pipe);
      if (c == EOF) {
        eof = true;
        if (!r) {
          int ret = check("pclose", pclose(pipe));
          if (ret > 0) {
            char buf[256] = "";
            snprintf(buf, sizeof(buf), "addr2line returned %d",
                     WEXITSTATUS(ret));
            throw SimpleStdException(buf + cmdExpl);
          }
          throw SimpleStdException(
              "unexpected EOF from addr2line (0 bytes read) + cmdExpl");
        }
      } else {
        if (noinfo) {
          int ret = check("pclose", pclose(pipe));
          if (ret > 0) {
            char buf[256] = "";
            snprintf(buf, sizeof(buf), "addr2line returned %d",
                     WEXITSTATUS(ret));
            throw SimpleStdException(buf + cmdExpl);
          }
          throw SimpleStdException("got line from addr2line after ?? ??:0" +
                                   cmdExpl);
        }

        r = true;
        while (c != EOF && c != '\n') {
          line += (char)c;
          c = fgetc(pipe);
        }
        if (c == EOF) {
          int ret = check("pclose", pclose(pipe));
          if (ret > 0) {
            char buf[256] = "";
            snprintf(buf, sizeof(buf), "addr2line returned %d (mid of line)",
                     WEXITSTATUS(ret));
            throw SimpleStdException(buf + cmdExpl);
          }
          throw SimpleStdException("unexpected EOF from addr2line" + cmdExpl);
        }

        if ((current + 1) < (ptrdiff_t)addresses.size() &&
            lineIsAddress(line, addresses[current + 1])) {
          if (current >= 0) {
            if (!noinfo && inlineFrames.size() == 0) {
              throw SimpleStdException("got no data from addr2line for index " +
                                       std::to_string(current) + cmdExpl);
            }
            inlineFramesRes.push_back(std::move(inlineFrames));
            noinfo = false;
            inlineFrames.clear();
          }
          current += 1;
          continue;
        } else if (current < 0) {
          throw SimpleStdException("did not get address at beginning" +
                                   cmdExpl);
        }

        std::string line2;
        c = fgetc(pipe);
        while (c != EOF && c != '\n') {
          line2 += (char)c;
          c = fgetc(pipe);
        }
        if (c == EOF) {
          int ret = check("pclose", pclose(pipe));
          if (ret > 0) {
            char buf[256] = "";
            snprintf(buf, sizeof(buf), "addr2line returned %d (second line)",
                     WEXITSTATUS(ret));
            throw SimpleStdException(buf + cmdExpl);
          }
          throw SimpleStdException("unexpected EOF from addr2line" + cmdExpl);
        }

        if (line == "??" && (line2 == "??:0" || line2 == "??:?")) {
          if (inlineFrames.size() != 0) {
            int ret = check("pclose", pclose(pipe));
            if (ret > 0) {
              char buf[256] = "";
              snprintf(buf, sizeof(buf), "addr2line returned %d",
                       WEXITSTATUS(ret));
              throw SimpleStdException(buf + cmdExpl);
            }
            throw SimpleStdException(
                "got ?? ??:0 line from addr2line after other lines" + cmdExpl);
          }
          noinfo = true;
        } else {
          long lineNr;
          std::string sourceFile;
          if (line2 == "??:0" || line2 == "??:?") {
            lineNr = 0;
            sourceFile = "";
          } else {
            size_t pos = line2.rfind(':');
            if (pos == std::string::npos) {
              int ret = check("pclose", pclose(pipe));
              if (ret > 0) {
                char buf[256] = "";
                snprintf(buf, sizeof(buf), "addr2line returned %d",
                         WEXITSTATUS(ret));
                throw SimpleStdException(buf + cmdExpl);
              }
              throw SimpleStdException(
                  "got no line number from addr2line in '" + line2 + "'" +
                  cmdExpl);
            }
            sourceFile = line2.substr(0, pos);
            std::string lineString = line2.substr(pos + 1);

            // Remove " (discriminator 1)" at end of string
            std::size_t lineSpacePos = lineString.find(' ');
            if (lineSpacePos != std::string::npos)
              lineString = lineString.substr(0, lineSpacePos);

            const char* lineStringC = lineString.c_str();
            char* end = NULL;
            lineNr = strtol(lineStringC, &end, 10);
            if (end != lineStringC + lineString.length() ||
                /*lineNr <= 0*/ lineNr < 0) {
              int ret = check("pclose", pclose(pipe));
              if (ret > 0) {
                char buf[256] = "";
                snprintf(buf, sizeof(buf), "addr2line returned %d",
                         WEXITSTATUS(ret));
                throw SimpleStdException(buf + cmdExpl);
              }
              throw SimpleStdException(
                  "got invalid line number from addr2line '" + lineString +
                  "'" + cmdExpl);
            }
          }
          inlineFrames.push_back(InlineStackFrame(line, sourceFile, lineNr));
        }
      }
    }
    int ret = check("pclose", pclose(pipe));
    if (ret > 0) {
      char buf[256] = "";
      snprintf(buf, sizeof(buf), "addr2line returned %d (beginning of line)",
               WEXITSTATUS(ret));
      throw SimpleStdException(buf);
    }

    if (current < 0 || current + 1 != (ptrdiff_t)addresses.size()) {
      throw SimpleStdException("Got " + std::to_string(current + 1) +
                               " traces, excpected " +
                               std::to_string(addresses.size()) + cmdExpl);
    }
    if (!noinfo && inlineFrames.size() == 0) {
      throw SimpleStdException("got no data from addr2line for index " +
                               std::to_string(current) + cmdExpl);
    }
    inlineFramesRes.push_back(std::move(inlineFrames));

    if (inlineFramesRes.size() != frames.size()) {
      throw SimpleStdException("inlineFramesRes.size() != frames.size()");
    }
    for (std::size_t i = 0; i < frames.size(); i++) {
      auto frame = frames[i];
      std::lock_guard<std::mutex> guard(frame->_addr2lineMutex);
      if (!frame->_hasAddr2line) {
        swap(frame->_inlineStackFrames, inlineFramesRes[i]);
        frame->_hasAddr2line = true;
      }
    }
  } catch (const SimpleStdException& e) {
    std::cerr << "Error while calling addr2line: " << e.what() << std::endl;
    for (const auto& frame : frames) {
      std::lock_guard<std::mutex> guard(frame->_addr2lineMutex);
      if (!frame->_hasAddr2line) {
        frame->_hasAddr2line = true;
      }
    }
  }
}

void StackFrame::callAddr2lineBatch(
    const std::vector<const StackFrame*>& frames) {
#if OS_WIN
  // Not implemented
  for (const auto& frame : frames) {
    std::lock_guard<std::mutex> guard(frame->_addr2lineMutex);
    if (!frame->_hasAddr2line) {
      frame->_hasAddr2line = true;
    }
  }
  return;
#endif

  std::map<std::string, std::vector<const StackFrame*>> framesBySO;

  for (const auto& frame : frames) {
    std::lock_guard<std::mutex> guard(frame->_addr2lineMutex);
    try {
      if (frame->_hasAddr2line) continue;

      if (!frame->hasSharedObject()) {
        // TODO: Are there cases when this is false?
        frame->_hasAddr2line = true;
        continue;
      }

      // Hack to avoid looking up JITted functions
      if (frame->sharedObjectName().substr(0, 4) == "mono") {
        frame->_hasAddr2line = true;
        continue;
      }

      if (!frame->hasSharedObjectBase())
        throw SimpleStdException("Unable to find shared object");

      std::string soName = frame->sharedObjectName() != ""
                               ? frame->sharedObjectName()
                               :
      // TODO: Should this ever be the case?
#if OS_UNIX
                               "/proc/" + std::to_string(getpid()) + "/exe"
#elif OS_WIN
                               getCallingFilename()
#else
#error
#endif
          ;

      framesBySO[soName].push_back(frame);
    } catch (const SimpleStdException& e) {
      std::cerr << "Error while calling addr2line: " << e.what() << std::endl;
      frame->_hasAddr2line = true;
    }
  }

  if (!framesBySO.size()) return;

  std::vector<std::string> soNames;
  for (const auto& entry : framesBySO) soNames.push_back(entry.first);
  std::sort(soNames.begin(), soNames.end());
  for (const auto& soName : soNames) {
    doAddr2lineBatch(soName, framesBySO[soName]);
  }
}

const StackTrace::CreateFromCurrentThread_t
    StackTrace::createFromCurrentThread =
        StackTrace::CreateFromCurrentThread_t();
StackTrace::StackTrace(UNUSED CreateFromCurrentThread_t ignore) {
  std::vector<void*> buffer(16);
#if OS_UNIX && !defined(__CYGWIN__)
  int nr;
  while ((unsigned int)(nr = backtrace(&buffer.front(),
                                       static_cast<int>(buffer.size()))) ==
         buffer.size()) {
    buffer.resize(buffer.size() * 2);
    assert(buffer.size() <= (unsigned int)std::numeric_limits<int>::max());
  }

  assert(nr >= 0);

  for (int i = 0; i < nr; i++) {
    _frames.push_back(StackFrame(buffer[i]));
  }
#elif OS_WIN
  ptrint eip, esp, ebp;
#ifdef _MSC_VER
  {
    CONTEXT context;
    RtlCaptureContext(&context);
#ifdef _WIN64
    eip = context.Rip;
    esp = context.Rsp;
    ebp = context.Rbp;
#else
    eip = context.Eip;
    esp = context.Esp;
    ebp = context.Ebp;
#endif
  }
#else
  asm("call L_a%= \n\t"
      "L_a%=: pop %0 \n\t"

#ifdef _WIN64
      "mov %%rsp, %1 \n\t"

      "mov %%rbp, %2 \n\t"
#else
         "mov %%esp, %1 \n\t"
         
         "mov %%ebp, %2 \n\t"
#endif
      : "=g"(eip), "=g"(esp), "=g"(ebp));
#endif

#ifdef _WIN64
  STACKFRAME64 frame;
  memset(&frame, 0, sizeof(frame));
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrPC.Offset = eip;
  frame.AddrStack.Mode = AddrModeFlat;
  frame.AddrStack.Offset = esp;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrFrame.Offset = ebp;

  CONTEXT context;
  memset(&context, 0, sizeof(context));

  while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(),
                     GetCurrentThread(), &frame, &context, NULL,
                     SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
    _frames.push_back(StackFrame((void*)frame.AddrPC.Offset));
  }

  /* // StackWalk64 does not set last error according to MSDN
    DWORD err = GetLastError ();
    if (err != ERROR_NOACCESS && err != ERROR_INVALID_ADDRESS) {
      try {
        checkWin ("StackWalk64", 0);
      } catch (const SimpleStdException& e) {
        std::cerr << "Error while creating stack trace: " << e.what () << std::endl;
      }
    }
    */
#else   // !_WIN64
  STACKFRAME frame;
  memset(&frame, 0, sizeof(frame));
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrPC.Offset = eip;
  frame.AddrStack.Mode = AddrModeFlat;
  frame.AddrStack.Offset = esp;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrFrame.Offset = ebp;

  while (StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(),
                   GetCurrentThread(), &frame, NULL, NULL,
                   SymFunctionTableAccess, SymGetModuleBase, NULL)) {
    _frames.push_back(StackFrame((void*)frame.AddrPC.Offset));
  }

  DWORD err = GetLastError();
  if (err != ERROR_NOACCESS && err != ERROR_INVALID_ADDRESS) {
    try {
      checkWin("StackWalk", 0);
    } catch (const SimpleStdException& e) {
      std::cerr << "Error while creating stack trace: " << e.what()
                << std::endl;
    }
  }
#endif  // !_WIN64
#else
  // Do nothing => empty stack trace
#endif
}

std::string StackTrace::toString() const {
  std::stringstream str;

  // Prepopulate inline stack frames by calling addr2line, but only make one addr2line invocation per binary.
  std::vector<const StackFrame*> framePtrs;
  for (const auto& frame : frames()) {
    framePtrs.push_back(&frame);
  }
  StackFrame::callAddr2lineBatch(framePtrs);

  int i = 0;
  size_t addrSize = 0;  //, symbSize = 0, locSize = 0;
  for (std::vector<StackFrame>::const_iterator it = frames().begin();
       it != frames().end(); it++)
    //it->toString (NULL, &addrSize, &symbSize, &locSize);
    it->toString(NULL, &addrSize, NULL, NULL);
  for (std::vector<StackFrame>::const_iterator it = frames().begin();
       it != frames().end(); it++)
    //str << it->toString (&i, &addrSize, &symbSize, &locSize) << std::endl;
    str << it->toString(&i, &addrSize, NULL, NULL) << std::endl;
  return str.str();
}

Exception::~Exception() throw() {}

void Exception::writeTo(std::ostream& stream) const throw() {
  stream << message() << std::endl;

  try {
    stream << stackTrace().toString();
  } catch (std::exception& e) {
    stream << "Error getting stack trace: ";
    stream << e.what();
  } catch (...) {
    stream << "Error getting stack trace";
  }
  stream << std::flush;
}

std::string Exception::toString() const throw() {
  std::stringstream str;
  writeTo(str);
  return str.str();
}

const char* Exception::what() const throw() {
  if (whatValueComputed) return whatValue.c_str();

  whatValue = toString();

  whatValueComputed = true;
  return whatValue.c_str();
}
}  // namespace Core
