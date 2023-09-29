
#'@title Generate a trace plot from an mcmc run
#'
#'@param mcmc An mcmc.sim object
#'@param var.names,exact.var.names var.names is the names of variables to include. If exact.var.names is TRUE, matches the given strings exactly. If exact.var.names is FALSE, matches any variable name that begins with any element of var.names, where the * character is treated as a wild card
#'
#'@export
trace.plot <- function(mcmc,
                       var.names=mcmc@control@var.names,
                       exact.var.names=F)
{
    var.names = match.var.names(mcmc, var.names, exact.var.names)

    samples = mcmc@samples[,,var.names]
    dim(samples) = c(chain=mcmc@n.chains, iteration=mcmc@n.iter, variable=length(var.names))
    dimnames(samples) = list(chain=NULL, iteration=NULL, variable=var.names)

    df = reshape2::melt(samples)
    ggplot2::ggplot(df, ggplot2::aes(iteration, value, color=chain)) +
        ggplot2::geom_line() + ggplot2::facet_wrap(~variable, scales='free_y')
}

density.plot <- function(mcmc)
{

}

#'@export
acceptance.plot <- function(mcmc,
                            window.iterations=ceiling(200/mcmc@control@thin),
                            by.block=F,
                            aggregate.chains=F,
                            blocks=mcmc@control@sample.steps,
                            exact.block.names=F)
{
    rates = extract.cumulative.acceptance.rate(mcmc=mcmc,
                                       window.iterations=window.iterations,
                                       by.block=by.block,
                                       aggregate.chains=aggregate.chains,
                                       blocks=blocks,
                                       exact.block.names=exact.block.names)
    df = reshape2::melt(rates)
    df = df[!is.na(df$value),]

    if (window.iterations >= mcmc@n.iter)
        y.lab = "Cumulative Acceptance Rate"
    else
        y.lab = paste0("Acceptance Rate over prior ",
                       format(window.iterations, big.mark=','),
                       " iterations")

    if (by.block)
        rv = ggplot2::ggplot(df, ggplot2::aes(iteration, value, color=block))
    else if (!aggregate.chains)
        rv = ggplot2::ggplot(df, ggplot2::aes(iteration, value, color=chain))
    else
        rv = ggplot2::ggplot(df, ggplot2::aes(iteration, value))

    if (.hasSlot(mcmc@control, 'target.acceptance.probability'))
        rv = rv + geom_hline(yintercept = mcmc@control@target.acceptance.probability, linetype='dashed')

    rv = rv + ggplot2::geom_line()

    if (by.block && !aggregate.chains && mcmc@n.chains>1)
        rv = rv + ggplot2::facet_wrap(~chain)

    rv = rv + ggplot2::ylim(0,1) + ggplot2::ylab(y.lab)

    rv
}

extract.distribution <- function(mcmc)
{

}



#'@export
#'
#'@return Returns an array indexed [chain,iteration,block]. If by.block==T, then there is one value of block for each var.block. If by.block==F, then there is only a single value of block ('all'). The values of the array represent the cumulative fraction of transitions that have been accepted from the end of the burn period up to and including the current iteration (including thinned transitions)
extract.cumulative.acceptance.rate <- function(mcmc,
                                               window.iterations=ceiling(200/mcmc@control@thin),
                                               by.block=F,
                                               aggregate.chains=F,
                                               blocks=mcmc@control@sample.steps,
                                               exact.block.names=F)
{
    blocks = match.block.names(mcmc, blocks, exact.block.names)
    dim.names = list(chain=1:mcmc@n.chains, iteration=1:mcmc@n.iter, block=blocks)

    n.accepted = mcmc@n.accepted[,,blocks]
    dim(n.accepted) = sapply(dim.names, length)
    dimnames(n.accepted) = dim.names

    cum.n.accepted = apply(n.accepted, c(1,3), cumsum)
    cum.n.accepted = apply(cum.n.accepted, c(2,1,3), function(x){x})
    dim(cum.n.accepted) = sapply(dim.names, length)
    dimnames(cum.n.accepted) = dim.names

    n.unthinned = get.n.unthinned.by.block(mcmc)[,,blocks]
    dim(n.unthinned) = sapply(dim.names, length)
    dimnames(n.unthinned) = dim.names

    cum.n.unthinned = apply(n.unthinned, c(1,3), cumsum)
    cum.n.unthinned = apply(cum.n.unthinned, c(2,1,3), function(x){x})
    dim(cum.n.unthinned) = sapply(dim.names, length)
    dimnames(cum.n.unthinned) = dim.names

    if (window.iterations < mcmc@n.iter)
    {
        num.to.subtract = mcmc@n.iter-window.iterations
        cum.n.accepted[,window.iterations + 1:num.to.subtract,] = cum.n.accepted[,window.iterations + 1:num.to.subtract,] -
            cum.n.accepted[,1:num.to.subtract,]
        cum.n.unthinned[,window.iterations + 1:num.to.subtract,] = cum.n.unthinned[,window.iterations + 1:num.to.subtract,] -
            cum.n.unthinned[,1:num.to.subtract,]
    }

    rv = apply(cum.n.accepted, (1:3)[c(!aggregate.chains, T, by.block)], sum) /
            apply(cum.n.unthinned, (1:3)[c(!aggregate.chains, T, by.block)], sum)

    dim.names = dim.names[c(!aggregate.chains, T, by.block)]
    dim(rv) = sapply(dim.names, length)
    dimnames(rv) = dim.names

    rv
}

#'@export
get.n.unthinned.by.block <- function(mcmc)
{
    dim.names = list(chain=1:mcmc@n.chains, iteration=1:mcmc@n.iter, block=mcmc@control@sample.steps)
    rv = array(0, dim=sapply(dim.names, length), dimnames=dim.names)
    n.blocks = length(mcmc@control@sample.steps)

    for (chain in 1:mcmc@n.chains)
    {
        counts = t(sapply(1:mcmc@n.iter, function(iter){
            raw.counts = 1 + ((mcmc@first.step.for.iter[iter] - 2 + 1:mcmc@control@thin) %% n.blocks)
            sapply(1:n.blocks, function(block){
                sum(raw.counts==block)
            })
        }))

        rv[chain,,] = counts
    }

    rv


}
