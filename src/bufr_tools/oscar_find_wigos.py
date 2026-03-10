import sys


from bufr_tools.getenvvalue import getOscarDumpPath

from bufr_tools.bufresohmsg_py import init_oscar_py  # noqa: E402
from bufr_tools.bufresohmsg_py import oscar_wigos_find_py  # noqa: E402

OSCAR_DUMP = getOscarDumpPath()

init_oscar_py(OSCAR_DUMP)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        for i, wigosId in enumerate(sys.argv):
            if i < 1:
                continue
            msg = oscar_wigos_find_py(wigosId)
            print(msg)
    else:
        print("Usage: python3 oscar_find_wigos.py WigosId(s)")

    exit(0)
