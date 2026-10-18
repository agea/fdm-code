"""Independent copy-on-write files on APFS, with ordinary copy fallback."""
import ctypes
import errno
import os
from pathlib import Path
import shutil
import tempfile

_lib = ctypes.CDLL('/usr/lib/libSystem.B.dylib', use_errno=True)
_clone = _lib.clonefile
_clone.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
_clone.restype = ctypes.c_int


def clone_file(src, dst):
    if _clone(os.fsencode(src), os.fsencode(dst), 0) != 0:
        code = ctypes.get_errno()
        raise OSError(code, os.strerror(code), os.fspath(src))
    return os.fspath(dst)


def clone_supported(source, destination_directory):
    with tempfile.TemporaryDirectory(prefix='.rr-clone-probe-', dir=destination_directory) as probe:
        try:
            clone_file(source, Path(probe) / 'probe')
        except OSError:
            return False
    return True


def copy_file(src, dst):
    try:
        return clone_file(src, dst)
    except OSError as error:
        if error.errno not in (errno.EXDEV, errno.ENOTSUP, errno.EINVAL, errno.EPERM):
            raise
        return shutil.copy2(src, dst)
