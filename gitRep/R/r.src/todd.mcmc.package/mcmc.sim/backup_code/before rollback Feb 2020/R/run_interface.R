
##------------------------------------------------------------------##
##-- THE INTERFACE TO ACTUALLY RUN MCMC BASED ON A CONTROL OBJECT --##
##------------------------------------------------------------------##


##-------------------------##
##-- NON-CACHE INTERFACE --##
##-------------------------##

#'@export
run.mcmc <- function(control,
                     n.iter,
                     starting.values=NULL,
                     prior.mcmc=NULL,
                     update.frequency=floor(n.iter/10),
                     update.detail=c('none','low','med','high')[3],
                     cache.frequency=200,
                     cache.dir=NA,
                     allow.overwrite.cache=F,
                     merge.with.prior.mcmc=T,
                     cores = if (.Platform$OS.type=='windows') 1 else parallel::detectCores(),
                     remove.cache.when.done=T)
{
    if (!is.na(cache.frequency) && !is.null(cache.dir) && !is.na(cache.dir))
        run.mcmc.with.cache(control=control,
                            n.iter=n.iter,
                            starting.values=starting.values,
                            prior.mcmc=prior.mcmc,
                            update.frequency=update.frequency,
                            update.detail=update.detail,
                            cache.frequency=cache.frequency,
                            cache.dir=cache.dir,
                            allow.overwrite.cache=allow.overwrite.cache,
                            merge.with.prior.mcmc=merge.with.prior.mcmc,
                            cores=cores,
                            remove.cache.when.done=remove.cache.when.done)
    else
    {
        # Parse the arguemnts to start the mcmc
        mcmc.arguments = parse.mcmc.arguments(control,
                                              starting.values=starting.values,
                                              prior.mcmc=prior.mcmc)

        if (!merge.with.prior.mcmc)
            prior.mcmc = NULL

        #-- Check cores argument --#
        if (is.na(cores))
            cores = 1
        cores = max(1, cores)

        if (mcmc.arguments$n.chains>1 && cores>1 && .Platform$OS.type=='windows')
        {
            print("Running on Windows allows us to use only one core")
            cores = 1
        }

        cores = min(cores, mcmc.arguments$n.chains)

        #-- Run in parallel --#
        parallel.results = parallel::mclapply(1:mcmc.arguments$n.chains, mc.cores=cores, function(chain){
            run.single.chain(control=control,
                             chain.state=mcmc.arguments$chain.states[[chain]],
                             n.iter=n.iter,
                             update.frequency=update.frequency,
                             update.detail=update.detail)
        })

        #-- Merge and Return --#

        # Merge parallel
        if (mcmc.arguments$n.chains==1)
            rv = parallel.results[[1]]
        else
            rv = mcmc.merge.parallel(parallel.results)

        # Merge serial
        if (!is.null(prior.mcmc) && merge.with.prior.mcmc)
            rv = mcmc.merge.serial(prior.mcmc, rv)

        rv
    }
}

##---------------------##
##-- CACHE INTERFACE --##
##---------------------##

#'@export
run.mcmc.with.cache <- function(control,
                     n.iter,
                     starting.values=NULL,
                     prior.mcmc=NULL,
                     update.frequency=floor(n.iter/10),
                     update.detail=c('none','low','med','high')[3],
                     cache.frequency=200,
                     cache.dir=NA,
                     allow.overwrite.cache=T,
                     merge.with.prior.mcmc=T,
                     cores = if (.Platform$OS.type=='windows') 1 else parallel::detectCores(),
                     remove.cache.when.done=T)
{
    create.mcmc.cache(dir=cache.dir,
                      control=control,
                      n.iter=n.iter,
                      starting.values=starting.values,
                      prior.mcmc=prior.mcmc,
                      cache.frequency=cache.frequency,
                      allow.overwrite.cache=allow.overwrite.cache,
                      merge.with.prior.mcmc=merge.with.prior.mcmc)

    run.mcmc.from.cache(dir=cache.dir,
                        chains=NULL,
                        update.frequency=update.frequency,
                        update.detail=update.detail,
                        cores=cores,
                        remove.cache.when.done=remove.cache.when.done)
}

#'@title Set up a cache for running an MCMC from
#'
#'@param dir The directory where the cache should be stored
#'@inheritParams run.mcmc
#'@param cache.frequency How often to cache to the disk
#'@param cache.prefix ???
#'@param allow.overwrite.cache If a previous cache is in place in dir, the function will throw an error if allow.overwrite.cache is FALSE. If allow.overwrite.cache is TRUE, it will overwrite the previously existing cache
#
#'@export
create.mcmc.cache <- function(dir,
                              control,
                              n.iter,
                              starting.values=NULL,
                              prior.mcmc=NULL,
                              cache.frequency=1000,
                              allow.overwrite.cache=F,
                              merge.with.prior.mcmc=T
)
{
    # Parse the arguemnts to start the mcmc
    mcmc.arguments = parse.mcmc.arguments(control,
                                          starting.values=starting.values,
                                          prior.mcmc=prior.mcmc)

    if (!merge.with.prior.mcmc)
        prior.mcmc = NULL

    # Check if cache already exists
    if (cache.exists(dir))
    {
        if (allow.overwrite.cache)
            do.remove.cache(dir)
        else
            stop(paste0("A cache for MCMC already exists in directory '", dir, "'"))
    }

    # Create the cache
    do.create.cache(dir=dir,
                    control=mcmc.arguments$control,
                    chain.states=mcmc.arguments$chain.states,
                    n.iter=n.iter,
                    cache.frequency=cache.frequency,
                    prior.mcmc=prior.mcmc)
}

extend.mcmc.cache.parallel <- function(dir)
{
    stop('not yet implemented')
}

extend.mcmc.cache.serial <- function(dir)
{
    stop('not yet implemented')
}

#'@title Run MCMC from a previously set-up cache
#'
#'@description Runs MCMC iterations from a previously set up cache
#'
#'@param dir The directory where the cache is stored
#'@param chains The numbers of the chains to run. If passed NULL (the default), runs all chains
#'@param update.frequency How often to print updates (print an update every update.frequency iterations). If NA, no updates are printed
#'@param update.detail The level of detail for updates
#'@param cores The maximum number of cores to use to run chains in parallel
#'
#'@return An MCMC object with length(chains) number of chains
#'
#'@export
run.mcmc.from.cache <- function(dir,
                                chains=NULL,
                                update.frequency=100,
                                update.detail=c('none','low','med','high')[3],
                                cores = if (.Platform$OS.type=='windows') 1 else parallel::detectCores(),
                                remove.cache.when.done=F)
{
    #-- Load cache control --#
    if (!cache.exists(dir))
        stop("No cache for MCMC exists in directory '", dir, "'")

    check.cache.corrupted(dir)

    global.control = get.cache.global.control(dir)

    #-- Check chains --#
    if (is.null(chains))
        chains = 1:global.control@n.chains
    else if (length(setdiff(chains, 1:global.control@chains))>0)
        stop(paste0("chains must include only numbers from 1 to ", global.control@n.chains))

    #-- Check cores argument --#
    if (is.na(cores))
        cores = 1
    cores = max(1, cores)

    if (length(chains)>1 && cores>1 && .Platform$OS.type=='windows')
    {
        print("Running on Windows allows us to use only one core")
        cores = 1
    }

    cores = min(cores, length(chains))


    #-- Run chains in parallel --#

    parallel::mclapply(chains, mc.cores=cores,
                       do.run.single.chain.with.cache,
                       dir=dir, global.control=global.control,
                       update.frequency=update.frequency, update.detail=update.detail)


    # Aggregate it together

    assemble.mcmc.from.cache(dir, chains=chains, allow.incomplete = F, remove.cache = remove.cache.when.done)
}

#'@title Delete previously set-up cache for running MCMC
#'
#'@inheritParams run.mcmc.from.cache
#'@param allow.remove.incomplete If FALSE, the function will only succeed in removing the cached files if all the MCMC chains are complete. If TRUE, removes all file regardless
#'
#'@export
remove.mcmc.cache <- function(dir,
                              allow.remove.incomplete=F)
{
    if (cache.exists(dir))
    {
        sampling.status = get.cache.sampling.status(dir)
        if (allow.remove.incomplete || sum(sampling.status$chunks.done)==0 ||
            sum(sampling.status$chunks.done) == sum(sampling.status$total.chunks))
            do.remove.cache(dir)
        else
            stop(paste0("Sampling is partially complete for the cache at '", dir, "'. To force removal, use allow.remove.incomplete=T"))
    }
}

#'@title Assemble an MCMC object from iterations stored in a cache
#'
#'@inheritParams run.mcmc.from.cache
#'@param allow.incomplete If FALSE, throws an error if the cache has not finished running. If TRUE, assembles an MCMC object out of all chains where any iterations have been run past the burn period, with a number of iterations equal to the mininum number of iterations run in any chain past the burn period
#'
#'@return An \link{mcmc.sim} object
#'
#'@export
assemble.mcmc.from.cache <- function(dir,
                                     allow.incomplete=F,
                                     chains=NULL,
                                     remove.cache.when.done=F)
{
    # Load cache control
    if (!cache.exists(dir))
        stop("No cache for MCMC exists in directory '", dir, "'")

    check.cache.corrupted(dir)

    global.control = get.cache.global.control(dir)


    # Check chains

    if (is.null(chains))
        chains = 1:global.control@n.chains
    else if (length(setdiff(chains, 1:global.control@n.chains))>0)
        stop(paste0("chains must include only numbers from 1 to ", global.control@n.chains))

    chunks.done = chunks.done.per.chain(dir, global.control=global.control, chains=chains)
    max.chunk = min(chunks.done)
    if (max.chunk < global.control@n.chunks) #we are not all done
    {
        if (allow.incomplete)
        {
            past.burn = global.control@save.chunk[chunks.done]
            if (!any(past.burn))
            {
                stop(paste0("Cannot assemble partial MCMC. Chain ",
                            ifelse(length(chains)==1, '', 's'),
                            paste0(chains, collapse=', '),
                            " ", ifelse(length(chains)==1, 'has', 'have'),
                            " not generated any samples past the burn-in period"))
            }
            else
            {
                max.chunk = min(chunks.done[past.burn])
                chains = chains[past.burn]

                print(paste0("Sampling is incomplete. Using ",
                             max.chunk, " chunk",
                             ifelse(max.chunk==1, '', 's'),
                             " from ",
                             length(chains), " chain",
                             ifelse(length(chains)==1, '', 's')
                             ))
            }
        }
        else
        {
            if (length(chains)==global.control@n.chains)
                stop(paste0("Samples cached in '", dir, "' are not completed. Use allow.incomplete=TRUE to assemble an MCMC from the partial results"))
            else if (length(chains==1))
                stop(paste0("Samples from chain ", chains,
                            " cached in '", dir, "' are not completed. Use allow.incomplete=TRUE to assemble an MCMC from the partial results"))
            else
                stop(paste0("Samples from chains ",
                            paste0(chains, collapse=', '),
                            " cached in '", dir, "' are not completed. Use allow.incomplete=TRUE to assemble an MCMC from the partial results"))
        }
    }


    rv = do.assemble.multiple.chains.from.cache(dir,
                                           global.control=global.control,
                                           chains=chains,
                                           chunks=1:max.chunk)

    if (remove.cache.when.done)
        remove.mcmc.cache(dir, allow.remove.incomplete = T)

    rv
}


#'@title Report the progress of an MCMC process running from a cache
#'
#'@inheritParams run.mcmc.from.cache
#'
#'@export
cache.progress <- function(dir)
{
    stop('not implemented yet')
}

##-------------##
##-- HELPERS --##
##-------------##

#To parse arguments to a run.mcmc.<x> function
parse.mcmc.arguments <- function(control,
                                 starting.values,
                                 prior.mcmc)
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
    }

    list(n.chains=n.chains,
         chain.states=chain.states,
         control=control)
}
