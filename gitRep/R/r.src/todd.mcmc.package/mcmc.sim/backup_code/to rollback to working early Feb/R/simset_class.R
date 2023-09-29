
setClass('simset',
         representation=list(parameter.names='character',
                             simulations='list',
                             parameters='array',
                             weights='numeric',
                             n.sim='integer',
                             n.parameters='integer'
         ))



setGeneric("extract.distribution", function(simset, fn) {
    standardGeneric("extract.distribution")
})

