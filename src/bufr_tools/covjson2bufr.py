import os
import sys

from bufr_tools.getenvvalue import getBufrTableDir, getPackageRootDir
from bufr_tools.bufresohmsg_py import covjson2bufr_py  # noqa: E402

BUFR_TABLE_DIR = getBufrTableDir()
RODEO_BUFR_DIR = getPackageRootDir()


def covjson2bufr(cov_json: str = "", bufr_schema: str = "default"):
    """
    This function creates the binary BUFR message from E-SOH coverage json.

    ### Keyword arguments:
    cov_json (str) -- A coverage json
    bufr_schema (str) -- Specify the BUFR message template/sequences

    Returns:
    binary -- BUFR message

    Raises:
    ---
    """
    if bufr_schema == "default":
        return covjson2bufr_py(cov_json, bufr_schema)
    return None


def covfile2bufr(cov_file_path: str = "", bufr_schema: str = "default"):
    """
    This function creates the binary BUFR message from E-SOH cov json file.

    ### Keyword arguments:
    cov_file_path (str) -- A coverage json
    bufr_schema (str) -- Specify the BUFR message template/sequences

    Returns:
    binary -- BUFR message

    Raises:
    ---
    """
    with open(cov_file_path, "rb") as file:
        coverage_str = file.read()
    # print(coverage_str);
    if bufr_schema == "default":
        return covjson2bufr(coverage_str, bufr_schema)
    return None


if __name__ == "__main__":
    all_msgs = ""

    if len(sys.argv) > 1:
        all_msgs += "["
        first_msg = True
        for i, file_name in enumerate(sys.argv):
            if i > 0:
                if os.path.exists(file_name):
                    bufr_content = covfile2bufr(file_name)
                    with open("test_out.bufr", "wb") as file:
                        file.write(bufr_content)
                else:
                    print("File not exists: {0}".format(file_name))
                    exit(1)
    else:
        print("Usage: python3 covjson2bufr.py coverage.json")

    exit(0)
