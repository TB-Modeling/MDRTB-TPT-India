# the purpose of this code is run the mcmc.mh using Todd's package
### Unknown simulation parameters
# coefTrans= coef of TB transmission upn each contact [0-inf] 
# coefComTrans= coef of TB transmission in community (compared to households) [0-1] 
# probMaxSeekCare= maximum prob of seeking care after 9 months [0-1] 
# probMaxMortalityTB= maximum prob of TB deaths in 9 months [0-1]
 
### Calibration targets to fit to:
# Incidence DSTB 
# Duration DSTB 
# mortality DSTB
# Prop of DSTB mortality to incidence (case fatality)
# DS TRANSMISSIONS

# Our C++ related functions
library("sys")
# install.packages("logitnorm")
library("logitnorm") #for logit function

options(stringsAsFactors = FALSE)#To avoid having to deal with factors and levels
##############################################################################################
'MCMC Helper Functions'
read.config = function ( filename ) { #Read the config file data into a variable and return it
  read.table ( filename, sep="=", strip.white=TRUE)
}
write.config = function ( config.data, filename ) {  #Write the data in config.data into a new config filename
  write.table( config.data,filename,sep="=",quote=FALSE,row.names=FALSE,col.names=FALSE)
}
get.value = function ( config.data, key ) {  #Read the value of variable 'key' from the config.data
  val = config.data[config.data == key,2]
  if (substr(val, nchar(val), nchar(val)) == ";") {    #Some values have trailing semi-colons, strip them
    val = substr(val, 1, nchar(val)-1) }
  as.numeric(val)
}
set.value = function( config.data, key, value ) {
  #Set the config.data variable 'key' to 'value'  #Note; it must be assigned to itself in the code:
  #for example:   #config.data = set.value ( config.data, 'coefHHTrans', 0.002 )
  config.data[config.data == key,2] = sprintf("%f;",value)
  config.data
}
library("data.table")
readData<-function(name=""){
  D<-as.data.frame(fread(name,header = F,blank.lines.skip = T,fill = T))
  D <- data.frame(apply(D, 2, function(x) as.numeric(as.character(x))))
  names(D)<-   c("rep","year", "pop", "nPrevDS" ,"nIncDS" ,"nIncDS_fastProg" ,"nIncDS_slowProg" ,"nIncDS_relapse" ,"nIncDS_failure",
                 "nPrevDR" ,"nIncDR" ,"nIncDR_fastProg", "nIncDR_slowProg" ,"nIncDR_relapse" ,"nIncDR_resis", "nIncDR_failure",
                 "nTransDS", "nTransDR", "nTransDS_HH" ,"nTransDR_HH", 
                 "nIncHH","nIncCom","nIncDsHH","nIncDsCom",
                 "nEnterELDS" ,"nEnterELDR", "nMortalityDS","nMortalityDR" ,
                 "nVisits","nDiagDS","nDiagDR","nDST","nLTFU",
                 "nTrtDS_DS" ,"nTrtDS_DR", "nTrtDR_DR" ,
                 "nFailedDSTrt_DS","nFailedDSTrt_DR","nFailedDRTrt_DR",
                 "nNewTB","nRetreatedTB","nNewDR","nRetreatedDR",
                 "nSUSDS","nELDS","nLLDS","nRDS","nADS","nTRDS_DS","nTRDR_DS",
                 #
                 "_coefTrans","_coefHHTrans","_coefComTrans","_probMaxMortalityTB",
                 "_probMaxSeekCare","_probResistance","_coefMaxInfect_DR_RelTo_DS",
                 # 
                 "durDS","nDurDS","durDR","nDurDR",
                 #
                 "prpHhPrevDS_diagDS","nHhPrevDS_diagDS",
                 "prpHhPrevDR_diagDS","nHhPrevDR_diagDS",
                 "prpHhPrevTB_diagDS","nHhPrevTB_diagDS",
                 "prpHhPrevDS_diagDR","nHhPrevDS_diagDR",
                 "prpHhPrevDR_diagDR","nHhPrevDR_diagDR",
                 "prpHhPrevTB_diagDR","nHhPrevTB_diagDR",
                 "prpHhPrevTB_diagTB","nHhPrevTB_diagTB",
                 #
                 "rngSeed"
  )
  # turn freq into rates per 100,000 people so changes in pop size wont matter
  freqVar<-c("nPrevDS" ,"nIncDS" ,"nIncDS_fastProg" ,"nIncDS_slowProg" ,"nIncDS_relapse" ,"nIncDS_failure",
             "nPrevDR" ,"nIncDR" ,"nIncDR_fastProg", "nIncDR_slowProg" ,"nIncDR_relapse" ,"nIncDR_resis", "nIncDR_failure",
             "nTransDS", "nTransDR", "nTransDS_HH" ,"nTransDR_HH", 
             "nIncHH","nIncCom","nIncDsHH","nIncDsCom",
             "nEnterELDS" ,"nEnterELDR", "nMortalityDS","nMortalityDR" ,
             "nVisits","nDiagDS","nDiagDR","nDST","nLTFU",
             "nTrtDS_DS" ,"nTrtDS_DR", "nTrtDR_DR" ,
             "nFailedDSTrt_DS","nFailedDSTrt_DR","nFailedDRTrt_DR")
  D[, sub(".","r",freqVar)]<-(D[,freqVar]/D$pop) *100000
  
  D$pHHTrans<-(D$nTransDS_HH+D$nTransDR_HH)/(D$nTransDS+D$nTransDR)
  D$pHHInc<-D$nIncHH/(D$nIncDS+D$nIncDR)
  D$pNewDR<-D$nNewDR/D$nNewTB
  D$pRetreatedDR<-D$nRetreatedDR/D$nRetreatedTB
  D$pFatalityDS<-(D$nMortalityDS)/D$nIncDS  
  D[D==-Inf]<- -1
  D[D==-1000]<-0
  return (D)
}
###############################################################################################
'MCMC Procedure Functions'
##----------------------------##
##-- THE PRIOR DISTRIBUTION --##
##----------------------------##
# Define the log prior for all parameters
log.prior = function(parameters){
  print("called log.prior function");
  lp<-
    dgamma(as.numeric(parameters['coefTrans1900']), shape = 3,scale = 0.033333, log=T) +
    dunif(as.numeric(parameters['coefComTrans']), min = 0, max = 1, log=T) +
    dbeta(as.numeric(parameters['probMaxSeekCare']), shape1 = 1.87,shape2 = 2.06, log=T) +
    dbeta(as.numeric(parameters['probMaxMortalityTB']), shape1 = 1.41,shape2 = 8.65, log=T) 
  return(lp)
}

##------------------------------##
##--  THE SIMULATION FUNCTION --##
##------------------------------##
run.simulation<-function(parameters=parameters ,chainID=0, iteration=""){
  sprintf("called run.simulation function, ChainID %s",chainID);
  # read config file
  config.data = read.config ( paste(base.config.file,".cfg",sep="") )
  # update the config file for this parameter specification
  paramSize=length(parameters)
  for ( i in c(1:paramSize) ) { 
    config.data = set.value ( config.data, names(parameters[i]), as.numeric(parameters[i]) )
  }
  # over-write the new config file to disk
  target.config.file<-paste(base.config.file,"_mcmc_",chainID,".cfg",sep="")
  write.config (config.data, target.config.file)
  # run C++ code: read the new config file, run the simulation.
  out = exec_wait (base.model.file, args=c(chainID, target.config.file,final.year)  )
  # check successful execution?
  if ( out == 100 ) {
    #Run was aborted by C++ for having too large an incidence during the burnin phase
    #Create an empty output object
    output<-data.frame(matrix(0,ncol=63, nrow=10))
  } else if (out == 0) {
    #Normal program execution
    sim<-readData( paste("out_mcmc_",chainID,sep=""))
  } else {  
    #Program exited abnormally
    print (sprintf("Program exited with error code : ", return_value))
    stop("Simulation did not execute successfully")
  }
  sim<-as.list(sim)
  return (sim)
}

 # sim<-readData( paste("out_mcmc_1",sep=""))  ;sim<-as.list(sim)

##-----------------------------##
##-- THE LOKELIHOOD FUNCTION --##
##-----------------------------##
# This function takes the output generated by run.simulation and should parse out the relevant simulation outptus first
' MCMC: log likelihood for fitting, using logit-normal dist for proportions and log-normal dist for other outputs'
log.lik <- function(sim ) {
  print("called log.lik function");
  # -incidence rate of dstb in 1970
  inc.dstb<- mean(sim$rIncDS[sim$year>1960] )
   
  # -duration of DSTB from  in MONTHS: this is a tally vector. we are reporting the mean obs and number of observations in each year, and will take the overall mean here
  dur.dstb<-sum(sim$durDS[sim$year>1960 ]*sim$nDurDS[sim$year>1960] )/sum(sim$nDurDS[sim$year>1960])
  
  # -mortality 
  mort.dstb<- mean(sim$rMortalityDS[sim$year>1960] )

  # -pHH transmission
  pHhPrevTB_diagTB= sum(sim$prpHhPrevTB_diagTB[sim$year>1960 ]*sim$nHhPrevTB_diagTB[sim$year>1960] )/sum(sim$nHhPrevTB_diagTB[sim$year>1960])
  
  # final list of liklihoods:
  res<-sum(
    #original likelihoods
    # log(dnorm(  (log(inc.dstb) - log(292))/ ((log(341)-log(199))/4)   )),
    # log(dnorm(  (log(dur.dstb) - log(1.2*12))/ ((log(1.6 * 12)-log(.9 *12))/4)   )),
    # log(dnorm(  (log(mort.dstb) - log(33))/ ((log(30)-log(36))/4)   )),
    # log(dnorm(  (logit(prpHhPrevTB_diagTB) - logit(0.03))/ ((logit(.02)-logit(.04))/4)   ))
    # revised ones:
    log(dnorm(  (log(inc.dstb) - log(292))/ ((log(320)-log(262))/4)   )),
    log(dnorm(  (log(dur.dstb) - log(1.2*12))/ ((log(1.31 * 12)-log(1.09 *12))/4)   )),
    log(dnorm(  (log(mort.dstb) - log(33))/ ((log(30)-log(36))/4)   )),
    log(dnorm(  (logit(pHhPrevTB_diagTB) - logit(0.03))/ ((logit(.027)-logit(.033))/4)   )) 
  )
  
  if ((res==-Inf) |(res==Inf)| is.nan(res)) res=-1000 #penalize bad simulations 
  return(res)
}
###############################################################################################
###############################################################################################
###############################################################################################
'RUNNING MCMC CHAIN'
## Requirements:
library(ggplot2)
library(distributions) #defines distributions as objects - handy for processing results (and potentially defining priors)
library(mcmc.sim) #runs the MCMC
HAVE.NOT.INSTALLED.PACKAGES = F #This is what you need to do to install my packages
if (HAVE.NOT.INSTALLED.PACKAGES)
{ #For first time installation, you also need these extra packages
  install.packages("mvtnorm")
  install.packages("tmvtnorm")
  install.packages("matrixcalc")
  install.packages("r/src/todd.mcmc/distributions_0.1.0.tar.gz", repos=NULL, type="source")
  install.packages("lhs")
  install.packages("r.src/todd.mcmc.package/mcmc.sim_0.2.0.tar.gz", repos=NULL, type="source")
  install.packages("r.src/todd.mcmc.package/mcmc.sim_0.2.2.tar.gz", repos=NULL, type="source")
}

##------------------##
##-- RUN THE MCMC --##
##------------------##
library(mcmc.sim) #runs the MCMC
# a potential starting point
var = cbind(coefTrans1900=.12,   coefComTrans=.06,
            probMaxMortalityTB=.05, probMaxSeekCare=.9)
# set up the original covariance matrix based ont he starting points (assuming normal distribution with 20 sd around mean)
covMat=diag(c((var/40)^2)); covMat
# covMat=covMat/100

# define control object
ctrl = create.adaptive.blockwise.metropolis.control( var.names=c("coefTrans1900","coefComTrans","probMaxSeekCare","probMaxMortalityTB"),
                                                     simulation.function = run.simulation,
                                                     pass.chain.to.simulation.function = T, 
                                                     pass.iteration.to.simulation.function = F,
                                                     log.prior.distribution = log.prior,
                                                     log.likelihood = log.lik,
                                                     burn = 0,
                                                     thin = 1,
                                                     initial.covariance.mat = covMat
)

##--------------------##
##-- INITIAL SET UP --##
##--------------------##
set.seed = 898798798 # random seef for mcmc
base.config.file = "c.src/seedConfig" ; # where the config file is located
base.model.file="c.src/mdrtb_mcmc2.out"; # where executable is located

# parallel chains? Use multiple starting points:
starting.values<-rbind( cbind(coefTrans1900=.12, coefComTrans=.06, probMaxMortalityTB=.05, probMaxSeekCare=.9),
                        cbind(coefTrans1900=.1, coefComTrans=.08, probMaxMortalityTB=.1, probMaxSeekCare=.8),
                        cbind(coefTrans1900=.09, coefComTrans=.09, probMaxMortalityTB=.15, probMaxSeekCare=.7),
                        cbind(coefTrans1900=.11, coefComTrans=.07, probMaxMortalityTB=.2, probMaxSeekCare=.6),
                        cbind(coefTrans1900=.13 , coefComTrans=.05, probMaxMortalityTB=.25, probMaxSeekCare=.5),
                        cbind(coefTrans1900=.14 , coefComTrans=.04, probMaxMortalityTB=.3, probMaxSeekCare=.4),
                        cbind(coefTrans1900=.15 , coefComTrans=.03, probMaxMortalityTB=.35, probMaxSeekCare=.3),
                        cbind(coefTrans1900=.12, coefComTrans=.06, probMaxMortalityTB=.4, probMaxSeekCare=.2))

# Default:
N.ITER = 10 #lenght of the chain
cache.freq = 1
final.year=1970
#Command line arguments if we want to change the N.ITER, cache.frequency  
args = commandArgs(trailingOnly=TRUE)
if (length(args) == 3) {  #Arguments are:  # [iterations] [cache.frequency] 
  N.ITER = as.numeric(args[1])
  cache.freq = as.numeric(args[2])
  final.year = as.numeric(args[3])
  print(paste("N.ITER ",N.ITER," cache.freq ",cache.freq," final.year ",final.year))
}


mcmc = run.mcmc(control = ctrl,
                n.iter=N.ITER,
                starting.values=starting.values,
                update.frequency=1,
                update.detail = "High",
                # cache.dir = paste("../mcmc.cache",chainID,sep=""),
                cache.dir = "../mcmc.cache.parallel",
                allow.overwrite.cache = T,
                cache.frequency = cache.freq,
                remove.cache.when.done = F)


