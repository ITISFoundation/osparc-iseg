#!/bin/bash

dir="${ISEG_ROOT}"
hex=`printf ${dir} | od -A n -t x1 | tr -d '[\n\t ]'`
code --folder-uri="vscode-remote://dev-container%2B${hex}/data/osparc-iseg"