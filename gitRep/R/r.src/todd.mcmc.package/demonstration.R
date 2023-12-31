#PK:
# install.packages("mvtnorm")
# install.packages("tmvtnorm")
# install.packages("matrixcalc")
# install.packages("~/Dropbox/2-MDR TB/Emily/Todd-MCMC/distributions_0.1.0.tar.gz", repos=NULL, type="source")
# install.packages("lhs")
# install.packages("~/Dropbox/2-MDR TB/Emily/Todd-MCMC/mcmc.sim_0.1.0.tar.gz", repos=NULL, type="source")


library(ggplot2)

#The two packages I have written
library(distributions) #defines distributions as objects - handy for processing results (and potentially defining priors)
library(mcmc.sim) #runs the MCMC

HAVE.NOT.INSTALLED.PACKAGES = F #This is what you need to do to install my packages
if (HAVE.NOT.INSTALLED.PACKAGES)
{
    install.packages('distributions_package/distributions_0.1.0.tar.gz', type='source')
    install.packages('mcmc.sim_0.1.0.tar.gz', type='source')
}

##--------------------##
##-- INITIAL SET UP --##
##--------------------##


set.seed = 898798798


##------------------------------------##
##-- SIMULATE 'TRUE' DATA TO FIT TO --##
##------------------------------------##

# Make up some true data we are fitting to
YEARS = 2010:2020
NOTIFICATIONS = rnorm(n=length(YEARS), mean=200 - 5*(YEARS-2010), sd=10)
print(qplot(YEARS, NOTIFICATIONS, geom='line') + ggtitle("Our 'True' Data to fit to"))


##------------------------------------##
##-- DEFINE THE SIMULATION FUNCTION --##
##------------------------------------##

# Defined a toy simulation function
# This just projects a line based off two parameters - slope and intercept - and says that the simulated
#  notifications are whatever is produced from that linear model
run.simulation = function(parameters)
{
    list(simulated.notifications=parameters['intercept'] + parameters['slope'] * (YEARS-2010))
}

##----------------------------##
##-- THE PRIOR DISTRIBUTION --##
##----------------------------##

# Define the log prior for the two parameters
#  intercept ~ normal(250, 50)
#  slope ~ normal(0, 25)
log.prior = function(parameters)
{
    dnorm(parameters['intercept'], mean=250, sd=50, log=T) +
        dnorm(parameters['slope'], mean=0, sd=25, log=T)
}

# This is to show off the distributions package
#  It does the same likelihood but with my nice distribution objects
prior.distribution = join.distributions(
    intercept = Normal.Distribution(mean=250, sd=50),
    slope = Normal.Distribution(mean=0, sd=25)
)

cat("Calculating the likelihood with two versions: \n",
    " Simple prior = ", log.prior(c(intercept=200,slope=-5)),
    " \n  Fancy object prior = ", calculate.density(prior.distribution, c(intercept=200,slope=-5), log=T))

##--------------------##
##-- THE LIKELIHOOD --##
##--------------------##

# Define a simple likelihood
#   For each data point in NOTIFICATIONS, assume the likelihood is
#   NOTIFICATIONS[i] ~ Normal(simulated_notification[i], 25)
log.likelihood = function(sim)
{
    sum(dnorm(NOTIFICATIONS, mean=sim$simulated.notifications, sd=25, log=T))
}


##------------------##
##-- RUN THE MCMC --##
##------------------##

ctrl = create.adaptive.blockwise.metropolis.control(var.names=c('intercept','slope'),
                                                    simulation.function = run.simulation,
                                                    log.prior.distribution = log.prior,
                                                    #To use the distributions package
                                                    #log.prior.distribution = get.density.function(prior.distribution, default.log=T),
                                                    log.likelihood = log.likelihood,
                                                    burn = 1000,
                                                    thin=25,
                                                    initial.covariance.mat = diag(c(10,1)))

#Going to run four chains
n.chains = 4
starting.values = cbind(intercept=rnorm(n.chains, 250, 50),
                        slope=rnorm(n.chains, 0, 25))

RUN.ALL.FOUR.AT.ONCE = F #just to demonstrate you can run these on parallel cores with one call
                         # or separately and then merge later
N.ITER = 5000

if (RUN.ALL.FOUR.AT.ONCE) #on parallel cores
{
    mcmc = run.mcmc(ctrl,
                    n.iter=N.ITER,
                    starting.values=starting.values,
                    update.frequency=1000)
}
if (!RUN.ALL.FOUR.AT.ONCE)
{
    mcmc1 = run.mcmc(ctrl,
                     n.iter=N.ITER,
                     starting.values = starting.values[1,],
                     update.frequency = 1000)
    mcmc2 = run.mcmc(ctrl,
                     n.iter=N.ITER,
                     starting.values = starting.values[2,],
                     update.frequency = 1000)
    mcmc3 = run.mcmc(ctrl,
                     n.iter=N.ITER,
                     starting.values = starting.values[3,],
                     update.frequency = 1000)
    mcmc4 = run.mcmc(ctrl,
                     n.iter=N.ITER,
                     starting.values = starting.values[4,],
                     update.frequency = 1000)

    mcmc = mcmc.merge.parallel(mcmc1, mcmc2, mcmc3, mcmc4)
}

#If you want the MCMC to cache to the disk as it goes, use arguments
# cache.frequency and cache.dir

##-----------------------------##
##-- DIAGNOSTICS ON THE MCMC --##
##-----------------------------##

print(trace.plot(mcmc)) + ggtitle("Trace Plot")
print("Gelman's Rhat statistic for convergence:")
print(get.rhats(mcmc))

##-----------------------------------------------------------------##
##-- PROFIT! (ie, inference on the posterior set of simulations) --##
##-----------------------------------------------------------------##

simset = extract.simset(mcmc)
#This object now contains the set of all simulations from the MCMC
# We can do fun stuff like this:

#Get the model simulated true notifications
extract.model.notifications.by.year = function(sim)
{
    rv = sim$simulated.notifications
    names(rv) = as.character(YEARS)
    rv
}
notifications.distribution = extract.simset.distribution(simset, extract.model.notifications.by.year)

#This just tells us what our model said every year
print("Simulated Notifications by Year")
print(cbind(mean.notifications=get.means(notifications.distribution),
        t(get.intervals(notifications.distribution))))

#Or we can plot it
df = as.data.frame(cbind(value=get.means(notifications.distribution),
                         t(get.intervals(notifications.distribution))))
df$year = YEARS
df$type = 'Model'
df = rbind(df,
           data.frame(value=NOTIFICATIONS,
                      ci.lower=NA,
                      ci.upper=NA,
                      year=YEARS,
                      type='Truth'))
print(ggplot(df, aes(year, value, ymin=ci.lower, ymax=ci.upper, color=type, fill=type)) +
    geom_ribbon(alpha=0.2) + geom_line(size=1) + geom_point(size=5) +
        ggtitle("Simulated Notifications (mean, 95% CI) vs Truth") + ylab("Notifications") + xlab('Year'))


#We can extract what the notifications reduction is from 2010 to 2020
extract.notifications.reduction = function(sim)
{
    n.2010 = sim$simulated.notifications[1]
    n.2020 = sim$simulated.notifications[11]

    (n.2010-n.2020)/n.2010
}

reduction.distribution = extract.simset.distribution(simset, extract.notifications.reduction)
print("Reduction in Notifications:")
print(c(mean.reduction=get.means(reduction.distribution), get.intervals(reduction.distribution)))
