
##---------------------------##
##-- DEFINE THE SUBCLASSES --##
##---------------------------##

setClass('adaptive_blockwise_metropolis_chain_state',
         contains='mcmcsim_chainstate',
         representation=list(cov.mat='matrix',
                             mean.transformed.parameters='numeric',
                             log.scaling.parameters='numeric',
                             block='integer'
         ))

setClass('adaptive_blockwise_metropolis_control',
         contains='mcmcsim_control',
         representation=list(n.blocks='integer',
                             var.blocks='list',
                             initial.covariance.mat='matrix',
                             n.iter.before.use.adaptive.covariance='numeric',
                             adaptive.covariance.base.update='numeric',
                             adaptive.covariance.update.prior.iter='numeric',
                             adaptive.covariance.update.decay='numeric',
                             initial.scaling.parameters='numeric',
                             use.adaptive.scaling='logical',
                             adaptive.scaling.base.update='numeric',
                             adaptive.scaling.update.prior.iter='numeric',
                             adaptive.scaling.update.decay='numeric',
                             reset.adaptive.scaling.update.after='integer',
                             target.acceptance.probability='numeric'
         ))

##------------------##
##-- CONSTRUCTORS --##
##------------------##

setMethod("create.initial.chain.state",
          signature(control = "adaptive_blockwise_metropolis_control", start.parameters="numeric"),
function(control, start.parameters)
{
    start.parameters = check.variable.names(start.parameters, desired.names=control@var.names)
    create.new.mcmcsim.chain.state('adaptive_blockwise_metropolis_chain_state',
                                   current.parameters=start.parameters,
                                   cov.mat=control@initial.covariance.mat,
                                   mean.transformed.parameters=do.transform.parameters(control, start.parameters),
                                   log.scaling.parameters=log(control@initial.scaling.parameters),
                                   block=as.integer(0),
                                   n.accepted=sapply(control@sample.steps, function(step){as.integer(0)}),
                                   run.time=as.numeric(0),
                                   first.step.for.iter=as.integer(NA)
                                   )
})

setMethod("check.merge.controls",
          signature(c1='adaptive_blockwise_metropolis_control', c2='adaptive_blockwise_metropolis_control'),
function(c1, c2, for.serial.merge)
{
    errors = do.check.merge.controls(c1, c2, for.serial.merge=for.serial.merge)

    #Do we want to check all the other parameters?

    errors
})

#'@export
create.metropolis.control <- function(var.names,
                                      simulation.function,
                                      log.prior.distribution,
                                      log.likelihood,
                                      proposal.sds,
                                      thin=length(var.names),
                                      burn=0,
                                      transformations=NULL,
                                      use.adaptive.scaling=F,
                                      adaptive.scaling.base.update=1,
                                      adaptive.scaling.update.prior.iter=10,
                                      adaptive.scaling.update.decay=0.5,
                                      reset.adaptive.scaling.update.after=0,
                                      target.acceptance.probability=0.234,
                                      initial.scaling.parameters=1)
{
    create.adaptive.blockwise.metropolis.control(var.names=var.names,
                                                 simulation.function=simulation.function,
                                                 log.prior.distribution=log.prior.distribution,
                                                 log.likelihood=log.likelihood,
                                                 initial.covariance.mat=diag(proposal.sds^2),
                                                 var.blocks=as.list(var.names),
                                                 thin=thin,
                                                 burn=burn,
                                                 transformations=transformations,
                                                 n.iter.before.use.adaptive.covariance = Inf,
                                                 use.adaptive.scaling=use.adaptive.scaling,
                                                 adaptive.scaling.base.update=adaptive.scaling.base.update,
                                                 adaptive.scaling.update.prior.iter=adaptive.scaling.update.prior.iter,
                                                 adaptive.scaling.update.decay=adaptive.scaling.update.decay,
                                                 reset.adaptive.scaling.update.after=reset.adaptive.scaling.update.after,
                                                 target.acceptance.probability=target.acceptance.probability,
                                                 initial.scaling.parameters=initial.scaling.parameters
                                                 )
}

#'@export
create.adaptive.blockwise.metropolis.control <- function(var.names,
                                                         simulation.function,
                                                         log.prior.distribution,
                                                         log.likelihood,
                                                         initial.covariance.mat,
                                                         var.blocks=list(var.names),
                                                         thin=length(var.blocks),
                                                         burn=0,
                                                         transformations=NULL,
                                                         n.iter.before.use.adaptive.covariance=0,
                                                         adaptive.covariance.base.update=0.2,
                                                         adaptive.covariance.update.prior.iter=500,
                                                         adaptive.covariance.update.decay=0.5,
                                                         use.adaptive.scaling=T,
                                                         adaptive.scaling.base.update=1,
                                                         adaptive.scaling.update.prior.iter=10,
                                                         adaptive.scaling.update.decay=0.5,
                                                         reset.adaptive.scaling.update.after=0,
                                                         target.acceptance.probability=0.234,
                                                         initial.scaling.parameters=2.38^2/sapply(var.blocks, length)
)
{
    # Check initial covariance matrix
    initial.covariance.mat = check.cov.mat.names(initial.covariance.mat,
                                                 desired.names=var.names,
                                                 arg.name.for.error='initial.covariance.mat')

    # Check that var.blocks is valid
    if (class(var.blocks) != 'list' ||
        length(var.blocks) == 0 ||
        any(sapply(var.blocks, class) != 'character'))
        stop("'var.blocks' must be a non-empty list, whose elements are character vectors")

    elements.of.var.blocks = unique(unlist(var.blocks))
    invalid.elements.of.var.blocks = sapply(elements.of.var.blocks, function(name){all(name != var.names)})
    if (any(invalid.elements.of.var.blocks))
        stop(paste0("The following name",
                    ifelse(sum(invalid.elements.of.var.blocks)==1, ' is', 's are'),
                    " are present in 'var.blocks', but are not given in 'var.names':",
                    paste0("'", elements.of.var.blocks[invalid.elements.of.var.blocks], "'", collapse=', ')))

    missing.from.blocks = sapply(var.names, function(name){all(name != unlist(var.blocks))})
    if (any(missing.from.blocks))
        stop(paste0("The following variable",
                    ifelse(sum(missing.from.blocks)==1, ' is', 's are'),
                    " not included in any of the elements of 'var.blocks': ",
                    paste0("'", var.names[missing.from.blocks], "'", collapse=', ')))

    # Set var.blocks names if missing
    if (is.null(names(var.blocks)))
        names(var.blocks) = sapply(var.blocks, function(block){
            if (length(block)==1)
                block
            else
                paste0("<",
                       paste0(block, collapse=','),
                       ">")
        })

    # Check initial scaling parameters and acceptance probabilities
    initial.scaling.parameters = check.variable.names(initial.scaling.parameters,
                                                      desired.names=names(var.blocks),
                                                      arg.name.for.error='initial.scaling.parameters')

    target.acceptance.probability = check.variable.names(target.acceptance.probability,
                                                        desired.names=names(var.blocks),
                                                        arg.name.for.error='target.acceptance.probability')

    # Check adaptive parameters
    if (adaptive.covariance.base.update<0)
        stop("adaptive.covariance.base.update cannot be less than zero")
    if (adaptive.scaling.base.update<0)
        stop("adaptive.scaling.base.update cannot be less than zero")

    if (adaptive.covariance.update.prior.iter<0)
        stop('adaptive.covariance.prior.iter cannot be less than zero')
    if (adaptive.scaling.update.prior.iter<0)
        stop('adaptive.scaling.prior.iter cannot be less than zero')

    if (adaptive.covariance.update.decay<0 || adaptive.covariance.update.decay>1)
        stop('adaptive.covariance.update.decay must be between zero and one')
    if (adaptive.scaling.update.decay<0 || adaptive.scaling.update.decay>1)
        stop('adaptive.scaling.update.decay must be between zero and one')

    # Create the control
    create.mcmcsim.control(class.name='adaptive_blockwise_metropolis_control',
                           var.names=var.names,
                           method='adaptive.blockwise.metropolis',
                           simulation.function=simulation.function,
                           log.prior.distribution=log.prior.distribution,
                           log.likelihood=log.likelihood,
                           thin=cast.to.integer(thin),
                           burn=cast.to.integer(burn),
                           transformations=transformations,
                           sample.steps=names(var.blocks),
                           n.blocks=length(var.blocks),
                           var.blocks=var.blocks,
                           initial.covariance.mat=initial.covariance.mat,
                           n.iter.before.use.adaptive.covariance=n.iter.before.use.adaptive.covariance,
                           adaptive.covariance.base.update=adaptive.covariance.base.update,
                           adaptive.covariance.update.prior.iter=adaptive.covariance.update.prior.iter,
                           adaptive.covariance.update.decay=adaptive.covariance.update.decay,
                           initial.scaling.parameters=initial.scaling.parameters,
                           use.adaptive.scaling=use.adaptive.scaling,
                           adaptive.scaling.base.update=adaptive.scaling.base.update,
                           adaptive.scaling.update.prior.iter=adaptive.scaling.update.prior.iter,
                           adaptive.scaling.update.decay=adaptive.scaling.update.decay,
                           reset.adaptive.scaling.update.after=as.integer(floor(reset.adaptive.scaling.update.after)),
                           target.acceptance.probability=target.acceptance.probability)
}

##--------------------------------------##
##-- THE FUNCTION THAT RUNS THE CHAIN --##
##--------------------------------------##


setMethod("run.single.chain",
          signature(control = "adaptive_blockwise_metropolis_control", chain.state="adaptive_blockwise_metropolis_chain_state"),
function(control,
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
    total.start.time = Sys.time()

    ##--------------------------##
    ##-- Check Initial Values --##
    ##--------------------------##
#Do we want to double check the chain state elements, or just assume they have not been messed with?


    ##--------------------------##
    ##-- Set Up Initial State --##
    ##--------------------------##

    if (is.null(initial.sim))
    {
        if (!is.na(update.frequency) && update.detail != 'none')
            cat("- Running initial simulation...\n")

        current.sim = control@simulation.function(chain.state@current.parameters)
    }
    else
        current.sim = initial.sim

    current.log.prior = control@log.prior.distribution(chain.state@current.parameters)
    if (current.log.prior == -Inf)
        stop("The log prior at the starting state evaluates to -Inf. You must pick another starting state.")

    current.log.likelihood = control@log.likelihood(current.sim)
    current.sim.index = NA

    current.transformed.parameters = do.transform.parameters(control, chain.state@current.parameters)

    if (current.log.likelihood==-Inf)
        print("WARNING: The log likelihood at the starting state evaluates to -Inf. Consider starting in a higher-likelihood region")


    ##------------------------------------##
    ##-- Set Up Variables for the Chain --##
    ##------------------------------------##

    # The random variables for accept/reject
    log.rands = log(runif(n.iter, 0, 1))

    # Indexing iterations
    prior.iterations = chain.state@n.unthinned.after.burn + chain.state@n.unthinned.burned
    to.burn.this.time = control@burn - chain.state@n.unthinned.burned

    # Storing simulations and samples
    n.unique.sim = 0

    # Zero matrix on which to build covariance matrix
    zero.matrix = matrix(0, nrow=control@n.var, ncol=control@n.var, dimnames=list(control@var.names, control@var.names))

    ##---------------------------------##
    ##-- Preallocate Data Structures --##
    ##---------------------------------##

    n.keep = max(0,floor((n.iter-to.burn.this.time)/control@thin))

    rv = create.skeleton.mcmc(control=control,
                              n.iter=n.keep,
                              n.chains=1)

    ##--------------------------------------------##
    ##-- SET UP FOR UPDATING SCALING PARAMETERS --##
    ##--------------------------------------------##

    #accessed [block, var]
    var.is.in.block = matrix(sapply(control@var.names, function(name){
        sapply(control@var.blocks, function(block){
            as.numeric(any(name==block))
        })
    }), ncol=control@n.var, dimnames=list(NULL, control@var.names))

    ##-------------------##
    ##-- Run the Chain --##
    ##-------------------##

    #-- Record start time --#
    iter.start.time = Sys.time()
    for (iter.inc in (1:n.iter))
    {

        #-- Set up iteration indexing --#
        #iter.inc = iter increment - the iterations we are running in this function call
        #iter = the total number of iterations since the start of the chain, including burn
        #iter.after.burn = the total number of iterations since the start of the chain, excluding burn (negative if we have not completed the burn-in)
        #keep.index = the index into our saved data structures at which to store this iteration (if storing)
        #total.iter.inc = iter.inc plus the number of prior iterations we have run this time around
        iter = prior.iterations + iter.inc
        iter.after.burn = iter - control@burn
        keep.index = floor((iter.inc - to.burn.this.time) / control@thin)
        total.iter.inc = prior.n.iter + iter.inc

        #-- Get the block --#
        chain.state@block = as.integer(1) + (chain.state@block %% control@n.blocks)
        block.vars = control@var.blocks[[chain.state@block]]
        if (is.na(chain.state@first.step.for.iter))
            chain.state@first.step.for.iter = chain.state@block

        #-- Print an update --#
        print.updates = !is.na(update.frequency) && update.detail != 'none' &&
            (update.frequency==1 || (total.iter.inc %% update.frequency)==1)
        if (print.updates)
        {

            if ((update.detail=='med' || update.detail=='high' || update.detail=='very_high')
                && total.iter.inc>1)
            {
                # Acceptance rate update
                total.n.accepted = prior.n.accepted + sum(rv@n.accepted.in.burn, na.rm=T) +
                    sum(rv@n.accepted, na.rm=t) + sum(chain.state@n.accepted, na.rm=T)
                cat("  ",
                    round(100*total.n.accepted/(total.iter.inc-1)),
                    "% (",
                    format(total.n.accepted, big.mark=','),
                    "/",
                    format(total.iter.inc-1, big.mark=','),
                    ") of iterations accepted so far\n", sep='')

                # Time update
                time.so.far = prior.run.time + as.numeric(difftime(Sys.time(), total.start.time, units='secs'))
                cat("  Total runtime: ",
                    get.timespan.text(time.so.far, max.spans.to.list = 2, digits.for.last.span = 0),
                    sep='')

                cat(" (",
                    get.timespan.text(time.so.far/(total.iter.inc-1), max.spans.to.list = 2, digits.for.last.span = 1),
                    " per iteration on average)",
                    sep='')

                cat("\n", sep='')
            }


            if (update.detail=='high' || update.detail=='very_high')
                print('---------------------------------------------------------')


            cat("- RUNNING ITERATION ", format(total.iter.inc, big.mark=','), sep='')
            if (update.frequency>1)
                cat(" TO ", format(min(total.iter.inc + update.frequency - 1, total.n.iter), big.mark=','), sep='')
            cat(" OF ", format(total.n.iter, big.mark=','),
                         " (", round(100 * (total.iter.inc-1)/total.n.iter), "% done so far)\n", sep='')


            if ((update.detail=='high' || update.detail=='very_high') && control@n.blocks>1)
            {
                if (is.null(names(control@var.blocks)))
                    print(paste0("Taking a step on <", paste0(block.vars, collapse=', '), ">"))
                else
                    print(paste0("Taking a step on '", names(control@var.blocks)[chain.state@block], "'"))
            }

        }

        #-- Set up the covariance matrix for block --#
        block.cov.mat = exp(chain.state@log.scaling.parameters[chain.state@block]) * chain.state@cov.mat[block.vars,block.vars]

        #-- Print Details --#
        if (print.updates && update.detail=='very_high')
        {
            print(paste0("Scaling Parameters = ", paste0(control@var.names, ' = ', round(exp(chain.state@log.scaling.parameters),4), collapse=', ') ))

#            print(paste0("Decomposed Covariance Matrix (sds on diagonal, correlations off diagonal):"))
#            sds = sqrt(diag(chain.state@cov.mat))
#            to.print = cov2cor(chain.state@cov.mat)
#            diag(to.print) = sds
#            print(round(to.print, 5))
        }

        #-- Propose new parameters --#
        proposed.transformed.parameters = current.transformed.parameters

        if (length(block.vars)==1)
            proposed.transformed.parameters[block.vars] = rnorm(1,
                                                                mean=current.transformed.parameters[block.vars],
                                                                sd=sqrt(block.cov.mat))
        else
            proposed.transformed.parameters[block.vars] = mvtnorm::rmvnorm(1,
                                                                           mean=current.transformed.parameters[block.vars],
                                                                           sigma=block.cov.mat)[1,]

        proposed.parameters = do.reverse.transform.parameters(control, proposed.transformed.parameters)

        proposed.log.prior = control@log.prior.distribution(proposed.parameters)

        if (print.updates && update.detail=='very_high')
        {
            print(paste0("Current params  = [",
                         paste0(paste0(control@var.names, '=', round(chain.state@current.parameters,4)), collapse=", "),
                         "]"))
            print(paste0("Proposed params = [",
                         paste0(paste0(control@var.names, '=', round(proposed.parameters,4)), collapse=", "),
                         "]"))
        }


        if (proposed.log.prior == -Inf)
        {
            acceptance.probability = 0
            accept = F

            #-- Print Details --#
            if (print.updates && (update.detail=='high' || update.detail=='very_high'))
            {
                print(paste0("Log prior evaluates to -Inf.",
                             " --> ",
                             ifelse(accept, "ACCEPT", "REJECT")))
            }
        }
        else
        {
            #-- Run the Sim and Evaluate Likelihood --#
            proposed.sim = control@simulation.function(proposed.parameters)

            proposed.sim.index = NA
            proposed.log.likelihood = control@log.likelihood(proposed.sim)

            #-- Compute the Acceptance Ratio --#
            if (proposed.log.likelihood==-Inf && current.log.likelihood==-Inf)
                log.acceptance.ratio = proposed.log.prior - current.log.prior
            else
                log.acceptance.ratio = proposed.log.likelihood + proposed.log.prior -
                                        current.log.likelihood - current.log.prior

            acceptance.probability = exp(min(0,log.acceptance.ratio))
            accept = log.acceptance.ratio > log.rands[iter.inc]

            #-- Print Details --#
            if (print.updates && (update.detail=='high' || update.detail=='very_high'))
            {
                print(paste0("Current log likelihood = ",
                             round(current.log.likelihood, 4),
                             ", proposed log likelihood = ",
                             round(proposed.log.likelihood, 4)))
                print(paste0("r = ",
                             round(exp(log.acceptance.ratio), 3),
                                     " --> ",
                                     ifelse(accept, "ACCEPT", "REJECT")))
            }
        }

        #-- Accept or Reject --#

        if (accept)
        {
            # Update current state
            current.sim = proposed.sim
            current.sim.index = proposed.sim.index
            current.transformed.parameters = proposed.transformed.parameters
            chain.state@current.parameters = proposed.parameters
            current.log.prior = proposed.log.prior
            current.log.likelihood = proposed.log.likelihood

            # Update accepted counts
            chain.state@n.accepted[chain.state@block] = as.integer(1) + chain.state@n.accepted[chain.state@block]
        }

        #-- Update iter counts in chain state --#
        if (iter.after.burn>0)
            chain.state@n.unthinned.after.burn = iter - control@burn
        else
            chain.state@n.unthinned.burned = iter

        #-- Update covariance matrix, mean transformed parameters, and scaling parameter --#
        if (iter > control@n.iter.before.use.adaptive.covariance)
        {
            covariance.update.step = control@adaptive.covariance.base.update /
                (control@adaptive.covariance.update.prior.iter + iter) ^ control@adaptive.covariance.update.decay

            chain.state@cov.mat = chain.state@cov.mat + covariance.update.step *
                ((current.transformed.parameters - chain.state@mean.transformed.parameters) %*%
                     t(current.transformed.parameters - chain.state@mean.transformed.parameters) - chain.state@cov.mat)
        }

        #make sure we update the mean AFTER updating the covariance matrix
        mean.parameters.update.step = control@adaptive.covariance.base.update /
            iter ^ control@adaptive.covariance.update.decay
        chain.state@mean.transformed.parameters = chain.state@mean.transformed.parameters +
            mean.parameters.update.step * (current.transformed.parameters - chain.state@mean.transformed.parameters)


        if (control@use.adaptive.scaling)
        {
            i.block = ceiling (iter / control@n.blocks)

            #calculate the update step and update
            scale.updates.step = control@adaptive.scaling.base.update / (control@adaptive.scaling.update.prior.iter + i.block) ^ control@adaptive.scaling.update.decay

            chain.state@log.scaling.parameters[chain.state@block] = chain.state@log.scaling.parameters[chain.state@block] +
                scale.updates.step * (acceptance.probability - control@target.acceptance.probability[chain.state@block])
        }

        #-- Update run times --#
        chain.state@run.time = chain.state@run.time +
            as.numeric(difftime(Sys.time(), iter.start.time, units='secs'))
        iter.start.time = Sys.time()

        #-- Store Current State if Warranted by thinning --#

        #wipe the n.accepted and run.time from the burn period
        if (iter==control@burn ||
            (iter < control@burn && iter==n.iter)) #ie, we are on the last iteration and still in the burn period
        {
            rv@n.accepted.in.burn[1,] = chain.state@n.accepted
            chain.state@n.accepted[] = as.integer(0)

            rv@run.times.in.burn[1] = chain.state@run.time
            chain.state@run.time = as.numeric(0)

            chain.state@first.step.for.iter = as.integer(NA)
        }

        if (iter > control@burn && (iter.after.burn %% control@thin)==0) #Store the current state
        {
            # Store sim index
            if (is.na(current.sim.index)) #store the simulation, as we have not before
            {
                n.unique.sim = n.unique.sim + 1
                rv@simulations[[n.unique.sim]] = current.sim
                current.sim.index = n.unique.sim
            }

            # Store sim, samples, likelihood, and prior
            rv@simulation.indices[1,keep.index] = current.sim.index
            rv@samples[1,keep.index,] = chain.state@current.parameters
            rv@log.likelihoods[1,keep.index] = current.log.likelihood
            rv@log.priors[1,keep.index] = current.log.prior

            # Store n.accepted and reset in chain state
            rv@n.accepted[1,keep.index,] = chain.state@n.accepted
            chain.state@n.accepted[] = as.integer(0)
            rv@first.step.for.iter[1,keep.index] = chain.state@first.step.for.iter
            chain.state@first.step.for.iter = as.integer(NA)

            # Store run.times and reset in chain state
            rv@run.times[1,keep.index] = chain.state@run.time
            chain.state@run.time = as.numeric(0)
        }

    }

    ##-- Package up and Return --##

    #Add the chain state and total run time to the rv
    rv@chain.states = list(chain.state)

    rv@total.run.time[1] = as.numeric(difftime(Sys.time(), total.start.time, units='secs'))

    if (return.current.sim)
        list(mcmc=rv, current.sim=current.sim)
    else
        rv
})

