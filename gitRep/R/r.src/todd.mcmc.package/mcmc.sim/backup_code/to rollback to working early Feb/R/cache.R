
##----------------------##
##-- THE MASTER CLASS --##
##----------------------##

setClass('mcmcsim_cachecontrol',
         representation=list(
             control='mcmcsim_control',
             n.chunks='integer',
             chunk.size='integer',
             save.chunk='logical',
             chunk.filenames='character',
             control.filename='character',
             chunk.done='logical',
             n.accepted.so.far='integer',
             total.runtime='numeric',
             chain.state='mcmcsim_chainstate',
             need.to.remove.dir='logical'
         ))


##-------------------------------------------------##
##-- WRAPPERS TO ACTUALLY USE THE RUN WITH CACHE --##
##-------------------------------------------------##

#'@export
resume.run.with.cache <- function(cache.dir,
                                  update.frequency=floor(n.iter/10),
                                  update.detail=c('none','low','med','high')[3],
                                  allow.overwrite.cache=T)
{
    cache.control.file = file.path(cache.dir, CACHE.SUBDIR.NAME, CACHE.CONTROL.NAME)
    if (!dir.exists(cache.dir))
        stop(paste0("The directory '", cache.dir, "' does not exist"))
    if (!file.exists(cache.control.file))
        stop(paste0("Directory '", cache.dir, "' has not been set up as a cache"))

    x=load(cache.control.file)
    cache.control = get(x[1])

    if (!any(class(cache.control)=='mcmcsim_cachecontrol'))
        stop(paste0("The cache control contained in '",
                    cache.dir,
                    "' did not encode a mcmcsim_cachecontrol object"))

    run.single.chain.with.cache(cache.control,
                                cache.dir=cache.dir,
                                update.frequency=update.frequency,
                                update.detail=update.detail)
}

##--------------------------------------##
##-- THE MAIN RUN-WITH-CACHE FUNCTION --##
##--------------------------------------##

setGeneric('run.single.chain.with.cache',
           function(cache.control, cache.dir, update.frequency, update.detail){standardGeneric('run.single.chain.with.cache')})

setMethod('run.single.chain.with.cache',
          signature(cache.control='mcmcsim_cachecontrol'),
function(cache.control, cache.dir, update.frequency, update.detail)
{
    #-- Check that done files are present --#
    for (chunk in (1:cache.control@n.chunks)[cache.control@chunk.done & cache.control@save.chunk])
    {
        if (!file.exists(file.path(cache.dir, cache.control@chunk.filenames[chunk])))
            stop(paste0("The ", get.ordinal(chunk),
                        " chunk of iterations was done, but the file where it was stored ('",
                        file.path(cache.dir, cache.control@chunk.filenames[chunk]),
                        "') does not exist."))
    }

    #-- Check that to-be-done files are not present --#
    for (chunk in (1:cache.control@n.chunks)[!cache.control@chunk.done & cache.control@save.chunk])
    {
        if (file.exists(file.path(cache.dir, cache.control@chunk.filenames[chunk])))
            stop(paste0("The file where the results of the ", get.ordinal(chunk),
                        " chunk of iterations is to be stored, ('",
                        file.path(cache.dir, cache.control@chunk.filenames[chunk]),
                        "') already exists."))
    }

    #-- Run the loop --#
    initial.sim = NULL
    total.n.iter = sum(cache.control@chunk.size)
    total.n.iter.undone = sum(cache.control@chunk.size[!cache.control@chunk.done])
    n.iter.so.far = sum(cache.control@chunk.size[cache.control@chunk.done])

    if (!is.na(update.frequency))
        print(paste0("RUNNING ",
                     format(total.n.iter.undone, big.mark=','),
                     " ITERATIONS ACROSS ",
                     sum(!cache.control@chunk.done),
                     " CHUNKS"))

    start.time = Sys.time()
    while (any(!cache.control@chunk.done))
    {
        chunk = (1:cache.control@n.chunks)[!cache.control@chunk.done][1]

        # Run MCMC
        mcmc.and.sim = run.single.chain(control=cache.control@control,
                                        chain.state=cache.control@chain.state,
                                        n.iter=cache.control@chunk.size[chunk],
                                        update.frequency=update.frequency,
                                        update.detail=update.detail,
                                        initial.sim=initial.sim,
                                        total.n.iter=total.n.iter,
                                        prior.n.iter=n.iter.so.far,
                                        prior.n.accepted=cache.control@n.accepted.so.far,
                                        prior.run.time=cache.control@total.runtime,
                                        return.current.sim=T)

        # Save MCMC to disk
        if (cache.control@save.chunk[chunk])
        {
            mcmc = mcmc.and.sim$mcmc
            save(mcmc, file=file.path(cache.dir, cache.control@chunk.filenames[chunk]))
        }

        # Update the cache control and save to disk
        cache.control@chain.state = mcmc.and.sim$mcmc@chain.states[[1]]
        cache.control@chunk.done[chunk] = T
        cache.control@n.accepted.so.far = cache.control@n.accepted.so.far +
            as.integer(sum(mcmc.and.sim$mcmc@n.accepted) + sum(mcmc.and.sim$mcmc@n.accepted.in.burn))
        cache.control@total.runtime = cache.control@total.runtime +
            as.numeric(difftime(Sys.time(), start.time, units='secs'))
        start.time = Sys.time()

        save(cache.control, file=file.path(cache.dir, cache.control@control.filename))

        # Update other state variables
        initial.sim = mcmc.and.sim$current.sim
        n.iter.so.far = n.iter.so.far + cache.control@chunk.size[chunk]

        # Clear the memory devoted to the last MCMC
        mcmc = NULL
        mcmc.and.sim = NULL
        gc()
    }

    #-- Load and merge --#
    if (!is.na(update.frequency))
        print("DONE RUNNING CHUNKS. LOADING CACHED RESULTS AND MERGING")

    mcmc.objects = lapply(cache.control@chunk.filenames[cache.control@save.chunk], function(filename){
        print(paste0("Loading results at '", filename, "'"))
        load(file.path(cache.dir, filename))
        mcmc
    })
    mcmc = mcmc.merge.serial(mcmc.objects)

    #-- Remove cache files --#
    delete.cache.control(cache.control, cache.dir)

    #-- Return --#
    mcmc
})


##--------------------------##
##-- CREATE CACHE CONTROL --##
##--------------------------##

CACHE.SUBDIR.NAME = 'cache'
CACHE.CONTROL.NAME = 'control.Rdata'
create.cache.control <- function(control,
                                 chain.state,
                                 n.iter,
                                 cache.frequency,
                                 cache.dir,
                                 allow.overwrite.cache)
{

    # Some Checks
    if (!dir.exists(cache.dir))
    {
        dir.created = dir.create(cache.dir)
        if (!dir.created)
            stop("The given cache.dir, '", cache.dir, "' does not exist and could not be created.")
#        stop("The given cache.dir, '", cache.dir, "' does not exist.")
    }

    if (is.null(cache.frequency) ||
        (class(cache.frequency) != 'numeric' && class(cache.frequency)!='integer') ||
        is.na(cache.frequency) ||
        length(cache.frequency) != 1 ||
        cache.frequency%%1 != 0)
        stop("cache.frequency must be a scalar integer")

    # Set up chunks and sizes
    n.chunks = ceiling(n.iter/cache.frequency)
    chunk.size = c(rep(cache.frequency, n.chunks-1), n.iter-cache.frequency*(n.chunks-1))

    #-- Set Up and Check File Names --#
    need.to.save = cumsum(chunk.size) > control@burn
    num.to.save = sum(need.to.save)

    #-- Set up filenames --#
    control.filename = file.path(CACHE.SUBDIR.NAME, CACHE.CONTROL.NAME)
    chunk.filenames = c(rep(as.character(NA), n.chunks-num.to.save),
                        file.path(CACHE.SUBDIR.NAME, paste0('cached', 1:num.to.save, '.Rdata')))

    need.to.remove.dir = !dir.exists(cache.dir)

    #-- Package it up --#
    cache.control = new('mcmcsim_cachecontrol',
        control=control,
        n.chunks=as.integer(n.chunks),
        chunk.size=as.integer(chunk.size),
        save.chunk=need.to.save,
        chunk.filenames=chunk.filenames,
        control.filename=control.filename,
        chunk.done=rep(F,n.chunks),
        chain.state=chain.state,
        n.accepted.so.far=as.integer(0),
        total.runtime=as.numeric(0),
        need.to.remove.dir=need.to.remove.dir)


    #-- Make directories if needed --#
    if (need.to.remove.dir)
        dir.create(dir)

    subdir = file.path(cache.dir, CACHE.SUBDIR.NAME)
    if (dir.exists(subdir))
    {
        if (allow.overwrite.cache)
        {
            print(paste0("DELETING previously existing cache subdirectory: '",
                         subdir, "'"))
            unlink(subdir, recursive = T)
        }
        else
            stop("The cache subdirectory '",
                 subdir,
                 "' already exists.")
    }
    dir.create(subdir)


    #-- Save to file and return --#
    save(cache.control, file=file.path(cache.dir, cache.control@control.filename))
    cache.control
}

delete.cache.control <- function(cache.control, cache.dir)
{
    unlink(file.path(cache.dir, CACHE.SUBDIR.NAME), recursive = T)
    if (cache.control@need.to.remove.dir)
    {
        if(length(dir(all.files=TRUE)) == 0)
            unlink(cache.dir, recursive=F)
        else
            print(paste0("Unable to delete directory '",
                         cache.dir,
                         "' because it is not empty even after deleting cache"))
    }
}
