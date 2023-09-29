# the purpose of this code is run the mcmc.mh using Todd's package
### Unknown simulation parameters
# coefTrans= coef of TB transmission upn each contact [0-inf] 
# coefComTrans= coef of TB transmission in community (compared to households) [0-1] 
# probMaxSeekCare= maximum prob of seeking care after 9 months [0-1] 
# probMaxMortalityTB= maximum prob of TB deaths in 9 months [0-1]
# coefResistance= coef of TB treatment failure leading to DRTB starting in 1970 [0-1]
# relativeInf_DR_to_DS= relative infectiousness of DRTB compared to DSTB [0-1]

### Calibration targets to fit to:
# Incidence DSTB 
# Duration DSTB 
# mortality DSTB
# Prop of DSTB mortality to incidence (case fatality)
# RR new & retreated DRTB

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
  D<- data.frame(apply(D, 2, function(x) as.numeric(as.character(x))))
  
  names(D) <- c("rep","year", "pop", "nPrevDS" ,"nIncDS" ,"nIncDS_fastProg" ,"nIncDS_slowProg" ,"nIncDS_relapse" ,"nIncDS_failure",
                "nPrevDR" ,"nIncDR" ,"nIncDR_fastProg", "nIncDR_slowProg" ,"nIncDR_relapse" ,"nIncDR_resis", "nIncDR_failure",
                "nTransDS", "nTransDR", "nTransDS_HH" ,"nTransDR_HH",
                "nIncHH","nIncCom","nIncDsHH","nIncDrHH",
                "nEnterELDS" ,"nEnterELDR", "nMortalityDS","nMortalityDR" ,
                "nVisits","nDiagDS","nDiagDR","nDST","nLTFU",
                "nTrtDS_DS" ,"nTrtDS_DR", "nTrtDR_DR" ,
                "nFailedDSTrt_DS","nFailedDSTrt_DR","nFailedDRTrt_DR",
                "nNewTB","nRetreatedTB","nNewDR","nRetreatedDR",
                "nSUSDS","nELDS","nLLDS","nRDS","nADS","nTRDS_DS","nTRDR_DS",
                
                "_coefTrans","_coefHHTrans","_coefComTrans","_probMaxMortalityTB",
                "_probMaxSeekCare","_probResistance","_coefMaxInfect_DR_RelTo_DS",
                #
                "durDS","nDurDS","durDR","nDurDR",
                #
                "nHhTraced_diagADS", "nHhMembersTraced_diagADS","nHhPrevADS_diagADS", "nHhPrevADR_diagADS", 
                "nHhTraced_diagADR", "nHhMembersTraced_diagADR","nHhPrevADS_diagADR", "nHhPrevADR_diagADR", 
                
                "nHhPrevLADS_diagADS", 
                "nHhPrevADS_hhOriginated_diagADS","nHhPrevLADS_hhOriginated_diagADS",
                #
                "rngSeed")
  
  # turn freq into rates so changes in pop size wont matter
  freqVar<-c("nPrevDS" ,"nIncDS" ,"nIncDS_fastProg" ,"nIncDS_slowProg" ,"nIncDS_relapse" ,"nIncDS_failure",
             "nPrevDR" ,"nIncDR" ,"nIncDR_fastProg", "nIncDR_slowProg" ,"nIncDR_relapse" ,"nIncDR_resis", "nIncDR_failure",
             "nTransDS", "nTransDR", "nTransDS_HH" ,"nTransDR_HH", 
             "nIncHH","nIncCom","nIncDsHH","nIncDrHH",
             "nEnterELDS" ,"nEnterELDR", "nMortalityDS","nMortalityDR",
             "nVisits","nDiagDS","nDiagDR",
             "nTrtDS_DS" ,"nTrtDS_DR", "nTrtDR_DR" ,
             "nFailedDSTrt_DS","nFailedDSTrt_DR","nFailedDRTrt_DR",
             "nSUSDS","nELDS","nLLDS","nRDS","nADS","nTRDS_DS","nTRDR_DS")
  
  D[, sub(".","r",freqVar)]<-(D[,freqVar]/D$pop) *100000
  
  #rate of DST among all visits
  D$rDST<-D$nDST/D$nVisits
  #rate of LTFU after diagnosis
  D$rLTFU<-D$nLTFU/(D$nDiagDS+D$nDiagDR)
  
  #prp of all transmission or incidence due to household contacts
  D$pHHTrans<-(D$nTransDS_HH+D$nTransDR_HH)/(D$nTransDS+D$nTransDR)
  D$pHHInc<-D$nIncHH/(D$nIncDS+D$nIncDR)
  
  # prevalence of TB among households at diagnosis:
  D$pHhPrevADS_diagADS=D$nHhPrevADS_diagADS/D$nHhMembersTraced_diagADS;
  D$pHhPrevLADS_diagADS=D$nHhPrevLADS_diagADS/D$nHhMembersTraced_diagADS;
  D$pHhPrevADS_hhOriginated_diagADS=D$nHhPrevADS_hhOriginated_diagADS/D$nHhPrevADS_diagADS;
  D$pHhPrevLADS_hhOriginated_diagADS=D$nHhPrevLADS_hhOriginated_diagADS/D$nHhPrevLADS_diagADS;
  #
  D$pHhPrevATB_diagADS=(D$nHhPrevADS_diagADS+D$nHhPrevADR_diagADS)/D$nHhMembersTraced_diagADS;
  D$pHhPrevATB_diagADR=(D$nHhPrevADS_diagADR+D$nHhPrevADR_diagADR)/D$nHhMembersTraced_diagADR;
  D$pHhPrevATB_diagATB=(D$nHhPrevADS_diagADS+
                          D$nHhPrevADR_diagADS+
                          D$nHhPrevADS_diagADR+
                          D$nHhPrevADR_diagADR)/(D$nHhMembersTraced_diagADS+D$nHhMembersTraced_diagADS);
  
  #proportion of incidence due to different paths:
  D$propIncDS_fastProg=D$nIncDS_fastProg/D$nIncDS
  D$propIncDR_fastProg=D$nIncDR_fastProg/D$nIncDR
  D$propIncDS_resis=D$nIncDR_resis/D$nIncDR
  D$propIncDS_failure=D$nIncDR_failure/D$nIncDR
  
  #proportion of treatments failed:
  D$prpDStrt_DR_failed<-D$nFailedDSTrt_DR/D$nTrtDS_DR
  D$prpDStrt_DS_failed<-D$nFailedDSTrt_DS/D$nTrtDS_DS
  D$prpDRtrt_DR_failed<-D$nFailedDRTrt_DR/D$nTrtDR_DR
  
  #proportion of DR among new and retreated patients 
  D$pNewDR<-D$nNewDR/D$nNewTB
  D$pRetreatedDR<-D$nRetreatedDR/D$nRetreatedTB
  
  D$pFatalityDS<-(D$nMortalityDS)/D$nIncDS  
  D[D==-Inf]<- -1
  D[is.na(D)]<- 0
  D[D== -1000]<-0
  # D<-D[D$year>1970,]
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
    dgamma(as.numeric(parameters['coefHHTrans1900']), shape = 3,scale = 0.0013333, log=T) +
    dgamma(as.numeric(parameters['coefHHTrans1900']), shape = 3,scale = 0.0013333, log=T) +
    dunif(as.numeric(parameters['coefComTrans']), min = 0, max = 1, log=T) +
    dbeta(as.numeric(parameters['probMaxSeekCare']), shape1 = 1.87,shape2 = 2.06, log=T) +
    
    dbeta(as.numeric(parameters['probMaxMortalityTB']), shape1 = 2.04,shape2 = 24.87, log=T) + # adjusted to quantile1=list(p=.025, x=0.01) quantile2=list(p=.975, x=0.20) 
    dbeta(as.numeric(parameters['probResistance']), shape1 = 1.75 ,shape2 = 168.23, log=T) +
    dunif(as.numeric(parameters['coefMaxInfect_DR_RelTo_DS']), min = 0, max = 1, log=T)
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
  out = exec_wait (base.model.file, args=c(chainID, target.config.file)  )
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
# sim<-readData( paste("c.src/out_mcmc_10",sep=""))  ;sim<-as.list(sim)

##-----------------------------##
##-- THE LOKELIHOOD FUNCTION --##
##-----------------------------##
# This function takes the output generated by run.simulation and should parse out the relevant simulation outptus first
' MCMC: log likelihood for fitting, using logit-normal dist for proportions and log-normal dist for other outputs'
log.lik <- function(sim ) {
  print("called log.lik function");
  # -incidence rate of dstb in 1970
  inc.dstb.1970<- sim$rIncDS[sim$year==1970] 
  # -incidence rate of ds  in 2000
  inc.dstb.2000<- sim$rIncDS[sim$year==2000] 
  # -incidence rate of ds  in 2018
  inc.dstb.2018<- sim$rIncDS[sim$year==2018] 
  
  # -duration of DSTB from 2000 to 2020 in MONTHS: this is a tally vector. we are reporting the mean obs and number of observations in each year, and will take the overall mean here
  dur.dstb.2000.forward<-sum(sim$durDS[sim$year>1999 ]*sim$nDurDS[sim$year>1999] )/sum(sim$nDurDS[sim$year>1999])
  
  # -prp of DS mortality to incidence from 2000 to 2020: this is a tally vector. we are reporting the mean obs and number of observations in each year, and will take the overall mean here
  var=sim$nMortalityDS[sim$year>1999]/sim$nIncDS[sim$year>1999]; 
  var[var==Inf]<-NA; var<- mean(var,na.rm = T);   if (is.na(var)|(var>1)) var = 1;
  prpFatality.dstb.2000.forward<-var
  
  # -mortality in 2018
  mort.dstb.2018<- sim$rMortalityDS[sim$year==2018] 
  
  # -average RR fraction of new and retreated cases each year between 2013 and 2018
  rrNew.2013.18=  mean(sim$pNewDR[sim$year>2013 & sim$year<2019])
  rrRetreated.2013.18=  mean(sim$pRetreatedDR[sim$year>2013 & sim$year<2019])
  
  # final list of liklihoods:
  res<-sum(
    log(dnorm(  (log(inc.dstb.1970) - log(292))/ ((log(341)-log(199))/4)   )), #same incidence target from 1970 and 2000 (we r looking for equilibrium)
    log(dnorm(  (log(inc.dstb.2000) - log(292))/ ((log(341)-log(199))/4)   )),
    log(dnorm(  (log(inc.dstb.2018) - log(199))/ ((log(136)-log(233))/4)   )), #reduction in incidence at 2% per year
    log(dnorm(  (log(dur.dstb.2000.forward) - log(1.2*12))/ ((log(1.6 * 12)-log(.9 *12))/4)   )),
    log(dnorm(  (logit(prpFatality.dstb.2000.forward) - logit(0.17))/ ((logit(.26)-logit(.13))/4)   )),
    log(dnorm(  (log(mort.dstb.2018) - log(33))/ ((log(30)-log(36))/4)   )),
    log(dnorm(  (logit(rrNew.2013.18) - logit(0.03))/ ((logit(.02)-logit(.04))/4)   )),
    log(dnorm(  (logit(rrRetreated.2013.18) - logit(0.14))/ ((logit(.12)-logit(.16))/4)   )))
  
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
var = cbind(coefTrans1900=.12, coefTrans2000=.11, coefComTrans=.06,
            probMaxMortalityTB=.05, probMaxSeekCare=.9,probResistance=.01, coefMaxInfect_DR_RelTo_DS=.2)
# set up the original covariance matrix based ont he starting points (assuming normal distribution with 20 sd around mean)
covMat=diag(c((var/40)^2)); covMat
# choose smaller step sizes for the last two params
covMat=cbind(covMat[,c(1:5)],covMat[,6:7]/100);covMat

# define control object
ctrl = create.adaptive.blockwise.metropolis.control( var.names=c("coefTrans1900","coefTrans2000","coefComTrans","probMaxSeekCare","probMaxMortalityTB","probResistance","coefMaxInfect_DR_RelTo_DS"),
                                                     var.blocks = list(step1=c("coefTrans1900","coefTrans2000","coefComTrans","probMaxSeekCare","probMaxMortalityTB"),
                                                                      step2=c("probResistance","coefMaxInfect_DR_RelTo_DS")),
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
starting.values<-rbind( cbind(coefTrans1900=.12, coefTrans2000=.11, coefComTrans=.06, probMaxMortalityTB=.05, probMaxSeekCare=.9,probResistance=.01, coefMaxInfect_DR_RelTo_DS=.2),
                        # cbind(coefTrans1900=.12, coefTrans2000=.11, coefComTrans=.06, probMaxMortalityTB=.05, probMaxSeekCare=.9,probResistance=.015, coefMaxInfect_DR_RelTo_DS=.1),
                        # cbind(coefTrans1900=.12, coefTrans2000=.11, coefComTrans=.06, probMaxMortalityTB=.05, probMaxSeekCare=.9,probResistance=.02, coefMaxInfect_DR_RelTo_DS=.1),
                        # cbind(coefTrans1900=.12, coefTrans2000=.11, coefComTrans=.06, probMaxMortalityTB=.05, probMaxSeekCare=.9,probResistance=.005, coefMaxInfect_DR_RelTo_DS=.3),
                        cbind(coefTrans1900=.12, coefTrans2000=.11, coefComTrans=.06, probMaxMortalityTB=.05, probMaxSeekCare=.9,probResistance=.001, coefMaxInfect_DR_RelTo_DS=.4),
                        # cbind(coefTrans1900=.12, coefTrans2000=.11, coefComTrans=.06, probMaxMortalityTB=.05, probMaxSeekCare=.9,probResistance=.001, coefMaxInfect_DR_RelTo_DS=.5),
                        # cbind(coefTrans1900=.12, coefTrans2000=.11, coefComTrans=.06, probMaxMortalityTB=.05, probMaxSeekCare=.9,probResistance=.0001, coefMaxInfect_DR_RelTo_DS=.6),
                        cbind(coefTrans1900=.12, coefTrans2000=.11, coefComTrans=.06, probMaxMortalityTB=.05, probMaxSeekCare=.9,probResistance=.0001, coefMaxInfect_DR_RelTo_DS=.7))
                        
# Default:
N.ITER = 10 #lenght of the chain
cache.freq = 1
#Command line arguments if we want to change the N.ITER, cache.frequency  
args = commandArgs(trailingOnly=TRUE)
if (length(args) == 2) {  #Arguments are:  # [iterations] [cache.frequency] 
  N.ITER = as.numeric(args[1])
  cache.freq = as.numeric(args[2])
  print(paste("N.ITER ",N.ITER," cache.freq ",cache.freq))
}


mcmc = run.mcmc(control = ctrl,
                n.iter=N.ITER,
                starting.values=starting.values,
                update.frequency=1,
                update.detail = "High",
                # cache.dir = paste("../mcmc.cache",chainID,sep=""),
                cache.dir = "../mcmc.cache.parallel5",
                allow.overwrite.cache = T,
                cache.frequency = cache.freq,
                remove.cache.when.done = F)





# set starting conditions based on last run
# mcmc = assemble.mcmc.from.cache(dir="~/Dropbox/Simulations/MDRTB/mdrtb1/mcmc.cache1x/", allow.incomplete = T)
# new.initial.cov.mat = matrix(rowMeans(sapply(mcmc@chain.states, function(cs){cs@cov.mat})),
#                              nrow=mcmc@n.var, ncol=mcmc@n.var, dimnames=list(mcmc@var.names, mcmc@var.names))
# new.initial.cov.mat
# new.initial.scaling.parameter = list(exp(rowMeans(sapply(mcmc@chain.states, function(cs){cs@log.scaling.parameters[[1]]}))))
# new.initial.scaling.parameter
# simset = extract.simset(mcmc, additional.burn=500, additional.thin=2)
# parameters.posterior = extract.simset.parameter.distribution(simset)
# new.starting.values=get.means(parameters.posterior)
#########


# v<-new.initial.scaling.parameter
# v<-as.numeric(unlist(v))
# new.initial.scaling.parameter<-c(unlist(v)[1],unlist(v)[2],unlist(v)[3],unlist(v)[4])
# names(new.initial.scaling.parameter)<-c("coefTrans","coefComTrans","probMaxSeekCare","probMaxMortalityTB")
# new.initial.scaling.parameter<-as.array(new.initial.scaling.parameter)