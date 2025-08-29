# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension
from setuptools import setup
from glob import glob
import sys

# __version__ = "0.0.1"

exclude_files_from_build = [
    "src/bufr_tools/bufresohmsgfrenc.cpp",
    "src/bufr_tools/covjson2bufr_main.cpp",
    "src/bufr_tools/bufresohmsg.cpp",
    "src/bufr_tools/bufrenc.cpp",
]

common_extra_compile_args = []
linux_extra_compile_args = ["--std=c++17"]
windows_extra_compile_args = ["-IC:\\temp\\rapidjson\\rapidjson-1.1.0\\include", "/std:c++17"]
if sys.platform == "win32":
    extra_compile_args_for_extension = common_extra_compile_args + windows_extra_compile_args
else:
    extra_compile_args_for_extension = common_extra_compile_args + linux_extra_compile_args

# The main interface is through Pybind11Extension.
# * You can add cxx_std=11/14/17, and then build_ext can be removed.
# * You can set include_pybind11=false to add the include directory yourself,
#   say from a submodule.
#
# Note:
#   Sort input source files if you glob sources to ensure bit-for-bit
#   reproducible builds (https://github.com/pybind/python_example/pull/53)

ext_modules = [
    Pybind11Extension(
        "bufr_tools.bufresohmsg_py",
        sorted([i for i in glob("src/bufr_tools/*.cpp") if i not in exclude_files_from_build]),
        # Example: passing in the version to the compiled code
        extra_compile_args=extra_compile_args_for_extension,
    ),
]

setup(ext_modules=ext_modules)
