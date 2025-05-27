import os
import sys


def getEnvValue(val_name: str, default_suffix: str = "") -> str:

    val = os.getcwd() + default_suffix
    val_env = ""
    try:
        val_env = os.environ[val_name]
    except Exception:
        pass

    if len(val_env):
        val = val_env
        if val_name == "RODEO_BUFR_DIR" and val[-1:] != "/":
            val += "/"
    else:
        if val_name != "RODEO_BUFR_DIR" and len(RODEO_BUFR_DIR):
            if default_suffix[:1] == "/":
                val = default_suffix
            else:
                val = RODEO_BUFR_DIR + "/" + default_suffix

    # print("ENV: {0} -> {1}".format(val_name, val))
    return val


RODEO_BUFR_DIR = getEnvValue("RODEO_BUFR_DIR")
sys.path.append(RODEO_BUFR_DIR + "/bufr/src")

from bufresohmsg_py import covjson2bufr_py  # noqa: E402

BUFR_TABLE_DIR = getEnvValue("BUFR_TABLE_DIR", "/usr/share/eccodes/definitions/bufr/tables/0/wmo/")


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
        return covjson2bufr_py(cov_json)
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
