#'@title The simset class
#'
#'@description A class representing a weighted set of simulations
#'
#'@slot simulations A list of simulations
#'@slot weights A numeric weight for each simulation
#'@slot n.sim The number of unique simulations
#'@slot parameters A matrix of parameters for each simulation, where row i corresponds to the parameters used to run the ith simulation, and each column represents a parameter value
#'@slot parameter.names,n.parameters The names and number of the parameters used to run simulations

#'@name simset
#'@rdname simset
#'@aliases simset-class
#'@exportClass simset
#'@export
setClass('simset',
         representation=list(simulations='list',
                             weights='numeric',
                             n.sim='integer',
                             parameters='matrix',
                             parameter.names='character',
                             n.parameters='integer',
                             parameter.transformations='list',
                             parameter.lower.bounds='numeric',
                             parameter.upper.bounds='numeric'
         ))


#'@title Extracts a simset from an object containing simulations
#'
#'@param object The object from which to extract the simset
#'
#'@export
setGeneric('extract.simset', function(object){
    standardGeneric('extract.simset')
})
setMethod('extract.simset',
          signature(object='mcmcsim'),
def=function(object){
    new('simset',
        parameter.names=object@var.names,
        simulations=object@simulations,
        parameters = t(sapply(1:length(object@simulations), function(i){
            mask = object@simulation.indices==i
            params = object@samples[mask]
            dim(params) = c(sum(mask), object@n.var)
            params[1,]
        })),
        weights=sapply(1:(length(object@simulations)), function(i){
            sum(object@simulation.indices==i)
        }),
        n.sim=length(object@simulations),
        n.parameters=object@n.var,
        parameter.transformations=object@control@transformations)#,
#        parameter.lower.bounds=object@control@lower.bounds,
#        parameter.upper.bounds=object@control@upper.bounds)
})


#'@title Extract a distribution of simulation results
#'
#'@description Gets a probability distribution over the results of a function applied to each simulation or its parameters or both
#'
#'@param simset A \link{simset} object
#'@param fn A function to be applied. If pass.to.fn=='simulation', the function's first parameters should be 'sim' (a simulation objet). If pass.to.fn=='parameters', the function's first argument should be 'parameters' (a vector of parameters). If pass.to.fn=='both', the function's first two parameters should be 'sim' and 'parameters'
#'@param pass.to.fn What to pass to fn as arguments
#'@param ... Additional parameters to be passed to fn
#'@param smooth Whether to return a smoothed distribution over continuous parameters
#'
#'@export
setGeneric("extract.simset.distribution",
           function(simset,
                    fn,
                    pass.to.fn=c('simulation','parameters','both')[1],
                    ...,
                    smooth=F)
{
    standardGeneric("extract.simset.distribution")
})
setMethod("extract.simset.distribution",
          signature(simset='simset', fn='function'),
def=function(simset,
             fn,
             pass.to.fn=c('simulation','parameters','both')[1],
             ...){

    #check pass.to.fn

    if (pass.to.fn == 'simulation')
        points = sapply(simset@simulations, fn, ...)
    else if (pass.to.fn == 'parameters')
        points = apply(simset@parameters, 1, fn, ...)
    else
        points = sapply(1:simset@n.sim, function(i){
            fn(simset@simulations[[i]], simset@parameters[i,], ...)
        })

    if (is.null(dim(points)))
        points = matrix(points, ncol=1)
    else
        points = t(points)

    if (!smooth)
        distributions::create.empiric.distribution(points, weights=simset@weights)
    else
        stop("Have not yet implemented smoothed distributions. Coming soon")
})

#'@title Extract the distribution of parameters in a simset
#'
#'@description Gets a probability distribution over the parameters in a simset
#'
#'@inheritParams extract.simset.distribution
#'@param simset A \link{simset} object
#'@param smooth Whether to return a smoothed distribution over continuous parameters
#'
#'@export
setGeneric("extract.simset.parameter.distribution",
           function(simset, smooth=F)
{
   standardGeneric("extract.simset.parameter.distribution")
})
setMethod("extract.simset.parameter.distribution",
          signature(simset='simset'),
def=function(simset, smooth=F)
{
    if (!smooth)
        distributions::Empiric.Distribution(simset@parameters, weights=simset@weights)
    else
        stop("Have not yet implemented smoothed distributions. Coming soon")
})


#extend.simulations
