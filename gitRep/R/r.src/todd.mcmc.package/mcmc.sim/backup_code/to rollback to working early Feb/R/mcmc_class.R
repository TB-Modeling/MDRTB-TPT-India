
# In general, use of MCMC here is going to go as follows
# Create an MCMC control object
# Run one or more chains
# Merge the chains into a single MCMC object
# Extend chains if desired
# Extract the posterior distribution
# Profit


##------------------------------##
##-- MASTER CLASS DEFINITIONS --##
##------------------------------##

setClass('mcmcsim_control',
         representation=list(var.names='character',
                             n.var='integer',
                             method='character',
                             thin='integer',
                             burn='integer',
                             simulation.function='function',
                             log.prior.distribution='function',
                             log.likelihood='function',
                             transformations='list',
                             sample.steps='character'))

setClass('mcmcsim',
         representation=list(control='mcmcsim_control',
                             var.names='character',
                             n.var='integer',
                             simulations='list',
                             simulation.indices='matrix',
                             samples='array',
                             log.likelihoods='matrix',
                             log.priors='matrix',
                             n.chains='integer',
                             chain.states='list',
                             n.iter='integer',
                             n.accepted='array',
                             run.times='matrix',
                             first.step.for.iter='matrix',
                             n.accepted.in.burn='matrix',
                             run.times.in.burn='numeric',
                             total.run.time='numeric'))

setClass('mcmcsim_chainstate',
         representation=list(
             current.parameters='numeric',
             n.unthinned.after.burn='integer',
             n.unthinned.burned='integer',
             n.accepted='integer',
             run.time='numeric',
             first.step.for.iter='integer'
         ))

##-------------------------------------------------------------------##
##-- CLASS METHODS THAT WILL BE IMPLEMENTED AT THE SUB-CLASS LEVEL --##
##--                                                               --##
##--        (To be called internally by the MCMC package -         --##
##--                    not directly by users)                     --##
##--                                                               --##
##-------------------------------------------------------------------##

setGeneric("run.single.chain", function(control,
                                        chain.state,
                                        n.iter,
                                        update.frequency=floor(n.iter/10),
                                        update.detail='med',
                                        initial.sim=NULL,
                                        total.n.iter=n.iter,
                                        prior.n.iter=0,
                                        prior.n.accepted=0,
                                        prior.run.time=0,
                                        return.current.sim=F)
{
    standardGeneric("run.single.chain")
})

setGeneric("create.initial.chain.state", function(control,
                                                  start.parameters)
{
    standardGeneric("create.initial.chain.state")
})

##-------------------------------------------------------------------##
##-- CLASS METHODS THAT *MAY* BE OVERRIDDEN AT THE SUB-CLASS LEVEL --##
##-------------------------------------------------------------------##

#'@title Check whether mcmc objects resulting from two mcmcsim_control objects can be merged
#'@return If the two controls are merge-able, returns an empty character vector. Otherwise, returns a vector of the reasons why the two cannot be merged
#'@export
setGeneric("check.merge.controls", function(c1, c2, for.serial.merge=T)
{
    standardGeneric('check.merge.controls')
})
setMethod("check.merge.controls",
          signature(c1='mcmcsim_control', c2='mcmcsim_control'),
          def = function(c1, c2, for.serial.merge=T)
{
    do.check.merge.controls(c1, c2, for.serial.merge)
})

##---------------------------------------##
##-- RUNNING AND EXTENDING MCMC CHAINS --##
##---------------------------------------##

#'@export
setGeneric("run.mcmc",
           function(control=NULL,
                    n.iter,
                    starting.values=NULL,
                    prior.mcmc=NULL,
                    update.frequency=floor(n.iter/10),
                    update.detail=c('none','low','med','high')[3],
                    cache.frequency=200,
                    cache.dir=NA,
                    cache.prefix='',
                    allow.overwrite.cache=T,
                    merge.with.prior.mcmc=T,
                    cores=parallel::detectCores()){
               standardGeneric('run.mcmc')
           })

setMethod("run.mcmc",
          signature(control='mcmcsim_control'),
function(control,
         n.iter,
         starting.values=NULL,
         prior.mcmc=NULL,
         update.frequency=floor(n.iter/10),
         update.detail=c('none','low','med','high')[3],
         cache.frequency=200,
         cache.dir=NA,
         cache.prefix='',
         allow.overwrite.cache=T,
         merge.with.prior.mcmc=T,
         cores=parallel::detectCores() )
{
    #-- Check Prior MCMC or starting values as appropriate --#
    if (is.null(prior.mcmc))
    {
        col.or.val = 'column'
        if (class(starting.values)=='numeric' || class(starting.values)=='integer')
        {
            col.or.val = 'element'
            starting.values = matrix(starting.values, nrow=1, dimnames=list(NULL, names(starting.values)))
        }

        if (class(starting.values)=='matrix' ||
                 (class(starting.values)=='array' && length(dim(starting.values))==2))
        {
            if (is.null(dimnames(starting.values)[[2]]))
            {
                if (dim(starting.values)[2] != control@n.vars)
                    stop("Starting values must have ",
                         control@n.var, " ",
                         col.or.val, ifelse(control@n.var==1,'','s'),
                         ", one for each variable detailed in control")
            }
            else
            {
                missing.var.names = control@var.names[sapply(control@var.names, function(name){
                    all(dimnames(starting.values)[[2]] != name)
                })]

                if (length(missing.var.names)>0)
                    stop(paste0("The following variables are not present among the named ",
                                col.or.val, "s in starting.values: ",
                                paste0("'", missing.var.names, "'", ', ')))
            }

            n.chains = dim(starting.values)[1]
            chain.states = lapply(1:n.chains, function(chain){
                create.initial.chain.state(control, start.parameters=check.variable.names(starting.values[chain,], desired.names=control@var.names))
            })
        }
        else
            stop("starting.values must be either a numeric with one value for each variable detailed in control, or a matrix with one column for each variable detailed in control")

    }
    else
    {
        if (all(class(prior.mcmc) != 'mcmcsim'))
            stop("prior.mcmc must be an object of class 'mcmcsim'")

        n.chains = prior.mcmc@n.chains
        chain.states = prior.mcmc@chain.states
        control = prior.mcmc@control

        if (!merge.with.prior.mcmc)
            prior.mcmc = NULL #does this actually do anything? The goal would be to free up memory
    }

    #-- Check cores argument --#
    if (is.na(cores))
        cores = 1
    cores = max(1, cores)

    if (n.chains>1 && cores>1 && .Platform$OS.type=='windows')
    {
        print("Running on Windows allows us to use only one core")
        cores = 1
    }

    cores = min(cores, n.chains)

    #-- Run in parallel --#
    parallel.results = parallel::mclapply(1:n.chains, mc.cores=cores, function(chain){
        do.run.single.chain(control=control,
                            chain.state=chain.states[[chain]],
                            n.iter=n.iter,
                            update.frequency=update.frequency,
                            update.detail=update.detail,
                            cache.frequency=cache.frequency,
                            cache.dir=cache.dir,
                            allow.overwrite.cache=T)
    })


    #-- Merge and Return --#

    # Merge parallel
    if (n.chains==1)
        rv = parallel.results[[1]]
    else
        rv = mcmc.merge.parallel(parallel.results)

    # Merge serial
    if (!is.null(prior.mcmc) && merge.with.prior.mcmc)
        rv = mcmc.merge.serial(prior.mcmc, rv)

    rv
})

#'The helper to actually run the chains
do.run.single.chain <- function(control,
                                chain.state,
                                n.iter,
                                update.frequency,
                                update.detail,
                                cache.frequency=200,
                                cache.dir=NA,
                                allow.overwrite.cache=F)
{
    if (is.na(cache.dir))  # If not using caching, just run it and return
    {
        run.single.chain(control=control,
                         chain.state=chain.state,
                         n.iter=n.iter,
                         update.frequency=update.frequency,
                         update.detail=update.detail)
    }
    else
    {
        cache.control = create.cache.control(control=control,
                                             chain.state=chain.state,
                                             n.iter=n.iter,
                                             cache.frequency=cache.frequency,
                                             cache.dir=cache.dir,
                                             allow.overwrite.cache=allow.overwrite.cache)

        run.single.chain.with.cache(cache.control,
                                    cache.dir=cache.dir,
                                    update.frequency=update.frequency,
                                    update.detail=update.detail)
    }
}



setGeneric("extract.simset", function(mcmc, n.unique.sim, n.additional.burn) {
    standardGeneric("extract.simset")
})





##--------------------------##
##-- PRIVATE CONSTRUCTORS --##
##--------------------------##


# Creates a skeleton MCMC object in which to put samples
create.skeleton.mcmc <- function(control,
                                 n.iter,
                                 n.chains=1,
                                 class.name='mcmcsim',
                                 ...)
{
    # Set array names
    if (n.iter==0)
        iterations.name = character()
    else
        iterations.name = 1:n.iter

    sample.dimnames = list(chain=1:n.chains, iteration=iterations.name, variable=control@var.names)
    sim.index.dimnames = sample.dimnames[1:2]
    n.accepted.dimnames = list(chain=1:n.chains, iteration=iterations.name, step=control@sample.steps)

    # Make data structures
    simulation.indices = matrix(as.integer(NA), nrow=n.chains, ncol=n.iter, dimnames=sim.index.dimnames)
    samples = array(as.numeric(NA), dim=sapply(sample.dimnames, length), dimnames=sample.dimnames)
    log.likelihoods = matrix(as.numeric(NA), nrow=n.chains, ncol=n.iter, dimnames=sim.index.dimnames)
    log.priors = matrix(as.numeric(NA), nrow=n.chains, ncol=n.iter, dimnames=sim.index.dimnames)
    n.accepted = array(as.integer(NA), dim=sapply(n.accepted.dimnames, length), dimnames=n.accepted.dimnames)
    run.times = matrix(as.numeric(NA), nrow=n.chains, ncol=n.iter, dimnames=sim.index.dimnames)
    first.step.for.iter = matrix(as.integer(NA), nrow=n.chains, ncol=n.iter, dimnames=sim.index.dimnames)

    run.times.in.burn = rep(as.numeric(0), n.chains)
    n.accepted.in.burn = matrix(as.integer(0), nrow=n.chains, ncol=length(control@sample.steps),
                                dimnames=list(chain=1:n.chains, step=control@sample.steps))
    total.run.time = rep(as.numeric(NA), n.chains)
    simulations = list()

    # Package and return
    new(Class = class.name,
        control=control,
        var.names=control@var.names,
        n.var=control@n.var,
        simulations=simulations,
        simulation.indices=simulation.indices,
        samples=samples,
        log.likelihoods=log.likelihoods,
        log.priors=log.priors,
        n.chains=as.integer(n.chains),
        n.iter=as.integer(n.iter),
        n.accepted=n.accepted,
        first.step.for.iter=first.step.for.iter,
        run.times=run.times,
        n.accepted.in.burn=n.accepted.in.burn,
        run.times.in.burn=run.times.in.burn,
        total.run.time=total.run.time)
}

# Create a formal MCMC object out of samples from a chain
package.mcmcsim <- function(control,
                            samples,
                            simulations,
                            simulation.indices,
                            log.likelihoods,
                            log.priors,
                            chain.states,
                            n.accepted,
                            class.name='mcmcsim',
                            ...)
{
stop("Not using this anymore. Use create.skeleton.mcmc")
    # Set array names
    n.chains = length(chain.states)
    n.iter = length(simulation.indices)

    if (n.iter==0)
        iterations.name = character()
    else
        iterations.name = 1:n.iter

    sample.dimnames = list(chain=1:n.chains, iteration=iterations.name, variable=control@var.names)
    sim.index.dimnames = sample.dimnames[1:2]
    n.accepted.dimnames = list(chain=1:n.chains, iteration=iterations.name, step=control@sample.steps)

    # Check array names
    if (class(simulation.indices)!='matrix' || !list.equals(dimnames(simulation.indices), sim.index.dimnames))
        simulation.indices = matrix(simulation.indices, nrow=n.chains, ncol=n.iter, dimnames=sim.index.dimnames)
    if (class(samples) != 'array' || !list.equals(dimnames(samples), sample.dimnames))
        samples = array(samples, dim=sapply(sample.dimnames, length), dimnames=sample.dimnames)
    if (class(log.likelihoods) != 'matrix' || !list.equals(dimnames(log.likelihoods), sim.index.dimnames))
        log.likelihoods = matrix(log.likelihoods, nrow=n.chains, ncol=n.iter, dimnames=sim.index.dimnames)
    if (class(log.priors) != 'matrix' || !list.equals(dimnames(log.priors), sim.index.dimnames))
        log.priors = matrix(log.priors, nrow=n.chains, ncol=n.iter, dimnames=sim.index.dimnames)
    if (class(n.accepted) != 'array' || !list.equals(dimnames(n.accepted), n.accepted.dimnames))
        n.accepted = array(n.accepted, dim=sapply(n.accepted.dimnames, length), dimnames=n.accepted.dimnames)

    # Package and return
    new(Class = class.name,
        control=control,
        simulations=simulations,
        simulation.indices=simulation.indices,
        samples=samples,
        log.likelihoods=log.likelihoods,
        log.priors=log.priors,
        n.chains=as.integer(1),
        n.iter=n.iter,
        n.accepted=n.accepted,
        chain.states=chain.states)
}

create.mcmcsim.control <- function(class.name,
                                   var.names,
                                   method,
                                   simulation.function,
                                   log.prior.distribution,
                                   log.likelihood,
                                   thin,
                                   burn,
                                   transformations,
                                   sample.steps,
                                   ...)
{
    #process transformations
    transformations = process.transformations(var.names, transformations)

    new(Class=class.name,
        var.names=var.names,
        n.var=length(var.names),
        method=method,
        simulation.function=simulation.function,
        log.prior.distribution=log.prior.distribution,
        log.likelihood=log.likelihood,
        thin=thin,
        burn=burn,
        transformations=transformations,
        sample.steps=sample.steps,
        ...
        )
}

create.new.mcmcsim.chain.state <- function(class.name, current.parameters, ...)
{
    new(Class=class.name,
        current.parameters=current.parameters,
        n.unthinned.after.burn=as.integer(0),
        n.unthinned.burned=as.integer(0),
        ...)
}
