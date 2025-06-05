import os


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
