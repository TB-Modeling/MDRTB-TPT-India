'MDRTB Calibration of Demographics (age, sex): history
'
# Imports ####
library("here")

path.module = "r/src/calibration/demography/age-sex"
setwd(paste(here(), path.module, sep="/"))
source("module.config.R")
source("module.functions.R")

# TODO's ####
# - Serialize datasets (save/load) for faster iteration: 
# https://stat.ethz.ch/R-manual/R-devel/library/base/html/load.html
# - time deltas for data load shows '1' second even though takes minutes


# Experimental data ########################################################
# Dataset toBeDetermined ####
# 

# Dataset add0_mult1-and-2-32by2times ####
'This dataset has a parameter "add" (additive mortality coefficient) which was sampled at only at value 0 param "mult" (multiplicative mortality coefficient) sampled at 1 as well as 2-32 where each step is equal to the previous step size value in the series * 2.

We commented out a lot of code that didn\'t apply to this analysis. We only care about the multiplicative param values in this analysis.'


# Imports 
library("here")

path.module = "r/src/calibration/demography/age-sex"
setwd(paste(here(), path.module, sep="/"))
source("module.config.R")
source("module.functions.R")

# Environment 
# Static
# The 2nd additive param sample value was actually supposed to be 0.07,
# so we'll that out of this analysis.
input.filename = "mdrtb.input.dataset_MODULE_calibration.demography.age-sex_PARAMS_add0_mult1-and-2-32by2times"
output.img.filename.base = "mdrtb.plot MODULE_calibration.demography.age-sex FIT_India2000 PARAMS_add0_mult1-and-2-32by2"
# vParams.add = c(0.0, 0.7)  # 0.7 will be filtered out below
vParams.add = c(0.0)
vParams.mult = c(1.0, 2.0, 4.0, 8.0, 16.0, 32.0)
# Calculate
input.path = paste(INPUT_DIR, "/", input.filename, sep="")
vParams.add.base  = vParams.add[1]
vParams.mult.base = vParams.mult[1]
# Load
data.raw = data.load.calibration.demographics(
  input.filename, DATA.FIELD_NAMES)
# Apply fix to remove erroneous 0.7 additive val in the dataset
data.raw = data.raw[data.raw$coefMort2000_add == 0, ]
# Transform
# data.summary.mean.byAddMult = 
#   data.transform.calibration.demographics.add_mult(data.raw)
# data.summary.mean.byAdd = 
#   data.transform.calibration.demographics.add(data.raw)
data.summary.mean.byMult = 
  data.transform.calibration.demographics.mult(data.raw)
# Alias
D = data.raw
# Da = data.summary.mean.byAdd
Dm = data.summary.mean.byMult
# Dam = data.summary.mean.byAddMult
remove(data.raw)
# remove(data.summary.mean.byAdd)
remove(data.summary.mean.byMult)
# remove(data.summary.mean.byAddMult)

# Verify 
# 1) Correct number of unique param values. Comments below represent validity condition for this.
# 2) n values sampled for a given param in given dataset are all equal.
table(D$coefMort2000_add)  # > 1
table(D$coefMort2000_multiply)  # > 1
# table(Da$coefMort2000_add)  # > 1
# table(Da$coefMort2000_multiply)  # == 1
# table(Dam$coefMort2000_add)  # > 1
# table(Dam$coefMort2000_multiply)  # > 1
table(Dm$coefMort2000_add)  # == 1
table(Dm$coefMort2000_multiply)  # > 1

# Additional verification: Checking values not skewed after filtering
Dm1 = Dm[Dm$coefMort2000_multiply == 1,]
length(unlist(Dm1[Dm1$year == 0]))  # 300
length(Dm1$ageF20)  # 300
Dm1y0 = Dm1[Dm1$year == 0, ]
Dm1y0$ageAll0  # 12188.8

# Functions 
# plot.subclass.PARAMS_filterable <- function(
#   D,
#   param.add.factors,
#   param.mult.factors,
#   output.filename,
#   filterMult=TRUE,
#   filterAdd=TRUE
# ) {
#   # Variables 
#   # Static
#   plotType = "l"
#   yAxisLabel = "n Individuals"
#   xAxisLabel = "Age group"
#   title = "Age Bucket Distribution"
#   legendTitle = "Mortality Functions"
#   legend.baseCase = "India 2000 A.D."
#   lineType.baseCase = "dotted"
#   lineType.comparativeCases = "solid"
#   lineWidth = 2
#   color.baseCase = "red"
#   # Calculated
#   output.path = paste(OUTPUT_DIR, "/", output.filename, ".jpeg", sep="")
#   if (!filterMult)
#     param.mult.factors = c(1)
#   if (!filterAdd)
#     param.add.factors = c(0)
#   numLines = length(param.add.factors) * length(param.mult.factors)
#   color.comparativeCases = viridis(numLines)
#   fieldBounds = grep("ageAll", DATA.FIELD_NAMES)
#   
#   # TODO: Dtermine dynamically based on seq(1, length(fieldBounds), x),
#   # Where 'x' is the lowest evenly divisible number of length(fieldBounds)
#   axis.x.ticks = c(1, 5, 9, 13, 17, 21)  # arbitrary / lowest divisible
#   # frequencyMax = 13000
#   frequencyMax = max(D[, fieldBounds])
#   # simulationYearsRan = 499
#   simulationYearsRan = max(D$year) + 1
#   
#   yearsBetweenComparativeCasePlots = 25
#   comparativeCasesYearStart = yearsBetweenComparativeCasePlots
#   comparativeCasesYearsToPlot = seq(
#     comparativeCasesYearStart, 
#     simulationYearsRan, 
#     yearsBetweenComparativeCasePlots)
#   yearsToPlot = c(1, comparativeCasesYearsToPlot)
#   yearsToPlot = yearsToPlot - 1  # Corrects for model code 0-indexing
#   axisTickLabels = 
#     sapply(axis.x.ticks, function(x) CATEGORY.AGE_GROUP.LABELS[x])
#   
#   # Plot: Globals 
#   jpeg(output.path, width=IMG.WIDTH, height=IMG.HEIGHT)
#   # TODO: change to horizontal setup?
#   # TODO: Dynamically determine?
#   par(mfrow=c(5,4))
#   
#   # Plot: Empirical data 
#   # Filter
#   D.base = D[D$year==0, ]
#   if (filterMult)
#     D.base = D.base[D.base$coefMort2000_multiply==1, ]
#   if (filterAdd)
#     D.base = D.base[D.base$coefMort2000_add==0, ]
#   data = unlist(D.base[,fieldBounds])
#   
#   # Visualize
#   plot.plot <- function(year) {
#     year.label = year + 1  # Fix for model code 0-indexing
#     plot(
#       data,ylim=c(0, frequencyMax),
#       xlim=c(0, length(CATEGORY.AGE_GROUP.LABELS)),type=plotType,
#       main=paste(title, " (Year ", year.label, ")"),ylab=yAxisLabel,
#       xlab=xAxisLabel,axes=FALSE,lwd=lineWidth,lty=lineType.baseCase,
#       col=color.baseCase)
#     axis(side=1,at=axis.x.ticks,labels=axisTickLabels)
#     axis(side=2,at=c(1, frequencyMax/2,frequencyMax),labels=c("", "", ""))  
#   }
#   
#   # Plot: Experimental data 
#   lapply(yearsToPlot, function(year) {
#     plot.plot(year)
#     # TODO: Better to not use these globals if possible
#     i = 1  # index for color vector
#     legend.comparativeCases = c()
#     lapply(c(1:length(param.add.factors)), function(c.a) {
#       lapply(c(1:length(param.mult.factors)), function(c.m) {
#         # Variables
#         c.a.val = param.add.factors[c.a]
#         c.m.val = param.mult.factors[c.m]
#         legend.case = paste("x*", c.m.val, " + ", c.a.val, sep="")
#         # Filter
#         D.i = D[D$year==year, ]
#         if (filterMult)
#           D.i = D.i[D.i$coefMort2000_multiply==c.m.val, ]
#         if (filterAdd)
#           D.i = D.i[D.i$coefMort2000_add==c.a.val, ]
#         data = unlist(D.i[,fieldBounds])
#         
#         # browser()
#         
#         # Visualize
#         # TODO: full detail: restore
#         # lines(data)  # minimal detail
#         if (year > 1)  # these lines aren't relevant for year 1
#           lines(  # moderate detail
#             data,type=plotType,lwd=lineWidth,lty=lineType.comparativeCases,
#             col=color.comparativeCases[i])
#         # Update globals
#         i <<- i + 1
#         legend.comparativeCases <<- c(legend.comparativeCases, legend.case)
#       })
#     })
#     # Plot: Legend 
#     # TODO: Dynamically determine ncol
#     if (year <= 1)
#       legend(
#         "topright",legend=c(legend.baseCase, legend.comparativeCases),
#         fill=c(color.baseCase, color.comparativeCases),bty="n",
#         title=legendTitle,ncol=4)
#   })
#   # Plot: Save
#   dev.off()  
# }

# Plot 
plot.FIT_India2000.BY_age.DATASET_add0_mult1and2to32by2times.FILTER_mult(Dm)
plot.FIT_India2000.BY_age.DATASET_add0_mult1and2to8by2times.FILTER_mult(Dm)

# Dataset add0-.05by.01_mult1-2by.1 ####
'This dataset has a parameter "add" (additive mortality coefficient) which was sampled at a range of 0 to 0.5 with step size 0.01 and param "mult" (multiplicative mortality coefficient) with range 1 to 2 with step size 0.'
# Imports 
library("here")

path.module = "r/src/calibration/demography/age-sex"
setwd(paste(here(), path.module, sep="/"))
source("module.config.R")
source("module.functions.R")

# Environment 
# Static
input.filename = "mdrtb.input.dataset_MODULE_calibration.demography.age-sex_PARAMS_add0-.05by.01_mult1-2by.1"
output.img.filename.base = "mdrtb.plot MODULE_calibration.demography.age-sex FIT_India2000 PARAMS_add0-.05by.01_mult1-2by.1"
vParams.add = c(0.00, 0.01, 0.02, 0.03, 0.04, 0.05)
vParams.mult = c(1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0)
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

# Verify 
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

# Functions 
'Plot: Calibration of age and/or sex demographics
Semi-generalized plotting function
@param D(data.frame): Input data'
# plot.subclass.PARAMS_add_mult <- function(
#   D,
#   param.add.factors,
#   param.mult.factors,
#   output.filename,
#   filterMult=TRUE,
#   filterAdd=TRUE
# ) {
#   # Variables 
#   # Static
#   plotType = "l"
#   yAxisLabel = "n Individuals"
#   xAxisLabel = "Age group"
#   title = "Age Bucket Distribution"
#   legendTitle = "Mortality Functions"
#   legend.baseCase = "India 2000 A.D."
#   lineType.baseCase = "dotted"
#   lineType.comparativeCases = "solid"
#   lineWidth = 2
#   color.baseCase = "red"
#   axis.x.ticks = c(1, 5, 9, 13, 17, 21)  # arbitrary
#   # Calculated
#   output.path = paste(OUTPUT_DIR, "/", output.filename, ".jpeg", sep="")
#   numLines = length(param.add.factors) * length(param.mult.factors)
#   color.comparativeCases = viridis(numLines)
#   fieldBounds = grep("ageAll", DATA.FIELD_NAMES)
#   
#   # frequencyMax = 13000
#   frequencyMax = max(D[, fieldBounds])
#   # simulationYearsRan = 499
#   simulationYearsRan = max(D$year) + 1
#   
#   yearsBetweenComparativeCasePlots = 25
#   comparativeCasesYearStart = yearsBetweenComparativeCasePlots
#   comparativeCasesYearsToPlot = seq(
#     comparativeCasesYearStart, 
#     simulationYearsRan, 
#     yearsBetweenComparativeCasePlots)
#   yearsToPlot = c(1, comparativeCasesYearsToPlot)
#   yearsToPlot = yearsToPlot - 1  # Corrects for model code 0-indexing
#   axisTickLabels = 
#     sapply(axis.x.ticks, function(x) CATEGORY.AGE_GROUP.LABELS[x])
#   
#   # Plot: Globals 
#   jpeg(output.path, width=IMG.WIDTH, height=IMG.HEIGHT)
#   par(mfrow=c(5,4))
#   
#   # Plot: Empirical data 
#   # Filter
#   D.base = D[D$year==0, ]
#   if (filterMult)
#     D.base = D.base[D.base$coefMort2000_multiply==1, ]
#   if (filterAdd)
#     D.base = D.base[D.base$coefMort2000_add==0, ]
#   data = unlist(D.base[,fieldBounds])
#   
#   # Visualize
#   plot.plot <- function(year) {
#     year.label = year + 1  # Fix for model code 0-indexing
#     plot(
#       data,ylim=c(0, frequencyMax),
#       xlim=c(0, length(CATEGORY.AGE_GROUP.LABELS)),type=plotType,
#       main=paste(title, " (Year ", year.label, ")"),ylab=yAxisLabel,
#       xlab=xAxisLabel,axes=FALSE,lwd=lineWidth,lty=lineType.baseCase,
#       col=color.baseCase)
#     axis(side=1,at=axis.x.ticks,labels=axisTickLabels)
#     axis(side=2,at=c(1, frequencyMax/2,frequencyMax),labels=c("", "", ""))  
#   }
#   
#   # Plot: Experimental data 
#   lapply(yearsToPlot, function(year) {
#     plot.plot(year)
#     
#     # TODO: temp: try moving legend here; then try to do only for year 1
#     
#     # TODO: Better to not use these globals if possible
#     i = 1  # index for color vector
#     legend.comparativeCases = c()
#     lapply(c(1:length(param.add.factors)), function(c.a) {
#       lapply(c(1:length(param.mult.factors)), function(c.m) {
#         # Variables
#         c.a.val = param.add.factors[c.a]
#         c.m.val = param.mult.factors[c.m]
#         legend.case = paste("x*", c.m.val, " + ", c.a.val, sep="")
#         # Filter
#         D.i = D[D$year==year, ]
#         D.i = D.i[D.i$coefMort2000_multiply==c.m.val, ]
#         D.i = D.i[D.i$coefMort2000_add==c.a.val, ]
#         data = unlist(D.i[,fieldBounds])
#         # Visualize
#         # TODO: full detail: restore
#         # lines(data)  # minimal detail
#         if (year > 1)  # these lines aren't relevant for year 1
#           lines(  # moderate detail
#             data,type=plotType,lwd=lineWidth,lty=lineType.comparativeCases,
#             col=color.comparativeCases[i])
#         # Update globals
#         i <<- i + 1
#         legend.comparativeCases <<- c(legend.comparativeCases, legend.case)
#       })
#     })
#     # Plot: Legend 
#     # TODO: Dynamically determine ncol
#     if (year <= 1)
#       legend(
#         "topright",legend=c(legend.baseCase, legend.comparativeCases),
#         fill=c(color.baseCase, color.comparativeCases),bty="n",
#         title=legendTitle,ncol=4)
#   })
#   # Plot: Save
#   dev.off()  
# }

'Plot: Calibration of age and/or sex demographics
Semi-generalized plotting function
@param D(data.frame): Input data'
# plot.subclass.PARAMS_filterable <- function(
#   D,
#   param.add.factors,
#   param.mult.factors,
#   output.filename,
#   filterMult=TRUE,
#   filterAdd=TRUE
# ) {
#   # Variables 
#   # Static
#   plotType = "l"
#   yAxisLabel = "n Individuals"
#   xAxisLabel = "Age group"
#   title = "Age Bucket Distribution"
#   legendTitle = "Mortality Functions"
#   legend.baseCase = "India 2000 A.D."
#   lineType.baseCase = "dotted"
#   lineType.comparativeCases = "solid"
#   lineWidth = 2
#   color.baseCase = "red"
#   # Calculated
#   output.path = paste(OUTPUT_DIR, "/", output.filename, ".jpeg", sep="")
#   if (!filterMult)
#     param.mult.factors = c(1)
#   if (!filterAdd)
#     param.add.factors = c(0)
#   numLines = length(param.add.factors) * length(param.mult.factors)
#   color.comparativeCases = viridis(numLines)
#   fieldBounds = grep("ageAll", DATA.FIELD_NAMES)
#   
#   # TODO: Dtermine dynamically based on seq(1, length(fieldBounds), x),
#   # Where 'x' is the lowest evenly divisible number of length(fieldBounds)
#   axis.x.ticks = c(1, 5, 9, 13, 17, 21)  # arbitrary / lowest divisible
#   # frequencyMax = 13000
#   frequencyMax = max(D[, fieldBounds])
#   # simulationYearsRan = 499
#   simulationYearsRan = max(D$year) + 1
#   
#   yearsBetweenComparativeCasePlots = 25
#   comparativeCasesYearStart = yearsBetweenComparativeCasePlots
#   comparativeCasesYearsToPlot = seq(
#     comparativeCasesYearStart, 
#     simulationYearsRan, 
#     yearsBetweenComparativeCasePlots)
#   yearsToPlot = c(1, comparativeCasesYearsToPlot)
#   yearsToPlot = yearsToPlot - 1  # Corrects for model code 0-indexing
#   axisTickLabels = 
#     sapply(axis.x.ticks, function(x) CATEGORY.AGE_GROUP.LABELS[x])
#   
#   # Plot: Globals 
#   jpeg(output.path, width=IMG.WIDTH, height=IMG.HEIGHT)
#   # TODO: change to horizontal setup?
#   # TODO: Dynamically determine?
#   par(mfrow=c(5,4))
#   
#   # Plot: Empirical data 
#   # Filter
#   D.base = D[D$year==0, ]
#   if (filterMult)
#     D.base = D.base[D.base$coefMort2000_multiply==1, ]
#   if (filterAdd)
#     D.base = D.base[D.base$coefMort2000_add==0, ]
#   data = unlist(D.base[,fieldBounds])
#   
#   # Visualize
#   plot.plot <- function(year) {
#     year.label = year + 1  # Fix for model code 0-indexing
#     plot(
#       data,ylim=c(0, frequencyMax),
#       xlim=c(0, length(CATEGORY.AGE_GROUP.LABELS)),type=plotType,
#       main=paste(title, " (Year ", year.label, ")"),ylab=yAxisLabel,
#       xlab=xAxisLabel,axes=FALSE,lwd=lineWidth,lty=lineType.baseCase,
#       col=color.baseCase)
#     axis(side=1,at=axis.x.ticks,labels=axisTickLabels)
#     axis(side=2,at=c(1, frequencyMax/2,frequencyMax),labels=c("", "", ""))  
#   }
#   
#   # Plot: Experimental data 
#   lapply(yearsToPlot, function(year) {
#     plot.plot(year)
#     # TODO: Better to not use these globals if possible
#     i = 1  # index for color vector
#     legend.comparativeCases = c()
#     lapply(c(1:length(param.add.factors)), function(c.a) {
#       lapply(c(1:length(param.mult.factors)), function(c.m) {
#         # Variables
#         c.a.val = param.add.factors[c.a]
#         c.m.val = param.mult.factors[c.m]
#         legend.case = paste("x*", c.m.val, " + ", c.a.val, sep="")
#         # Filter
#         D.i = D[D$year==year, ]
#         if (filterMult)
#           D.i = D.i[D.i$coefMort2000_multiply==c.m.val, ]
#         if (filterAdd)
#           D.i = D.i[D.i$coefMort2000_add==c.a.val, ]
#         data = unlist(D.i[,fieldBounds])
#         
#         # browser()
#         
#         # Visualize
#         # TODO: full detail: restore
#         # lines(data)  # minimal detail
#         if (year > 1)  # these lines aren't relevant for year 1
#           lines(  # moderate detail
#             data,type=plotType,lwd=lineWidth,lty=lineType.comparativeCases,
#             col=color.comparativeCases[i])
#         # Update globals
#         i <<- i + 1
#         legend.comparativeCases <<- c(legend.comparativeCases, legend.case)
#       })
#     })
#     # Plot: Legend 
#     # TODO: Dynamically determine ncol
#     if (year <= 1)
#       legend(
#         "topright",legend=c(legend.baseCase, legend.comparativeCases),
#         fill=c(color.baseCase, color.comparativeCases),bty="n",
#         title=legendTitle,ncol=4)
#   })
#   # Plot: Save
#   dev.off()  
# }


# Plot 
plot.FIT_India2000.BY_age.DATASET_add_0toPt5byPt01_mult_1to2byPt1.FILTER_add_mult(Dam)
plot.FIT_India2000.BY_age.DATASET_add_0toPt5byPt01_mult_1to2byPt1.FILTER_mult(Dm)
plot.FIT_India2000.BY_age.DATASET_add_0toPt5byPt01_mult_1to2byPt1.FILTER_add(Da)

# Dataset: 4x6 ####
'This dataset has 4 additive and 6 multiplicative mortality coefficients.'
# Environment
# Static
input.4x6.filename = "demographyResults-4x6"
img.filename = "ageDistCalib-4x6"
vParams.4x6.add = c(0, .065, .07, .075)
vParams.4x6.mult = c(1, 1.025, 1.05, 1.075, 1.1, 1.125)
# Calculate
input.4x6.path = paste(INPUT_DIR, "/", input.4x6.filename, sep="")
vParams.4x6.add.base  = vParams.4x6.add[1]
vParams.4x6.mult.base = vParams.4x6.mult[1]
# Load
data.4x6.raw = data.load.calibration.demographics(
  input.4x6.filename, DATA.FIELD_NAMES)
# Transform
data.4x6.summary.mean.byAddMult = 
  data.transform.calibration.demographics.add_mult(data.4x6.raw)
data.4x6.summary.mean.byAdd = 
  data.transform.calibration.demographics.add(data.4x6.raw)
data.4x6.summary.mean.byMult = 
  data.transform.calibration.demographics.mult(data.4x6.raw)
# Alias
D = data.4x6.raw
Da = data.4x6.summary.mean.byAdd
Dm = data.4x6.summary.mean.byMult
Dam = data.4x6.summary.mean.byAddMult
input.path = input.4x6.path
vParams.add = vParams.4x6.add
vParams.mult = vParams.4x6.mult
vParams.add.base  = vParams.4x6.add.base
vParams.mult.base = vParams.4x6.mult.base
remove(data.4x6.raw)
remove(data.4x6.summary.mean.byAdd)
remove(data.4x6.summary.mean.byMult)
remove(data.4x6.summary.mean.byAddMult)
remove(input.4x6.path)
remove(vParams.4x6.add)
remove(vParams.4x6.mult)
remove(vParams.4x6.add.base)
remove(vParams.4x6.mult.base)
# Verify
#   Unique values
#   - Comments represent validity condition
table(D$coefMort2000_add)  # > 1
table(D$coefMort2000_multiply)  # > 1
table(Da$coefMort2000_add)  # > 1
table(Da$coefMort2000_multiply)  # == 1
table(Dam$coefMort2000_add)  # > 1
table(Dam$coefMort2000_multiply)  # > 1
table(Dm$coefMort2000_add)  # == 1
table(Dm$coefMort2000_multiply)  # > 1
# Plot 
plot.BYage.DATASET4x6.FILTER4add_6mult(Dam)

# Dataset: 6x6 ####
'This dataset has 1 additive and 1 multiplicative mortality param and 
is gendered'
# Static
inputFileName_6x6 = "demographyResults-6x6"
imgFileName_6x6 = "ageDistCalib-6x6"
inputFileName = "demographyResults-6x6"  # add & mult
vAdditiveCoefficients_6x6.set. = c(0, .025, .05, .075, 1.01, 1.125);
vMultCoefficients_6x6.set. = c(1, 1.025, 1.05, 1.075, 1.01, 1.125);
# Load
data.6x6.raw = data.load.calibration.demographics(
  inputFileName, DATA.FIELD_NAMES)
# Transform
data.6x6.summary.mean.byAddMult = 
  data.transform.calibration.demographics.add_mult(data.6x6.raw)
data.6x6.summary.mean.byAdd = 
  data.transform.calibration.demographics.add(data.6x6.raw)
data.6x6.summary.mean.byMult = 
  data.transform.calibration.demographics.mult(data.6x6.raw)
# Summarize
summarize.data.model(data.raw)
summarize.data.empirical(data.raw)
# Functions ####
# Snapshot 1: plot.subclass.PARAMS_add_mult 2019/01/16 10:30pm
# plot.subclass.PARAMS_add_mult <- function(
# D,
# param.add.factors,
# param.mult.factors,
# output.filename
# ) {
#   # Variables 
#   # Static
#   plotType = "l"
#   yAxisLabel = "n Individuals"
#   xAxisLabel = "Age group"
#   title = "Age Bucket Distribution"
#   legendTitle = "Mortality Functions"
#   legend.baseCase = "India 2000 A.D."
#   lineType.baseCase = "dotted"
#   lineType.comparativeCases = "solid"
#   lineWidth = 2
#   color.baseCase = "yellow"
#   axis.x.ticks = c(1, 5, 9, 13, 17, 21)  # arbitrary
#   # Calculated
#   output.path = paste(OUTPUT_DIR, "/", output.filename, ".jpeg", sep="")
#   numLines = length(param.add.factors) * length(param.mult.factors)
#   color.comparativeCases = viridis(numLines)
#   # TODO: Also calculate these from dataset
#   frequencyMax = 13000
#   simulationYearsRan = 499
#   yearsBetweenEachPlot = 25
#   yearsToPlot = seq(0, simulationYearsRan, yearsBetweenEachPlot)
#   axisTickLabels = 
#     sapply(axis.x.ticks, function(x) CATEGORY.AGE_GROUP.LABELS[x])
#   
#   # Plot: Globals 
#   jpeg(output.path, width=IMG.WIDTH, height=IMG.HEIGHT)
#   par(mfrow=c(5,4))
#   
#   # Plot: Empirical data
#   # Filter
#   D.base = D[D$year==0, ]
#   D.base = D.base[D.base$coefMort2000_multiply==1, ]
#   D.base = D.base[D.base$coefMort2000_add==0, ]
#   fieldBounds = grep("ageAll", DATA.FIELD_NAMES)
#   data = unlist(D.base[,fieldBounds])
#   # Visualize
#   plot.year <- function(year) {
#     plot(
#       data,ylim=c(0, frequencyMax),
#       xlim=c(0, length(CATEGORY.AGE_GROUP.LABELS)),type=plotType,
#       main=paste(title, " (Year ", year, ")"),ylab=yAxisLabel,
#       xlab=xAxisLabel,axes=FALSE,lwd=lineWidth,lty=lineType.baseCase,
#       col=color.baseCase)
#     axis(side=1,at=axis.x.ticks,labels=axisTickLabels)
#     axis(side=2,at=c(1, frequencyMax/2,frequencyMax),labels=c("", "", ""))
#   }
#   
#   # Plot: Experimental data 
#   lapply(yearsToPlot, function(year) {
#     plot.year(year)
#     i = 1  # index for color vector
#     legend.comparativeCases = c()
#     lapply(c(1:length(param.add.factors)), function(c.a) {
#       lapply(c(1:length(param.mult.factors)), function(c.m) {
#         # Variables
#         c.a.val = param.add.factors[c.a]
#         c.m.val = param.mult.factors[c.m]
#         legend.case = paste("x*", c.m.val, " + ", c.a.val, sep="")
#         # Filter
#         D.i = D[D$year==year, ]
#         D.i = D.i[D.i$coefMort2000_multiply==c.m.val, ]
#         D.i = D.i[D.i$coefMort2000_add==c.a.val, ]
#         data = unlist(D.i[,fieldBounds])
#         # Visualize
#         # TODO: Restore full detail
#         # lines(data)
#         lines(
#           data,type=plotType,lwd=lineWidth,lty=lineType.comparativeCases,
#           col=color.comparativeCases[i])
#         # Update globals
#         i <<- i + 1
#         legend.comparativeCases <<- c(legend.comparativeCases, legend.case)
#         
#         # TODO: Debugging: does every line have the same vals?
#         # print(paste("year: ", year, " line num: ", i-1, " data.len: ", length(data)))
#         # browser()
#       })
#     })
#     # Plot: Legend 
#     legend(
#       "topright",legend=c(legend.baseCase, legend.comparativeCases),
#       fill=c(color.baseCase, color.comparativeCases),bty="n",
#       title=legendTitle)
#   })
#   # Plot: Save
#   dev.off()  
# }

# Plot
plot._BY.age_FILTER.1add0pt075.female(data.6x6.summary.mean.byAdd)
plot._BY.age_FILTER.6mult(data.6x6.summary.mean.byMult)
plot._BY.age_FILTER.1add0pt075.male(data.6x6.summary.mean.byAdd)
plot._BY.age.sex_FILTER.1add0pt07(data.6x6.summary.mean.byMult)
plot._BY.age_FILTER.6add_6mult(data.6x6.summary.mean.byAddMult)
plot._BY.age_FILTER.6mult(data.6x6.summary.mean.byMult)

# Dataset: 1x1 0.7 ####
'This dataset has 1 additive mortality param and is ungendered'
inputFileName_1x1 = "demographyResults-0.7"
inputFileName = "demographyResults-0.7"
data.0pt07.raw = data.load.calibration.demographics(
  inputFileName, DATA.FIELD_NAMES.UNGENDERED)
data.0pt07.summary.mean.byAdd = 
  data.transform.calibration.demographics.add(data.0pt07.raw)

# Dataset: 4x4 ####
inputFileName_4x4 = "demographyResults-4x4"
inputFileName_4x4gender = "demographyResults-4x4-ByGender"
imgFileName_4x4 = "ageDistCalib-4x4"
vAdditiveCoefficients_4x4.set. = c(0, .065, .07, .075)
vMultCoefficients_4x4.set. = c(1, 1.05, 1.1, 1.3);

# Dataset: 1x4
vAdditiveCoefficients_1x4.set. = c(0, 0.065, 0.07, .075)
vMultCoefficients_1x4.set. = c();

# Dataset: Unnamed previous datasets ####
plot.something(data.summary.mean.byMult)  # broken: shows no image
plot.populationGrowth(data.summary.mean.byMult)  # broken?
plot.hhDist(data.summary.mean.byMult)  # borken?


# Empirical data #########################################################
# India 2000-2016 demographics ####
'In this particular analysis, we used year 0 of experimental data because it
was set in the model to mirror '
summarize.data.empirical(data.6x6.summary.mean.byAdd)
plot.empirical.y2000.male_vs_female(data.6x6.summary.mean.byAdd)

