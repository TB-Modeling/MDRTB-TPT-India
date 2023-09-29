'Config'
library("here")

ROOT_DIR <- here()
SRC_DIR <- paste(ROOT_DIR,"/r/src", sep="")
INPUT_DIR <- paste(ROOT_DIR,"/r/input", sep="")
STATIC_DIR <- paste(ROOT_DIR,"/r/static", sep="")
OUTPUT_DIR <- paste(ROOT_DIR,"/r/output", sep="")

setwd(SRC_DIR)
