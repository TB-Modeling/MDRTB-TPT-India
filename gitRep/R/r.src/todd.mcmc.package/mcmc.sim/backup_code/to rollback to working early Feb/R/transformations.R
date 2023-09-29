
setClass('transformation',
         representation=list(transform='function',
                             reverse.transform='function',
                             name='character'))

#'@export
create.transformation <- function(transform.function, reverse.transform.function, name)
{

    new('transformation',
        transform=transform.function,
        reverse.transform=reverse.transform.function,
        name=name)
}

DEFINED.TRANSFORMATIONS = list(log=create.transformation(transform.function = log,
                                                 reverse.transform.function = exp,
                                                 name='log'),
                       logit=create.transformation(transform.function = function(x){log(x) - log(1-x)},
                                                   reverse.transform.function = function(x){1/(1+exp(-x))},
                                                   name='logit'))

process.transformations <- function(var.names, transformations)
{
    transformations = as.list(transformations)

    if (is.null(transformations) || length(transformations)==0)
        transformations = lapply(var.names, function(name){NULL})
    if (is.null(names(transformations)))
    {
        if (length(transformations) != length(var.names))
            stop(paste0("If names are not specified for the 'transformations' parameter, it must have the same length as 'var.names' (",
                        length(var.names), ")"))
    }
    else
    {
        unused.transformation.names = setdiff(names(transformations), var.names)
        if (length(unused.transformation.names) > 0)
            warning(paste0("The following transformation",
                           ifelse(length(unused.transformation.names>1, "s were", 'was')),
                           " given but ",
                           ifelse(length(unused.transformation.names>1, "are not variables", 'is not a variable')),
                           " specified in 'var.names': ",
                           paste0("'", unused.transformation.names, "'", collapse=", ")
            ))

        transformations = transformations[var.names]
    }
    names(transformations) = var.names

    transformations = lapply(transformations, function(one.transformation){
        if (is(one.transformation, 'transformation'))
            one.transformation
        else if (is(one.transformation, 'character'))
        {
            if (length(one.transformation)!=1)
                stop("Only single names (ie not vectors) are permitted as elements of 'transformations")

            if (any(names(DEFINED.TRANSFORMATIONS)==tolower(one.transformation)))
                DEFINED.TRANSFORMATIONS[[tolower(one.transformation)]]
            else
                stop(paste0("'", one.transformation, "' is not a previously defined transformation.",
                            " Defined transformations are: ",
                            paste0("'", names(DEFINED.TRANSFORMATIONS), "'", collapse=", ")))
        }
        else
            stop("transformations must be either an instance of the 'transformation' class or a character name of a defined transformation")
    })

    transformations
}

do.transform.parameters <- function(control, parameters)
{
    rv = sapply(1:control@n.var, function(i){
        if (is.null(control@transformations[[i]]))
            parameters[i]
        else
            control@transformations[[i]]@transform(parameters[i])
    })

    if (any(is.na(rv)))
    {
        na.mask = is.na(rv)
        if (is.null(control@var.names))
        {
            stop(paste0("The transformation",
                        ifelse(sum(na.mask)>1, "s", ""),
                        " for the ",
                        get.ordinal.list.string((1:control@n.var)[na.mask]),
                        "parameter",
                        ifelse(sum(na.mask)>1, "s", ""),
                        "produced NA value",
                        ifelse(sum(na.mask)>1, "s", ""),
                        ". The input parameter",
                        ifelse(sum(na.mask)>1, "s were", " was"),
                        ": ",
                        paste0(parameters[na.mask], collapse=", ")))
        }
        else
        {
            stop(paste0("The transformation",
                        ifelse(sum(na.mask)>1, "s", ""),
                        " for the following parameter",
                        ifelse(sum(na.mask)>1, "s", ""),
                        "produced NA value",
                        ifelse(sum(na.mask)>1, "s", ""),
                        ": ",
                        paste0("'", control@var.names[na.mask], "'", collapse=', '),
                        ". The input parameter",
                        ifelse(sum(na.mask)>1, "s were", " was"),
                        ": ",
                        paste0(parameters[na.mask], collapse=", ")))
        }
    }

    if (any(is.infinite(rv)))
    {
        inf.mask = is.infinite(rv)
        if (is.null(control@var.names))
        {
            stop(paste0("The transformation",
                        ifelse(sum(inf.mask)>1, "s", ""),
                        " for the ",
                        get.ordinal.list.string((1:control@n.var)[inf.mask]),
                        "parameter",
                        ifelse(sum(inf.mask)>1, "s", ""),
                        "produced infinite value",
                        ifelse(sum(inf.mask)>1, "s", ""),
                        ". The input parameter",
                        ifelse(sum(inf.mask)>1, "s were", " was"),
                        ": ",
                        paste0(parameters[inf.mask], collapse=", ")))
        }
        else
        {
            stop(paste0("The transformation",
                        ifelse(sum(inf.mask)>1, "s", ""),
                        " for the following parameter",
                        ifelse(sum(inf.mask)>1, "s", ""),
                        "produced infinite value",
                        ifelse(sum(inf.mask)>1, "s", ""),
                        ": ",
                        paste0("'", control@var.names[inf.mask], "'", collapse=', '),
                        ". The input parameter",
                        ifelse(sum(inf.mask)>1, "s were", " was"),
                        ": ",
                        paste0(parameters[inf.mask], collapse=", ")))
        }
    }

    rv
}

do.reverse.transform.parameters <- function(control, transformed.parameters)
{
    rv = sapply(1:control@n.var, function(i){
        if (is.null(control@transformations[[i]]))
            parameters[i]
        else
            control@transformations[[i]]@reverse.transform(transformed.parameters[i])
    })

    if (any(is.na(rv)))
    {
        na.mask = is.na(rv)
        if (is.null(control@var.names))
        {
            stop(paste0("The reverse transformation",
                        ifelse(sum(na.mask)>1, "s", ""),
                        " for the ",
                        get.ordinal.list.string((1:control@n.var)[na.mask]),
                        "parameter",
                        ifelse(sum(na.mask)>1, "s", ""),
                        "produced NA value",
                        ifelse(sum(na.mask)>1, "s", ""),
                        ". The input parameter",
                        ifelse(sum(na.mask)>1, "s were", " was"),
                        ": ",
                        paste0(parameters[na.mask], collapse=", ")))
        }
        else
        {
            stop(paste0("The reverse transformation",
                        ifelse(sum(na.mask)>1, "s", ""),
                        " for the following parameter",
                        ifelse(sum(na.mask)>1, "s", ""),
                        "produced NA value",
                        ifelse(sum(na.mask)>1, "s", ""),
                        ": ",
                        paste0("'", control@var.names[na.mask], "'", collapse=', '),
                        ". The input parameter",
                        ifelse(sum(na.mask)>1, "s were", " was"),
                        ": ",
                        paste0(parameters[na.mask], collapse=", ")))
        }
    }

    rv
}
