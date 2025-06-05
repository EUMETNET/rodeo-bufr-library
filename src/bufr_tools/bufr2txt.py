import os
import sys

from getenvvalue import getEnvValue

RODEO_BUFR_DIR = getEnvValue("RODEO_BUFR_DIR")

sys.path.append(RODEO_BUFR_DIR + "/src/bufr_tools")

from bufresohmsg_py import bufrprint_py  # noqa: E402
from bufresohmsg_py import bufrlog_clear_py  # noqa: E402
from bufresohmsg_py import init_bufrtables_py  # noqa: E402
from bufresohmsg_py import init_oscar_py  # noqa: E402


BUFR_TABLE_DIR = getEnvValue("BUFR_TABLE_DIR", "/usr/share/eccodes/definitions/bufr/tables/0/wmo/")
OSCAR_DUMP = getEnvValue("OSCAR_DUMP", "src/oscar/oscar_stations_all.json")

init_bufrtables_py(BUFR_TABLE_DIR)
init_oscar_py(OSCAR_DUMP)


def bufr2text(bufr_file_path: str = "") -> list[str]:
    ret_str = bufrprint_py(bufr_file_path)
    return ret_str


if __name__ == "__main__":

    if len(sys.argv) > 1:
        for i, file_name in enumerate(sys.argv):
            if i > 0:
                if os.path.exists(file_name):
                    msg = bufr2text(file_name)
                    print(msg)
                    # print("Print log messages")
                    # for l_msg in bufrlog_py():
                    #    print(l_msg)
                    bufrlog_clear_py()
                else:
                    print("File not exists: {0}".format(file_name))
                    exit(1)
    else:
        print("Usage: python3 bufr2txt.py bufr_file(s)")

    exit(0)
