'MDRTB Calibration of Demographics (age, sex): Config

Static configuration variables for the analysis, applicable to every
experimental dataset.

Exlplanation of variable naming
- CASE.ALL_CAPS = Constants
- .myVar = Private calculation variable
'
# Imports ####
library("here")

path.src = "r/src/"
setwd(paste(here(), path.src, sep="/"))
source("config.R")
source("functions.R")

path.module = "r/src/calibration/demography/age-sex"
setwd(paste(here(), path.module, sep="/"))


# Config variables ##########################################################
# Empirical data
.staticDataFileName_PopByAgeSex = "0-PopByAgeSex.csv"
STATIC_DATA.PopByAgeSex.PATH = paste(
  STATIC_DIR, "/", .staticDataFileName_PopByAgeSex, sep="")

# Fields and factors
CATEGORY.AGE_GROUP.LABELS=c(
  "0-4",
  "5-9",
  "10-14",
  "15-19 ",
  "20-24",
  "25-29",
  "30-34",
  "35-39",
  "40-44",
  "45-49",
  "50-54",
  "55-59",
  "60-64",
  "65-69",
  "70-74",
  "75-79",
  "80-84",
  "85-89",
  "90-94",
  "95-99",
  "100-104")

# Dataset field names
.numAgeBuckets = length(CATEGORY.AGE_GROUP.LABELS)
.ageFieldSuffices = c(0:(.numAgeBuckets-1))
FACTORS.FIELD_NAMES = c("coefMort2000_multiply","coefMort2000_add")
DATA.FIELD_NAMES.UNGENDERED = c(
  "tid","year","pop","prpMale","medAge","medHHsize",
  paste("ageAll",.ageFieldSuffices,sep = ""),
  paste("HH",seq(0,10,1),sep = ""),
  "totalHH","deaths",
  FACTORS.FIELD_NAMES)
DATA.FIELD_NAMES = c(
  DATA.FIELD_NAMES.UNGENDERED, 
  paste("ageM",.ageFieldSuffices,sep = ""),
  paste("ageF",.ageFieldSuffices,sep = ""))

# Plotting
IMG.WIDTH = 1500
IMG.HEIGHT = 1500

