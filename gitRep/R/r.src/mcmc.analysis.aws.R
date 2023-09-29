# the purpose of this code is to run mcmc analysis from a cache 
library(ggplot2)
library(mcmc.sim) #runs the MCMC
library(distributions)

# mcmc = assemble.mcmc.from.cache(dir="~/Dropbox/Simulations/MDRTB/mdrtb1/mcmc.cache1/", allow.incomplete = T)
# mcmc = assemble.mcmc.from.cache(dir="~/Dropbox/Simulations/MDRTB/mdrtb/mcmc.cache.parallel.mcmc5/", allow.incomplete = T)
mcmc = assemble.mcmc.from.cache(dir="~/run1/mcmc.cache.parallel/", allow.incomplete = T)
# ##-- MERGE PARALLEL CHAINS --##
# mcmc = assemble.mcmc.from.cache(dir="Caches/mcmc.cache1/", allow.incomplete = T)
# mcmc = mcmc.merge.parallel(mcmc, mcmc1)



##-- To analyze the MCMC chain --##
library(mcmc.sim)
mcmc@n.chains
mcmc@n.iter #see how many iterations we have here (after thinning)

#-- Trace plots - the trace.plot function --#
jpeg("1-tracePlot.jpeg")
trace.plot(mcmc) #by default, this plots all the variables. That's a lot here
dev.off()
################################################################################
#-- Log likelihood and prior --#
################################################################################
#we store the log likelihood and log prior, accessed [chain,iteration]
mcmc@log.likelihoods[1,]
mcmc@log.priors[1,]
#or we can plot them
# likelihood.plot(mcmc)
#since the prior is often quite different from the likelihood, it's useful to plot them separately
# likelihood.plot(mcmc, show.log.prior = F)
jpeg("2-liklihood.jpeg")
likelihood.plot(mcmc, show.log.prior = F, show.log.likelihood = T,show.log.prior.plus.likelihood = F)
dev.off()
################################################################################
#-- Acceptance rates --#
################################################################################
#to check how our acceptance rate over the past 50 iterations
get.cumulative.acceptance.rates(mcmc, window.iterations=50)

#or you can just plot it
jpeg("3-acceptance.jpeg")
acceptance.plot(mcmc, window.iterations = 50)
dev.off()
################################################################################
##-- HOW TO LEVERAGE THE EXISTING CHAIN FOR STARTING VALUES --##
################################################################################
# new.initial.cov.mat = matrix(rowMeans(sapply(mcmc@chain.states, function(cs){cs@cov.mat})),
#                              nrow=mcmc@n.var, ncol=mcmc@n.var, dimnames=list(mcmc@var.names, mcmc@var.names))
# new.initial.cov.mat
# new.initial.scaling.parameter = list(exp(rowMeans(sapply(mcmc@chain.states, function(cs){cs@log.scaling.parameters[[1]]}))))
# new.initial.scaling.parameter
################################################################################
#-- ANALYZE SIMULATED OUTPUTS --#
################################################################################
simset = extract.simset(mcmc)
#if you want to burn extra initial simulations (ie, not use the first simulations), use additional.burn
#If you want to thin further, use additional.thin
# simset = extract.simset(mcmc, additional.burn=0, additional.thin=2)
################################################################################
##-- LOOKING AT THE POSTERIOR DISTRIBUTION OVER PARAMETERS --##
################################################################################
# parameters.posterior = extract.simset.parameter.distribution(simset)
# get.means(parameters.posterior)
# get.intervals(parameters.posterior, coverage=.95)
# cov2cor(get.covariance.matrix(parameters.posterior))
################################################################################
##-- BEST SIMULATIONS?  --##
################################################################################
simset@weights
mat<-cbind(simset@weights,simset@parameters);
mat<-cbind(mat, rep(1:dim(mat)[1]))
colnames(mat)[1]<-"weights"
colnames(mat)[ncol(mat)  ]<-"id"
mat[1,]
#the bst parameter set:
best<-mat[order(mat[,1],decreasing = T),][1,]
best
# 
#the best simulation
D<-simset@simulations[[best["id"]]]
D['rngSeed']
D$year

# varNames<-c("pop", "rPrevDS" ,"rIncDS" ,"rIncDS_fastProg" ,"rIncDS_slowProg" ,"rIncDS_relapse" ,"rIncDS_failure",
#             "rPrevDR" ,"rIncDR" ,"rIncDR_fastProg", "rIncDR_slowProg" ,"rIncDR_relapse" ,"rIncDR_resis", "rIncDR_failure",
#             "rMortalityDS","rMortalityDR" ,"pFatalityDS",
#             "rVisits","rDiagDS","rDiagDR","rDST","rLTFU",
#             "rTrtDS_DS" ,"rTrtDS_DR", "rTrtDR_DR" ,
#             "durDS","durDR",
#             "prpHhPrevDS_diagDS", 
#             "prpHhPrevDR_diagDS",
#             "prpHhPrevTB_diagDS",
#             "prpHhPrevDS_diagDR",
#             "prpHhPrevDR_diagDR",
#             "prpHhPrevTB_diagDR",
#             "prpHhPrevTB_diagTB",
#             "nNewDR","nRetreatedDR",
#             "pHHTrans","pHHInc","pNewDR","pRetreatedDR")
# jpeg("summaryPlots-mcmc.jpeg",width =3000,height = 3000,res = 200)
# par(mfrow=c(7,6))
# lapply(varNames,function(x){
#   plot(x=unlist(D["year"]),y = unlist(D[x]),type="l",col=vCols,lwd=2,main=x )})
# dev.off()

################################################################################
##-- ANALYZE OUTPUTS --##
################################################################################
drawPlot<-function(varName){
  extract.output <- function(sim)
  {
    if(varName=="nIncDS") rv = sim$nIncDS/sim$pop * 100000
    if(varName=="nMortalityDS") rv = sim$nMortalityDS/sim$pop * 100000
    if(varName=="nPrevDS") rv = sim$nPrevDS/sim$pop * 100000
    if(varName=="prpHhPrevTB_diagTB") rv = (sim$prpHhPrevTB_diagTB)
    if(varName=="durDS") rv = (sim$durDS)
    if(varName=="pFatality")  rv=sim$nMortalityDS/sim$nIncDS; 
    if(varName=="pNewDR")  rv=sim$pNewDR;
    if(varName=="pRetreatedDR")  rv=sim$pRetreatedDR;
    # print(rv)
    if (!all(!is.nan(rv)) || !all(!is.na(rv))) {
      r=rep(0,10)
      print("problem!!!")
    }
    names(rv) = sim$year
    rv
  }
  output.distribution = extract.simset.distribution(simset, extract.output)
  #Some quick little code to plot it for you
  intervals = get.intervals(output.distribution, coverage = 0.95)
  df = data.frame(out = get.means(output.distribution),
                  year = c(1900: 1969),
                  ci.lower = intervals[1,],
                  ci.upper = intervals[2,])
  # jpeg(paste("5-o-",varName,".jpeg",sep=""))
  b<-ggplot(df, aes(x=year, y=out, ymin=ci.lower, ymax=ci.upper)) + 
    geom_ribbon(alpha=0.2) + geom_line() + geom_point() + ylim(0,NA)
  #naming the plot
  main="";  h=0;
  if(varName=="nIncDS") {main="Incidence of DSTB"; h=292;}
  if(varName=="nMortalityDS") {main="Mortality of DSTB";h=33;}
  if(varName=="nPrevDS") main="Prevalence of DSTB"
  if(varName=="prpHhPrevTB_diagTB") {main="Proportion of households with prevalent TB at the time of diagnosis";H=.03}
  if(varName=="durDS") {main="Druation of DSTB"; h=14.4;}
  if(varName=="pFatality") {main="Ratio of DSTB Fatality"; h=0.17}
  if(varName=="pNewDR") {main="RR new DRTB"; h=0.03}
  if(varName=="pRetreatedDR") {main="RR retreated DRTB"; h=0.14}
  b=b+ggtitle(main) +geom_hline(yintercept=h,linetype="dashed",color = "red" )
  ggsave(filename =paste("5-o-",varName,".jpeg",sep="") )
  
}

drawPlot(varName = "nIncDS")
drawPlot(varName = "durDS")
drawPlot(varName = "nMortalityDS")
drawPlot(varName = "prpHhPrevTB_diagTB")
drawPlot(varName = "pRetreatedDR")

################################################################################
##-- TO ITERATE THROUGH THE SIMULATIONS MANUALLY --##
################################################################################
#let's plot each simulation as a separate line
# (this is not the most efficient code to do this, but is more readable)
df = NULL
extract.output <- function(sim)
{
  if(varName=="nIncDS") rv = sim$nIncDS/sim$pop * 100000
  if(varName=="nMortalityDS") rv = sim$nMortalityDS/sim$pop * 100000
  if(varName=="nPrevDS") rv = sim$nPrevDS/sim$pop * 100000
  if(varName=="prpHhPrevDS") rv = (sim$prpHhPrevDS)
  if(varName=="nTransDS_HH") rv = sim$nTransDS_HH/sim$nTransDS
  if(varName=="durDS") rv = (sim$durDS)
  if(varName=="pFatality")rv=sim$nMortalityDS/sim$nIncDS;
  if(varName=="pNewDR")  rv=sim$pNewDR;
  if(varName=="pRetreatedDR")  rv=sim$pRetreatedDR;
  names(rv) = sim$year
  print(rv)
  rv
}
varName="nIncDS"
for (i in 1:simset@n.sim)
{
  df = rbind(df,
             data.frame(year=1960:1969,
                        out = extract.output(simset@simulations[[i]]),
                        weight=simset@weights[i],
                        simulation=i))
}
ggplot(df, aes(x=year, y=out, group=simulation)) +
  geom_line(aes(size=weight/20), alpha=0.1) +
  scale_size_continuous() + ylim(0,NA)
simset@weights

