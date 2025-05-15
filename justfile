# list recipes
default:
    @just --list

set positional-arguments

# Run all docker services. After running the database needs cleanup, run just destroy
all: lint

# Check if the python version is 3.11 for reproducability
_check-python-version:
    #!/usr/bin/env bash
    set -euxo pipefail

    # Get Python version
    if command -v python &>/dev/null; then
        python_version=$(python --version 2>&1 | cut -d ' ' -f2)
    elif command -v python3 &>/dev/null; then
        python_version=$(python3 --version 2>&1 | cut -d ' ' -f2)
    else
        echo "Python not found. Failing..."
        exit 1
    fi

    # Extract major and minor version numbers
    major_version=$(echo "$python_version" | cut -d '.' -f1)
    minor_version=$(echo "$python_version" | cut -d '.' -f2)

    # Check if Python version is greater than or equal to 3.11
    if [[ "$major_version" -lt 3 || ( "$major_version" -eq 3 && "$minor_version" -lt 11 ) ]]; then
        echo "Error: Python version must be greater than or equal to 3.11"
        exit 1
    fi


# Run pip-compile for all the requirement files
pip-compile: _check-python-version
    #!/usr/bin/env bash
    set -euxo pipefail

    find . \
        -iname "*requirements.in" \
        -type f \
        -print \
        -execdir \
        pip-compile --upgrade --no-emit-index-url \
        {} ';'


# ---------------------------------------------------------------------------- #
#                                    test                                      #
# ---------------------------------------------------------------------------- #
# Run the pre-commit hook
lint: _check-python-version
    #!/usr/bin/env bash
    set -euxo pipefail

    if ! command -v pre-commit &> /dev/null; then
        python -m pip install pre-commit
    fi

    pre-commit run --config './.pre-commit-config.yaml' --all-files --color=always --show-diff-on-failure
