import copy
import json
import os
import sys

from bufr_tools.getenvvalue import (
    getBufrRadarTableDir,
    getOscarDumpPath,
    getEsohSchema,
    getPackageRootDir,
    getRadarCFDir,
)

from bufr_tools.bufresohmsg_py import bufresohmsgmem_py  # noqa: E402
from bufr_tools.bufresohmsg_py import bufrlog_clear_py  # noqa: E402
from bufr_tools.bufresohmsg_py import init_bufr_schema_py  # noqa: E402
from bufr_tools.bufresohmsg_py import init_bufrtables_py  # noqa: E402
from bufr_tools.bufresohmsg_py import init_oscar_py  # noqa: E402
from bufr_tools.bufresohmsg_py import init_radar_cf_py  # noqa: E402
from bufr_tools.bufresohmsg_py import bufr_sdwigos_py  # noqa: E402
from bufr_tools.radar_cf import radar_cf  # noqa: E402

ESOH_SCHEMA = getEsohSchema()
BUFR_TABLE_DIR = getBufrRadarTableDir()
OSCAR_DUMP = getOscarDumpPath()
RODEO_BUFR_DIR = getPackageRootDir()
RADAR_CF_DIR = getRadarCFDir()

init_bufrtables_py(BUFR_TABLE_DIR)
init_bufr_schema_py(ESOH_SCHEMA)
init_oscar_py(OSCAR_DUMP)
init_radar_cf_py(radar_cf)
bufr_sdwigos_py("0-20010-0-0")


def build_all_json_payloads_from_radar_bufr(bufr_content: object) -> list[dict]:
    """
    This function creates the e-soh-message-spec json schema(s) from a BUFR file.

    ### Keyword arguments:
    bufr_file_path (str) -- A BUFR File Path

    Returns:
    str -- mqtt message(s)

    Raises:
    ---
    """
    ret_str = []

    msg_str_list = bufresohmsgmem_py(bufr_content, len(bufr_content))

    for json_str in msg_str_list:
        json_bufr_msg = json.loads(json_str)
        ret_str.append(copy.deepcopy(json_bufr_msg))

    return ret_str


def bufr2mqtt(bufr_file_path: str = "") -> list[str]:
    with open(bufr_file_path, "rb") as file:
        bufr_content = file.read()
    ret_str = bufresohmsgmem_py(bufr_content, len(bufr_content))
    return ret_str


if __name__ == "__main__":
    all_msgs = ""
    if len(sys.argv) > 1:
        all_msgs += "["
        first_msg = True
        for i, file_name in enumerate(sys.argv):
            if i > 0:
                if os.path.exists(file_name):
                    msg = bufr2mqtt(file_name)
                    for m in msg:
                        if first_msg and len(m):
                            first_msg = False
                        else:
                            all_msgs += ","
                        all_msgs += m
                    # print("Print log messages")
                    # for l_msg in bufrlog_py():
                    #    print(l_msg)
                    bufrlog_clear_py()
                else:
                    print("File not exists: {0}".format(file_name))
                    exit(1)
        all_msgs += "]"
        json_msg = json.loads(all_msgs)
        print(json.dumps(json_msg, indent=4))
    else:
        print("Usage: python3 bufr2esomsg.py bufr_file(s)")

    exit(0)
