import os
from importlib.resources import files


def getEnvValue(val_name: str, default_suffix: str = "") -> str:
    global RODEO_BUFR_DIR
    val = os.getcwd() + default_suffix
    val_env = ""
    try:
        val_env = os.environ[val_name]
    except Exception:
        pass

    if len(val_env):
        val = val_env
        if val_name == "RODEO_BUFR_DIR":
            if val[-1:] != "/":
                val += "/"
            RODEO_BUFR_DIR = val
    else:
        if val_name != "RODEO_BUFR_DIR" and len(RODEO_BUFR_DIR):
            if default_suffix[:1] == "/":
                val = default_suffix
            else:
                val = RODEO_BUFR_DIR + "/" + default_suffix

    # print("ENV: {0} -> {1}".format(val_name, val))
    return val


def getBufrTableDir() -> str:
    """
    Get BUFR table directory from environment variable or default path.
    """
    return os.getenv("BUFR_TABLE_DIR", "/usr/share/eccodes/definitions/bufr/tables/0/wmo/")


def getPackageRootDir() -> str:
    packge_path = files("bufr_tools")
    return f"{packge_path}"


def getOscarDumpPath() -> str:
    return getPackageRootDir() + "/data/oscar/oscar_stations_all.json"


def getEsohSchema() -> str:
    return getPackageRootDir() + "/data/schemas/bufr_to_e_soh_message.json"


def getRadarCFDir() -> str:
    return getPackageRootDir() + "/data/radar/radar_cf.py"


def getBufrRadarTableDir() -> str:
    """
    Get BUFR table directory from environment variable or from package.
    """
    radar_table_dir = os.getenv("BUFR_TABLE_DIR")
    if radar_table_dir is None or len(radar_table_dir) < 2:
        radar_table_dir = getPackageRootDir() + "/data/tables/opera/"
    return radar_table_dir
