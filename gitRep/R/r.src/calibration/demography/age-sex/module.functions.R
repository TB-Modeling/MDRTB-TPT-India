'MDRTB Calibration of Demographics (age, sex): Functions

# Explanation: Subsectioning
- Module static: Module-specific, dataset independent
- Datasets: Dataset dependent; Experimental datasets with some empirical
datasets/data as well
'
# Imports ####
library("data.table")
library("here")
library("RColorBrewer")
library("viridis")

path.src = "r/src/"
setwd(paste(here(), path.src, sep="/"))
source("config.R")
source("functions.R")

path.module = "r/src/calibration/demography/age-sex"
setwd(paste(here(), path.module, sep="/"))
source("module.config.R")


# TODO's ####
# - Move some of these to general functions; but will need to all variables
# to be passed / have default values


# Module static: Load ####
data.load.calibration.demographics <- function(
  fileName=inputFileName,
  fields=DATA.FIELD_NAMES
) {
  inputPath = paste(INPUT_DIR, "/", fileName, sep="")
  
  start_time = Sys.time()
  print(paste("Reading file:", inputPath))
  
  D = readData.demog(
    name=inputPath,
    fields=fields)
  
  end_time = Sys.time()
  elapsed = time.elapsed.seconds(end_time - start_time)
  print(paste("Complete (", elapsed, " seconds)", sep=""))  
  return(D)
}

# Module static: Transform ####
'Transform into summary, additive mortality coefficient'
data.transform.calibration.demographics.add <- function(D) {
  start_time = Sys.time()
  print("Transforming to summarized dataset...")
  
  DS = summarize.mean.year.factor(D, "coefMort2000_add")
  
  end_time = Sys.time()
  elapsed = time.elapsed.seconds(end_time - start_time)
  print(paste("Complete (", elapsed, " seconds)", sep=""))  
  return(DS)
}

'Transform into summary, multiplicative mortality coefficient'
data.transform.calibration.demographics.mult <- function(D) {
  start_time = Sys.time()
  print("Transforming to summarized dataset...")
  
  DS = summarize.mean.year.factor(D, "coefMort2000_multiply")
  
  end_time = Sys.time()
  elapsed = time.elapsed.seconds(end_time - start_time)
  print(paste("Complete (", elapsed, " seconds)", sep=""))  
  return(DS)
}

'Transform into summary, multipicative & additive mortality coefficients'
data.transform.calibration.demographics.add_mult <- function(D) {
  start_time = Sys.time()
  print("Transforming to summarized dataset...")
  
  DS = summarize.mean.year.2factors(
    D, "coefMort2000_add", "coefMort2000_multiply")
  
  end_time = Sys.time()
  elapsed = time.elapsed.seconds(end_time - start_time)
  print(paste("Complete (", elapsed, " seconds)", sep=""))
  return(DS)
}

# Module static: Summarize ####
'Summarize simulation data
@param D(data.frame): Input data'
summarize.data.model <- function(D) {
  print("# Summarizing dataset")
  print("")
  # print("First row: ")
  # print("D[1,]")
  # print(D[1,])
  # print("")
  
  # TODO: summarize.quantile.year<-function(D,q){
  # for ex: summarize.quantile.year(D, 0.25), and check the distance
  # large distance means more stochastic. and there is a way to check
  # how 
  
  print("## Summarizing columns")
  print("top number: x, Value within the column")
  print("bottom number: n, Number of occurrences of value")
  print("")
  
  
  print("### No transformation")
  print("table(D$coefMort2000_add)")
  print(table(D$coefMort2000_add))
  print("table(D$coefMort2000_multiply)")
  print(table(D$coefMort2000_multiply))
  print("table(D$coefMort2000_multiply)")
  print("table(D$coefMort2000_add, D$coefMort2000_multiply)")
  table(D$coefMort2000_add, D$coefMort2000_multiply)
  print("")
  
  
  print("### Transformation: summarize.mean.year.factor = add")
  print("START: D.a<-summarize.mean.year.factor(D, 
    factorName='coefMort2000_add')")
  start_time <- Sys.time()
  D.a <- summarize.mean.year.factor(D, factorName="coefMort2000_add")
  end_time <- Sys.time()
  elapsed = time.elapsed.seconds(end_time - start_time)
  print(paste("RESULT (", elapsed, " seconds): "))
  print("table(D.a$coefMort2000_add)")
  print(table(D.a$coefMort2000_add))
  print("table(D.a$coefMort2000_multiply)")
  print(table(D.a$coefMort2000_multiply))
  print("")

  print("### Transformation: summarize.mean.year.factor = multiply")
  print("START: D.m<-summarize.mean.year.factor(D, 
    factorName='coefMort2000_multiply')")
  D.m<-summarize.mean.year.factor(D, factorName="coefMort2000_multiply")
  print("RESULT: ")
  print("table(D.m$coefMort2000_add)")
  print(table(D.m$coefMort2000_add))
  print("table(D.m$coefMort2000_multiply)")
  print(table(D.m$coefMort2000_multiply))
  print("")
}

'Summarize: Empirical data, population by age/sex in India
@param D(data.frame): Input data'
summarize.data.empirical <- function(D) {
  # https://population.un.org/wpp/DataQuery/
  vCols=viridis(70)
  D<-read.csv(STATIC_DATA.PopByAgeSex.PATH,header = T)
  D[1,]
  # par(mfrow=c(5,7));#
  par(mfrow=c(1,1));
  year=1990
  plot(
    as.numeric(D[,(year-1990+5)]),
    type="l",
    ylim=c(0,70000),
    main="Population by age/sex (female/male)",
    ylab="Population by age/sex (thousands)",
    xaxt="n")
  axis(
    side=1,
    at=c(1:42),
    labels=c(CATEGORY.AGE_GROUP.LABELS,CATEGORY.AGE_GROUP.LABELS))
  
  lapply(c(1991:2050),function(year){
    lines(
      as.numeric(D[,(year-1990+5)]),
      main=year,
      type="l",
      col=vCols[year-1990])
  })
  legend(
    "topright",
    legend=c(1990:2050),
    fill=c("black",vCols[1:50]),bty = "n",title = "Year")
  
  ## Total population size:
  names(D)[65]
  b<-plot(
    colSums(D[,c(5:65)]),
    main="Population ",
    ylab="Total population (thousands)",
    xaxt="n",
    xlab="year")
  axis(
    side=1,
    at=c(0:60),
    labels=c(1990:2050))  
  b
}


# Module static: Plot Base ####
'Plot: Calibration of age and/or sex demographics
Advanced base class for calibrating for additive and/or multiplicative mortality coefficients, by gender or not, and with additional filters
@param D(data.frame): Input data'
plot.base <- function(
  D,
  img.width=IMG.WIDTH,
  img.height=IMG.HEIGHT,
  factors.fieldNames=FACTORS.FIELD_NAMES,
  baseCase.add=0,
  baseCase.mult=1,
  coefficients.add.=vAdditiveCoefficients.set.,
  coefficients.mult.=vMultCoefficients.set.,
  frequencyMax=13000,  # TODO: extrapolate from data
  simulationYearsRan=499,
  yearsBetweenEachPlot=25,
  plotType="l",
  # yAxisLabel="Frequency",
  yAxisLabel="n Individuals",
  xAxisLabel="Age group",
  title.static="Age Bucket Distribution",
  legendTitle="Mortality Functions",
  imgFileName=imgFileName.,
  legend.baseCase="India, 2000 A.D.",
  lineType.baseCase="dotted",
  lineType.comparativeCases="solid",
  lineWidth=2,
  ageGroupNames=CATEGORY.AGE_GROUP.LABELS,
  byGender=FALSE,
  filterFemale=FALSE,
  filterMale=FALSE,
  numAgeBuckets=length(CATEGORY.AGE_GROUP.LABELS),
  color.male="red",
  color.female="blue",
  color.baseCase.whenStatic="yellow",
  numBaseCases=1,
  nLinesWhenChangeToStaticBaseCaseColor=8,  # arbitrary
  axisTickAgeBuckets=c(1, 5, 9, 13, 17, 21)  # arbitrary
) {
  # Validate
  mutuallyExclusiveVars.sex = c(byGender, filterFemale, filterMale)
  gender.nOptions = sum(as.numeric(mutuallyExclusiveVars.sex))
  analysis.isGendered = gender.nOptions > 0
  if (gender.nOptions > 1) {
    msg = paste(
      "Exception mutuallyExclusiveVars.sex: \n", "Only 1 of the 3 ", 
      "variables byGender, filterFemale, and filterMale can be set to", 
      "TRUE.", sep="")
    stop(msg)
  }
  
  # Setup: plot vals
  jpeg(paste(OUTPUT_DIR, "/", imgFileName, ".jpeg", sep=""), 
       width=img.width, height=img.height)
  par(mfrow=c(5,4))  # par(mfrow=c(1,1))
  
  # Setup: Precalculations
  has.add = isTRUE(grep(factors.fieldNames, "coefMort2000_add") > 0)
  has.mult = isTRUE(grep(factors.fieldNames, "coefMort2000_multiply") > 0)
  coefficients.add = coefficients.add.
  coefficients.mult = coefficients.mult.
  if (!has.add)
    coefficients.add = baseCase.add
  if (!has.mult)
    coefficients.mult = baseCase.mult
  
  numLines = length(coefficients.add) * length(coefficients.mult)
  
  color.baseCase.isStatic = 
    numLines > nLinesWhenChangeToStaticBaseCaseColor
  if (color.baseCase.isStatic) {
    color.baseCase = color.baseCase.whenStatic
    color.comparativeCases = viridis(numLines)
  } else {
    img.colorPalette = viridis(numLines + numBaseCases)
    color.baseCase = img.colorPalette[1]
    color.comparativeCases = img.colorPalette[2:length(img.colorPalette)]
  }
  
  if (analysis.isGendered)
    frequencyMax = frequencyMax / 2
  
  axisTickLabels = 
    sapply(axisTickAgeBuckets, function(x) ageGroupNames[x])
  
  yearsToPlot = seq(0, simulationYearsRan, yearsBetweenEachPlot)
  
  # Plot
  lapply(yearsToPlot, function(year) {
    title = paste(title.static, " (Year ", year, ")")
    
    D2 = D[D$year==0, ]
    if (has.mult)
      D2 = D2[D2$coefMort2000_multiply==baseCase.mult, ]
    if (has.add)
      D2 = D2[D2$coefMort2000_add==baseCase.add, ]
    
    # TODO: Modify factor-related parameters such as factor field name,
    # factor base case, list of factors, to allow dynamism. Allow several,
    # as part of a series of ifs, or do it recursively.
    # x1 = D[D$year==0, ]
    # if (length(factors.fieldNames) >= 1)
    #   x2 = x1[x1[[factors.fieldNames[1]]]=="???", ]
    # if (length(factors.fieldNames) >= 2)
    #   x3 = x2[x2[[factors.fieldNames[2]]]=="???", ]
    
    # Plot: Base case
    fieldBounds = NULL
    baseCaseData = NULL
    
    # TODO: Gender-based dist looks off; isn't year0 experimental data
    # the same as empirical? Check exp first, then empirical
    if (!analysis.isGendered)
      fieldBounds = grep("ageAll", DATA.FIELD_NAMES)
    if (filterFemale)
      fieldBounds = grep("ageF", DATA.FIELD_NAMES)
    if (filterMale)
      fieldBounds = grep("ageM", DATA.FIELD_NAMES)
    if (!is.null(fieldBounds))
      baseCaseData = unlist(D2[,fieldBounds])
    
    plot(
      baseCaseData,
      ylim=c(0, frequencyMax),
      xlim=c(0, length(CATEGORY.AGE_GROUP.LABELS)),
      type=plotType,
      main=title,
      # sub="",  # this actually shows beneath the x axis lab
      ylab=yAxisLabel,
      xlab=xAxisLabel,
      axes=FALSE,   # removes automatic axis labels
      lwd=lineWidth,
      lty=lineType.baseCase,
      col=color.baseCase)
    axis(
      side=1,  # 1 = x; bottom
      at=axisTickAgeBuckets,
      labels=axisTickLabels)
    axis(
      side=2,  # 2 = y; left
      at=c(1, frequencyMax/2,frequencyMax),
      labels=c("", "", ""))
    
    # Plot: Comparative cases
    i = 1  # index for color vector
    legend.comparativeCases = c()
    # TODO: pass factor names as params ratehr than D$coef...
    lapply(c(1:length(coefficients.add)), function(c.a) {
      lapply(c(1:length(coefficients.mult)), function(c.m) {
        c.a.val = coefficients.add[c.a]
        c.m.val = coefficients.mult[c.m]
        legend.case = paste("x*", c.m.val, " + ", c.a.val, sep="")
        
        
        D2 = D[D$year==year, ]
        # TODO: Errored out here
        if (has.mult)
          D2 = D2[D2$coefMort2000_multiply==c.m.val, ]
        if (has.add)
          D2 = D2[D2$coefMort2000_add==c.a.val, ]
        
        if (!analysis.isGendered) {
          calibrationTestDataAll = unlist(D2[,fieldBounds])
          lines(
            calibrationTestDataAll,
            type=plotType,
            lwd=lineWidth,
            lty=lineType.comparativeCases,
            col=color.comparativeCases[i])        
          
        } else {
          if (filterFemale | byGender) {
            fieldBounds = grep("ageF", DATA.FIELD_NAMES)
            calibrationTestDataFemale = unlist(D2[,fieldBounds])
            lines(
              calibrationTestDataFemale,
              type=plotType,
              lwd=lineWidth,
              lty=lineType.comparativeCases,
              col=color.female)
          }
          if (filterMale | byGender) {
            fieldBounds = grep("ageM", DATA.FIELD_NAMES)
            calibrationTestDataMale = unlist(D2[,fieldBounds])
            lines(
              calibrationTestDataMale,
              type=plotType,
              lwd=lineWidth,
              lty=lineType.comparativeCases,
              col=color.male)
          }
        }
        
        i <<- i + 1
        legend.comparativeCases <<- c(legend.comparativeCases, legend.case)
      })
    })
    
    # Plot: Add legend
    if (!byGender)
      legend(
        "topright",
        legend=c(legend.baseCase, legend.comparativeCases),
        fill=c(color.baseCase, color.comparativeCases),
        bty="n",  # n = omit box around legend, o(default) = include
        title=legendTitle)
    else
      legend(
        "topright",
        legend=c("♂", "♀"),
        fill=c(color.male, color.female),
        bty="n",  # n = omit box around legend, o(default) = include
        title=legendTitle)
  })
  dev.off()  
}

# Module static: Plot Subclasses ####
# Basic or semi advanced plots w/out all the bells and whistles.
'Plot: Calibration of age and/or sex demographics
Semi-generalized plotting function
@param D(data.frame): Input data'
plot.subclass.PARAMS_add_mult <- function(
  D,
  param.add.factors,
  param.mult.factors,
  output.filename,
  filterMult=TRUE,
  filterAdd=TRUE
) {
  # Variables ####
  # Static
  plotType = "l"
  yAxisLabel = "n Individuals"
  xAxisLabel = "Age group"
  title = "Age Bucket Distribution"
  legendTitle = "Mortality Functions"
  legend.baseCase = "India 2000 A.D."
  lineType.baseCase = "dotted"
  lineType.comparativeCases = "solid"
  lineWidth = 2
  color.baseCase = "red"
  axis.x.ticks = c(1, 5, 9, 13, 17, 21)  # arbitrary
  # Calculated
  output.path = paste(OUTPUT_DIR, "/", output.filename, ".jpeg", sep="")
  numLines = length(param.add.factors) * length(param.mult.factors)
  color.comparativeCases = viridis(numLines)
  fieldBounds = grep("ageAll", DATA.FIELD_NAMES)
  
  # frequencyMax = 13000
  frequencyMax = max(D[, fieldBounds])
  # simulationYearsRan = 499
  simulationYearsRan = max(D$year) + 1
  
  yearsBetweenComparativeCasePlots = 25
  comparativeCasesYearStart = yearsBetweenComparativeCasePlots
  comparativeCasesYearsToPlot = seq(
    comparativeCasesYearStart, 
    simulationYearsRan, 
    yearsBetweenComparativeCasePlots)
  yearsToPlot = c(1, comparativeCasesYearsToPlot)
  yearsToPlot = yearsToPlot - 1  # Corrects for model code 0-indexing
  axisTickLabels = 
    sapply(axis.x.ticks, function(x) CATEGORY.AGE_GROUP.LABELS[x])
  
  # Plot: Globals ####
  jpeg(output.path, width=IMG.WIDTH, height=IMG.HEIGHT)
  par(mfrow=c(5,4))
  
  # Plot: Empirical data ####
  # Filter
  D.base = D[D$year==0, ]
  if (filterMult)
    D.base = D.base[D.base$coefMort2000_multiply==1, ]
  if (filterAdd)
    D.base = D.base[D.base$coefMort2000_add==0, ]
  data = unlist(D.base[,fieldBounds])
  
  # Visualize
  plot.plot <- function(year) {
    year.label = year + 1  # Fix for model code 0-indexing
    plot(
      data,ylim=c(0, frequencyMax),
      xlim=c(0, length(CATEGORY.AGE_GROUP.LABELS)),type=plotType,
      main=paste(title, " (Year ", year.label, ")"),ylab=yAxisLabel,
      xlab=xAxisLabel,axes=FALSE,lwd=lineWidth,lty=lineType.baseCase,
      col=color.baseCase)
    axis(side=1,at=axis.x.ticks,labels=axisTickLabels)
    axis(side=2,at=c(1, frequencyMax/2,frequencyMax),labels=c("", "", ""))  
  }
  
  # Plot: Experimental data ####
  lapply(yearsToPlot, function(year) {
    plot.plot(year)
    
    # TODO: temp: try moving legend here; then try to do only for year 1
    
    # TODO: Better to not use these globals if possible
    i = 1  # index for color vector
    legend.comparativeCases = c()
    lapply(c(1:length(param.add.factors)), function(c.a) {
      lapply(c(1:length(param.mult.factors)), function(c.m) {
        # Variables
        c.a.val = param.add.factors[c.a]
        c.m.val = param.mult.factors[c.m]
        legend.case = paste("x*", c.m.val, " + ", c.a.val, sep="")
        # Filter
        D.i = D[D$year==year, ]
        D.i = D.i[D.i$coefMort2000_multiply==c.m.val, ]
        D.i = D.i[D.i$coefMort2000_add==c.a.val, ]
        data = unlist(D.i[,fieldBounds])
        # Visualize
        # TODO: full detail: restore
        # lines(data)  # minimal detail
        if (year > 1)  # these lines aren't relevant for year 1
          lines(  # moderate detail
            data,type=plotType,lwd=lineWidth,lty=lineType.comparativeCases,
            col=color.comparativeCases[i])
        # Update globals
        i <<- i + 1
        legend.comparativeCases <<- c(legend.comparativeCases, legend.case)
      })
    })
    # Plot: Legend ####
    # TODO: Dynamically determine ncol
    if (year <= 1)
      legend(
        "topright",legend=c(legend.baseCase, legend.comparativeCases),
        fill=c(color.baseCase, color.comparativeCases),bty="n",
        title=legendTitle,ncol=4)
  })
  # Plot: Save
  dev.off()  
}

'Plot: Calibration of age and/or sex demographics
Semi-generalized plotting function
@param D(data.frame): Input data'
plot.subclass.PARAMS_filterable <- function(
  D,
  param.add.factors,
  param.mult.factors,
  output.filename,
  filterMult=TRUE,
  filterAdd=TRUE
) {
  # Variables ####
  # Static
  plotType = "l"
  yAxisLabel = "n Individuals"
  xAxisLabel = "Age group"
  title = "Age Bucket Distribution"
  legendTitle = "Mortality Functions"
  legend.baseCase = "India 2000 A.D."
  lineType.baseCase = "dotted"
  lineType.comparativeCases = "solid"
  lineWidth = 2
  color.baseCase = "red"
  # Calculated
  output.path = paste(OUTPUT_DIR, "/", output.filename, ".jpeg", sep="")
  if (!filterMult)
    param.mult.factors = c(1)
  if (!filterAdd)
    param.add.factors = c(0)
  numLines = length(param.add.factors) * length(param.mult.factors)
  color.comparativeCases = viridis(numLines)
  fieldBounds = grep("ageAll", DATA.FIELD_NAMES)
  
  # TODO: Dtermine dynamically based on seq(1, length(fieldBounds), x),
  # Where 'x' is the lowest evenly divisible number of length(fieldBounds)
  axis.x.ticks = c(1, 5, 9, 13, 17, 21)  # arbitrary / lowest divisible
  # frequencyMax = 13000
  frequencyMax = max(D[, fieldBounds])
  # simulationYearsRan = 499
  simulationYearsRan = max(D$year) + 1
  
  yearsBetweenComparativeCasePlots = 25
  comparativeCasesYearStart = yearsBetweenComparativeCasePlots
  comparativeCasesYearsToPlot = seq(
    comparativeCasesYearStart, 
    simulationYearsRan, 
    yearsBetweenComparativeCasePlots)
  yearsToPlot = c(1, comparativeCasesYearsToPlot)
  yearsToPlot = yearsToPlot - 1  # Corrects for model code 0-indexing
  axisTickLabels = 
    sapply(axis.x.ticks, function(x) CATEGORY.AGE_GROUP.LABELS[x])
  
  # Plot: Globals ####
  jpeg(output.path, width=IMG.WIDTH, height=IMG.HEIGHT)
  # TODO: change to horizontal setup?
  # TODO: Dynamically determine?
  par(mfrow=c(5,4))
  
  # Plot: Empirical data ####
  # Filter
  D.base = D[D$year==0, ]
  if (filterMult)
    D.base = D.base[D.base$coefMort2000_multiply==1, ]
  if (filterAdd)
  D.base = D.base[D.base$coefMort2000_add==0, ]
  data = unlist(D.base[,fieldBounds])
  
  # Visualize
  plot.plot <- function(year) {
    year.label = year + 1  # Fix for model code 0-indexing
    plot(
      data,ylim=c(0, frequencyMax),
      xlim=c(0, length(CATEGORY.AGE_GROUP.LABELS)),type=plotType,
      main=paste(title, " (Year ", year.label, ")"),ylab=yAxisLabel,
      xlab=xAxisLabel,axes=FALSE,lwd=lineWidth,lty=lineType.baseCase,
      col=color.baseCase)
    axis(side=1,at=axis.x.ticks,labels=axisTickLabels)
    axis(side=2,at=c(1, frequencyMax/2,frequencyMax),labels=c("", "", ""))  
  }
  
  # Plot: Experimental data ####
  lapply(yearsToPlot, function(year) {
    plot.plot(year)
    # TODO: Better to not use these globals if possible
    i = 1  # index for color vector
    legend.comparativeCases = c()
    lapply(c(1:length(param.add.factors)), function(c.a) {
      lapply(c(1:length(param.mult.factors)), function(c.m) {
        # Variables
        c.a.val = param.add.factors[c.a]
        c.m.val = param.mult.factors[c.m]
        legend.case = paste("x*", c.m.val, " + ", c.a.val, sep="")
        # Filter
        D.i = D[D$year==year, ]
        if (filterMult)
          D.i = D.i[D.i$coefMort2000_multiply==c.m.val, ]
        if (filterAdd)
          D.i = D.i[D.i$coefMort2000_add==c.a.val, ]
        data = unlist(D.i[,fieldBounds])
        
        # browser()
        
        # Visualize
        # TODO: full detail: restore
        # lines(data)  # minimal detail
        if (year > 1)  # these lines aren't relevant for year 1
          lines(  # moderate detail
            data,type=plotType,lwd=lineWidth,lty=lineType.comparativeCases,
            col=color.comparativeCases[i])
        # Update globals
        i <<- i + 1
        legend.comparativeCases <<- c(legend.comparativeCases, legend.case)
      })
    })
    # Plot: Legend ####
    # TODO: Dynamically determine ncol
    if (year <= 1)
      legend(
        "topright",legend=c(legend.baseCase, legend.comparativeCases),
        fill=c(color.baseCase, color.comparativeCases),bty="n",
        title=legendTitle,ncol=4)
  })
  # Plot: Save
  dev.off()  
}

# Dataset unknown ####
'Plot something
Not sure what this plots yet; image currently broken
@param D(data.frame): Input data'
plot.something <- function(D) {
  jpeg(paste(OUTPUT_DIR, "/", "ageDistCalib.jpeg", sep=""), 
       width=1000, height=1000)
  vCols=viridis(7)
  par(mfrow=c(5,4))#par(mfrow=c(1,1))
  
  lapply(seq(0,499,25),function(x){
    DD<-D[D$year==0 & D$coefMort2000==1,]
    plot(
      unlist(DD[,7:27]),
      ylim=c(0,12000),
      type="l",
      lwd=2,
      main=paste("Year =",x," deaths= ",DD$deaths),
      ylab="Freq",
      xlab="AgeDist",
      col="red")
    
    lapply(c(1:7),function(c){
      DD<-D[D$year==x & D$coefMort2000==c,]
      lines(
        unlist(DD[,7:27]),
        type="l",
        lwd=2,
        main=paste("Year =",x," deaths= ",DD$deaths),
        ylab="prob",
        xlab="AgeDist",
        col=vCols[c])
    })
    legend(
      "topright",
      legend=c(1:7),
      fill=vCols[c(1:7)],
      bty="n",
      title="coefMortality")
  })
  dev.off()
}

'Plot population growth
@param D(data.frame): Input data'
plot.populationGrowth <- function(D) {
  lapply(c(1,2,2.5,3,4),function(c){
    jpeg(paste(OUTPUT_DIR, "/ageDistCalib-",c,".jpeg",sep = ""),
         width=1000,height=1000)
    par(mfrow=c(5,4))#par(mfrow=c(1,1))
    
    lapply(seq(0,499,25),function(x){
      DD<-D[D$year==0 & D$coefMort2000==1,]
      plot(
        unlist(DD[,7:27]),
        ylim=c(0,12000),
        type="l",
        lwd=2,
        main="",
        ylab="Freq",
        xlab="Age groups",
        col="black",
        xaxt="n")
      axis(
        side=1,
        at=c(1:21),
        labels=CATEGORY.AGE_GROUP.LABELS)
      DD<-D[D$year==x & D$coefMort2000==c,]
      lines(
        unlist(DD[,7:27]),
        type="l",
        lwd=2,
        main=paste("Year =",x," deaths= ",DD$deaths),
        ylab="prob",
        xlab="AgeDist",
        col=img.color.palette)
      mtext(
        side=3,
        text=paste("Year =",x," deaths= ", round(DD$deaths,1)), padj=-2)
      legend(
        "topright",
        legend=c,
        fill=img.color.palette,
        bty="n",
        title="coefMortality")
    })
    dev.off()
  })  
}

# Dataset 6x6 ####
# Params ({paramNum: nFactors}): {1: 6, 2: 6}
# 6x6
plot._BY.age_FILTER.6add_6mult <- function(D) {
  # 6x6 plot is currently the default vals
  plot.base(D)
}
plot._BY.age_FILTER.6add_6mult.followup01 <- function(D) {
  # Static ####
  fileName = "ageDistCalib-6mult-followup01"
  filePath = paste(OUTPUT_DIR, "/", fileName, ".jpeg", sep="")
  
  # Transform ####
  # Thishas been pre-done before call:
  # D = data.transform.calibration.demographics.add_mult(D)
  
  # Filter #####
  D6 = D[D$year==25, ]
  D7 = D6[D6$coefMort2000_add==0, ]
  unq = unique(D7$coefMort2000_multiply)
  base = unq[1]
  cases = unq[2:length(unq)]
  baseCaseDataUnfiltered = D7[D7$coefMort2000_multiply==base, ]
  fields = grep("ageAll", names(baseCaseDataUnfiltered))
  baseCaseData = unlist(baseCaseDataUnfiltered[,fields])
  
  # Plot ####
  jpeg(filePath, width=img.width, height=img.height)
  plot(baseCaseData)
  
  lapply(cases, function(coef) {
    # Filter ####
    unfiltered = D7[D7$coefMort2000_multiply==coef, ]
    fields = grep("ageAll", names(unfiltered))
    caseData = unlist(unfiltered[,fields])
    
    # Plot ####
    lines(caseData)
  })
  
  dev.off()
}
# 1x1
plot._BY.age_FILTER.1add0pt075.female <- function(D) {
  # closest to best val as tested from another dataset
  coefficients.add.best = c(0.075)
  plot.base(
    D=D,
    imgFileName="ageDistCalib_1add0pt075_f",
    title.static="Age Distribution, Female, (Mortality x: x*1 + 0.075), ",
    factors.fieldNames=c("coefMort2000_add"),
    coefficients.add.=coefficients.add.best,
    filterFemale=TRUE,
  )
}
plot._BY.age_FILTER.1add0pt075.male <- function(D) {
  # closest to best val as tested from another dataset
  coefficients.add.best = c(0.075)
  plot.base(
    D=D,
    imgFileName="ageDistCalib_1add0pt075_m",
    title.static="Age Distribution, Male, (Mortality x: x*1 + 0.075), ",
    factors.fieldNames=c("coefMort2000_add"),
    coefficients.add.=coefficients.add.best,
    filterMale=TRUE,
  )
}
# 1x6
plot._BY.age_FILTER.6mult <- function(D) {
  # plot.base(
  #   D=D,
  #   imgFileName="ageDistCalib-6mult",
  #   factors.fieldNames=c("coefMort2000_multiply"),
  #   coefficients.mult.=vMultCoefficients_6x6.set.,
  #   byGender=FALSE,
  # )
  
  # Setup: plot vals
  jpeg(paste(OUTPUT_DIR, "/", imgFileName, ".jpeg", sep=""), 
       width=img.width, height=img.height)
  par(mfrow=c(5,4))  # par(mfrow=c(1,1))
  
  
  
}
# Empirical
'Data is from year 0 of this experimental dataset, which is tuned to y2000'
plot.empirical.y2000.male_vs_female <- function(D) {
  maleFields = grep("ageM", DATA.FIELD_NAMES)
  femaleFields = grep("ageF", DATA.FIELD_NAMES)
  D2 = D[D$year==0, ]
  dataMaleN = D2[, maleFields]
  dataFemaleN = D2[, femaleFields]
  dataMaleP = dataMaleN / (sum(dataMaleN))
  dataFemaleP = dataFemaleN / (sum(dataFemaleN))
  
  jpeg(paste(OUTPUT_DIR, "/", imgFileName, ".jpeg", sep=""), 
       width=img.width, height=img.height)
  plot(
    unlist(dataMaleP),
    type="l",
    col="blue"
  )
  lines(
    unlist(dataFemaleP),
    col="red"
  )
  dev.off()
}

# Dataset 1x1 ####
# Params ({paramNum: nFactors}): {1: 1}
plot._BY.age.sex_FILTER.1add0pt07 <- function(D) {
  coefficients.add.best = c(0.07)  # based on testing
  plot.base(
    D=D,
    imgFileName="ageDistCalib_1add0pt07_bySex",
    title.static="Age Bucket Distribution, (Mortality x: x*1 + 0.07), ",
    factors.fieldNames=c("coefMort2000_add"),
    coefficients.add.=coefficients.add.best,
    byGender=TRUE,
  )
}
plot._BY.age_FILTER.1add0pt07.female <- function(D) {
  coefficients.add.best = c(0.07)  # based on testing
  plot.base(
    D=D,
    imgFileName="ageDistCalib_1add0pt07_f",
    title.static="Age Bucket Distribution, Female, (Mortality x: x*1 + 0.07), ",
    factors.fieldNames=c("coefMort2000_add"),
    coefficients.add.=coefficients.add.best,
    filterFemale=TRUE,
  )
}
plot._BY.age_FILTER.1add0pt07.male <- function(D) {
  coefficients.add.best = c(0.07)  # based on testing
  plot.base(
    D=D,
    imgFileName="ageDistCalib_1add0pt07_m",
    title.static="Age Bucket Distribution, Male, (Mortality x: x*1 + 0.07), ",
    factors.fieldNames=c("coefMort2000_add"),
    coefficients.add.=coefficients.add.best,
    filterMale=TRUE,
  )
}


# Dataset 4x6 ####
# Params ({paramNum: nFactors}): {1: 4, 2: 6}
plot.BYage.DATASET4x6.FILTER4add_6mult <- function(D) {
  # Code utilizing abstraction ####
  # Variables
  vParams.4x6.add = c(0, .065, .07, .075)
  vParams.4x6.mult = c(1, 1.025, 1.05, 1.075, 1.1, 1.125)
  d4x6.output.filename = 
    "mdrtb-calib-pop-y2000 DATASET-4x6 PARAMS-add-mult BY-age"
  # Plot
  plot.subclass.PARAMS_add_mult(
    D=D,
    param.add.factors=vParams.4x6.add,
    param.mult.factors=vParams.4x6.mult,
    output.filename=d4x6.output.filename)
}

# Dataset add0-.05by.01_mult1-2by.1 ####
plot.FIT_India2000.BY_age.DATASET_add_0toPt5byPt01_mult_1to2byPt1.FILTER_add_mult <- function(D) {
  # This code is now abstracted. Variables and functions originally used 
  # can now be be found in`module.analyze.history.R`.
  output.filename = 
    paste(output.img.filename.base, "FILTER_add_mult", "BY_age", sep=".")
  # plot.subclass.PARAMS_add_mult(
  #   D=D,
  #   param.add.factors=vParams.add,
  #   param.mult.factors=vParams.mult,
  #   output.filename=output.filename)
  plot.subclass.PARAMS_filterable(
    D=D,
    param.add.factors=vParams.add,
    param.mult.factors=vParams.mult,
    output.filename=output.filename,
    filterAdd=TRUE,
    filterMult=TRUE)
}

plot.FIT_India2000.BY_age.DATASET_add_0toPt5byPt01_mult_1to2byPt1.FILTER_add <- function(D) {
  # This code is now abstracted. Variables and functions originally used
  # can now be be found in`module.analyze.history.R`.
  output.filename =
    paste(output.img.filename.base, "FILTER_add", "BY_age", sep=".")
  plot.subclass.PARAMS_filterable(
    D=D,
    param.add.factors=vParams.add,
    param.mult.factors=vParams.mult,
    output.filename=output.filename,
    filterAdd=TRUE,
    filterMult=FALSE)
}

plot.FIT_India2000.BY_age.DATASET_add_0toPt5byPt01_mult_1to2byPt1.FILTER_mult <- function(D) {
  # This code is now abstracted. Variables and functions originally used 
  # can now be be found in`module.analyze.history.R`.
  output.filename = 
    paste(output.img.filename.base, "FILTER_mult", "BY_age", sep=".")
  plot.subclass.PARAMS_filterable(
    D=D,
    param.add.factors=vParams.add,
    param.mult.factors=vParams.mult,
    output.filename=output.filename,
    filterMult=TRUE,
    filterAdd=FALSE)
}

# Dataset add0_mult1and2to32by2times ####
plot.FIT_India2000.BY_age.DATASET_add0_mult1and2to32by2times.FILTER_mult <- function(D) {
  # Disclaimer: This code is now abstracted. Variables and functions
  # originally used can now be be found in`module.analyze.history.R`.
  # This function produces absurdly high values for x16 and x32 runs.
  output.filename =
    paste(output.img.filename.base, "FILTER_mult", "BY_age", sep=".")
  plot.subclass.PARAMS_filterable(
    D=D,
    param.add.factors=vParams.add,
    param.mult.factors=vParams.mult,
    output.filename=output.filename,
    filterMult=TRUE,
    filterAdd=FALSE)
}
plot.FIT_India2000.BY_age.DATASET_add0_mult1and2to8by2times.FILTER_mult <- function(Dm) {
  # Disclaimer: This code is now abstracted. Variables and functions
  # originally used can now be be found in`module.analyze.history.R`.
  output.filename =
    paste(output.img.filename.base, "FILTER_mult2to8", "BY_age", sep=".")
  vParams.mult.modified = c(1.0, 2.0, 4.0, 8.0)
  Dm = Dm[Dm$coefMort2000_multiply %in% vParams.mult.modified, ]
  plot.subclass.PARAMS_filterable(
    D=Dm,
    param.add.factors=vParams.add,
    param.mult.factors=vParams.mult.modified,
    output.filename=output.filename,
    filterMult=TRUE,
    filterAdd=FALSE)
}

# Dataset add0_mult1and2to32by2times ####
plot.FIT_India2000.BY_age.DATASET_toBeDetermined.FILTER_add_mult <- function(D) {
  # Disclaimer: This code is now abstracted. Variables and functions
  # originally used can now be be found in`module.analyze.history.R`.
  output.filename = 
    paste(output.img.filename.base, "FILTER_add_mult", "BY_age", sep=".")
  plot.subclass.PARAMS_filterable(
    D=D,
    param.add.factors=vParams.add,
    param.mult.factors=vParams.mult,
    output.filename=output.filename,
    filterAdd=TRUE,
    filterMult=TRUE)
}
