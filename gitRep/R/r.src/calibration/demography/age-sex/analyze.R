'MDRTB Calibration of Demographics (age, sex)

# About
Input files are space-delimited text files. Each line represents output 
values from running the simulation for a single year. Each year within 
the same simulation run is line break delimited. Multiple simulation 
scenarios are not delimited, but must be determined logically. For 
example, a 300-year run would show values "0, 2, ... 299" in the 
position of the year parameter for each space-delimited line. After 300
lines of this, the year parameter on the next line would read "0", and
this is how it can logically be determined that the data from this 
point onwards is from a different simulation scenario.

Input values can be referred to here: Driver::runThread_calibDemog; 
also referred to in readData.demog

# Explanations
1. File name
Example:
* Filename: mdrtb.input.dataset MODULE_calibration.demography.age-sex PARAMS_add0-.05by.01_mult1-2by.1.txt
* Meaning: This is a an input file for the "mdrtb" project that is a dataset, for the r src calibration file subsection, demography category, age-sex dynamics module, with a parameter "add" which was sampled at a range of 0 to 0.5 with step size 0.01 and param "mult" with range 1 to 2 with step size 0.1.
 
# Prerequisities
1. Install dependencies
  install.packages("RColorBrewer")
  install.packages("viridis")
  install.packages("here")
2. Make sure everything is set correctly under in analysis-dependent config,
  particularly...
  - `vAdditiveCoefValues` (should match what\'s inside 
`runThread_calibDemog_ageDist` in `Main.cpp`) 
  - `byGender.`
  - `inputFileName`
'
# Dataset: add0_mult1-and-2-32by2*
'This dataset has a parameter "add" (additive mortality coefficient) which was sampled at only at value 0 param "mult" (multiplicative mortality coefficient) sampled at 1 as well as 2-32 where each step is equal to the previous step size value in the series * 2.

We commented out a lot of code that didn\'t apply to this analysis. We only care about the multiplicative param values in this analysis.'


# Imports ####
library("here")

path.module = "r/src/calibration/demography/age-sex"
setwd(paste(here(), path.module, sep="/"))
source("module.config.R")
source("module.functions.R")

# Environment ####
# Static
# The 2nd additive param sample value was actually supposed to be 0.07,
# so we'll that out of this analysis.
input.filename = "mdrtb.input.dataset_MODULE_calibration.demography.age-sex_PARAMS_toBeDetermined"
output.img.filename.base = "mdrtb.plot MODULE_calibration.demography.age-sex FIT_India2000 PARAMS_toBeDetermined"
# TODO:     vector<double> vAddCoef = {0.0, 0.35, 0.07};
# vector<double> vMultCoef = {1.0, 2.0, 4.0, 8.0};
vParams.add = c("TBD")
vParams.mult = c("TBD")
# Calculate
input.path = paste(INPUT_DIR, "/", input.filename, sep="")
vParams.add.base  = vParams.add[1]
vParams.mult.base = vParams.mult[1]
# Load
data.raw = data.load.calibration.demographics(
  input.filename, DATA.FIELD_NAMES)
# Transform
data.summary.mean.byAddMult =
  data.transform.calibration.demographics.add_mult(data.raw)
data.summary.mean.byAdd =
  data.transform.calibration.demographics.add(data.raw)
data.summary.mean.byMult = 
  data.transform.calibration.demographics.mult(data.raw)
# Alias
D = data.raw
Da = data.summary.mean.byAdd
Dm = data.summary.mean.byMult
Dam = data.summary.mean.byAddMult
remove(data.raw)
remove(data.summary.mean.byAdd)
remove(data.summary.mean.byMult)
remove(data.summary.mean.byAddMult)

# Verify ####
# 1) Correct number of unique param values. Comments below represent validity condition for this.
# 2) n values sampled for a given param in given dataset are all equal.
table(D$coefMort2000_add)  # > 1
table(D$coefMort2000_multiply)  # > 1
table(Da$coefMort2000_add)  # > 1
table(Da$coefMort2000_multiply)  # == 1
table(Dam$coefMort2000_add)  # > 1
table(Dam$coefMort2000_multiply)  # > 1
table(Dm$coefMort2000_add)  # == 1
table(Dm$coefMort2000_multiply)  # > 1


# Plot ####
# 

## Scratch #####
# TODO: When ready for the next run, replace toBeDetermined in these 
# files with the name of the actual dataset, and fill out other vals.
