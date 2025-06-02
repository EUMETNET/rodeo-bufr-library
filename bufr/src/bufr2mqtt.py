import copy
import json
import os
import sys

from getenvvalue import getEnvValue

RODEO_BUFR_DIR = getEnvValue("RODEO_BUFR_DIR")
sys.path.append(RODEO_BUFR_DIR + "/bufr/src")

from bufresohmsg_py import bufresohmsgmem_py  # noqa: E402
from bufresohmsg_py import bufrlog_clear_py  # noqa: E402
from bufresohmsg_py import bufrlog_py  # noqa: E402
from bufresohmsg_py import init_bufr_schema_py  # noqa: E402
from bufresohmsg_py import init_bufrtables_py  # noqa: E402
from bufresohmsg_py import init_oscar_py  # noqa: E402

BUFR_TABLE_DIR = getEnvValue("BUFR_TABLE_DIR", "/usr/share/eccodes/definitions/bufr/tables/0/wmo/")

init_bufrtables_py(BUFR_TABLE_DIR)
init_oscar_py(RODEO_BUFR_DIR + "/bufr/oscar/oscar_stations_all.json")
init_bufr_schema_py(RODEO_BUFR_DIR + "/bufr/schemas/bufr_to_e_soh_message.json")


def build_all_json_payloads_from_bufr(bufr_content: object) -> list[str]:
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
    # ret_str = bufresohmsg_py(bufr_file_path)
    with open(bufr_file_path, "rb") as file:
        bufr_content = file.read()
    # print(type(bufr_content))
    print(len(bufr_content))
    ret_str = bufresohmsgmem_py(bufr_content, len(bufr_content))
    return ret_str


if __name__ == "__main__":
    test_path = "./test/test_data/bufr/SYNOP_BUFR_2718.bufr"
    test_schema_path = "./src/ingest/schemas/bufr_to_e_soh_message.json"
    msg = ""

    init_bufr_schema_py("./src/ingest/schemas/bufr_to_e_soh_message.json")

    if len(sys.argv) > 1:
        for i, file_name in enumerate(sys.argv):
            if i > 0:
                if os.path.exists(file_name):
                    msg = bufr2mqtt(file_name)
                    for m in msg:
                        print(m)
                    print("Print log messages")
                    for l_msg in bufrlog_py():
                        print(l_msg)
                    bufrlog_clear_py()
                else:
                    print("File not exists: {0}".format(file_name))
                    exit(1)

    else:
        msg = bufr2mqtt(test_path)
        for m in msg:
            print(m)

    exit(0)
