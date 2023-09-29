library(mcmc.sim)
library(distributions)
library(ggplot2)

#just going to load up your mcmc
mcmc = assemble.mcmc.from.cache('mcmc.cache/',T)

##-- HOW TO MERGE PARALLEL CHAINS --##

#I'm going to copy your mcmc and pretend they were three chains run in parallel
mcmc.1 = mcmc.2 = mcmc.3 = mcmc
merged = mcmc.merge.parallel(mcmc.1, mcmc.2, mcmc.3)

#or if you have an arbitrary number of chains, you can pass them as a list
list.of.separate.chains = list(mcmc.1, mcmc.2, mcmc.3)
merged = mcmc.merge.parallel(list.of.separate.chains)


##-- HOW TO LEVERAGE THE EXISTING CHAIN FOR STARTING VALUES --##

new.initial.cov.mat = matrix(rowMeans(sapply(mcmc@chain.states, function(cs){cs@cov.mat})),
                             nrow=mcmc@n.var, ncol=mcmc@n.var, dimnames=list(mcmc@var.names, mcmc@var.names))

new.initial.scaling.parameter = list(exp(rowMeans(sapply(mcmc@chain.states, function(cs){cs@log.scaling.parameters[[1]]}))))


##-- LOOKING AT THE POSTERIOR DISTRIBUTION OVER PARAMETERS --##

parameters.posterior = extract.simset.parameter.distribution(simset)
get.means(parameters.posterior)
cov2cor(get.covariance.matrix(parameters.posterior))

##-- HOW TO EXTRACT THE POSTERIOR DISTRIBUTION (for things besides the parameters) --#

simset = extract.simset(mcmc)

#this function is going to be called from extract.simset.distribution on every simulation
extract.incidence <- function(sim)
{
    rv = sim$nIncDS
    names(rv) = sim$year
    rv
}

#extract.simset.distribution calls the given function (extract.incidence) 
# on every simulation in the simset
#Then it makes a distribution out of the results of the function call across
# all the simulations
incidence.distribution = extract.simset.distribution(simset, extract.incidence)

#Some quick little code to plot it for you
intervals = get.intervals(incidence.distribution, coverage = 0.95)
df = data.frame(incidence = get.means(incidence.distribution),
                year = 1960:1969,
                ci.lower = intervals[1,],
                ci.upper = intervals[2,])
ggplot(df, aes(x=year, y=incidence, ymin=ci.lower, ymax=ci.upper)) + 
    geom_ribbon(alpha=0.2) + geom_line() + geom_point() + ylim(0,NA)


##-- TO ITERATE THROUGH THE SIMULATIONS MANUALLY --##

#let's plot each simulation as a separate line
# (this is not the most efficient code to do this, but is more readable)
df = NULL
for (i in 1:simset@n.sim)
{
    df = rbind(df,
               data.frame(year=1960:1969,
                          incidence = extract.incidence(simset@simulations[[i]]),
                          weight=simset@weights[i],
                          simulation=i))
}
ggplot(df, aes(x=year, y=incidence, group=simulation)) +
    geom_line(aes(size=weight/20), alpha=0.1) +
    scale_size_continuous() + ylim(0,NA)


