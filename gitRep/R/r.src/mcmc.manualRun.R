#prop fatality>1 causes an error


# the purpose of this code is run the mcmc.mh using Todd's package

### Unknown simulation parameters
# coefTrans= [0-inf]
# coefComTrans= [0-1]
# probMaxSeekCare=[0-1]
# probMaxMortalityTB=[0-1]

### Calibration targets to fit to:
# Incidence DSTB
# Duration DSTB
# Prop of DSTB mortality to incidence


####################################################################################################
# Manunal test of linkage to C++
####################################################################################################
source("r/src/mcmc.functions.R")
# initial parameter to start the chain
coefTrans=.1
coefComTrans=0.06
probMaxSeekCare=0.9
probMaxMortalityTB=0.05

#getting a named list
parameters=mget(c("coefTrans","coefComTrans","probMaxSeekCare","probMaxMortalityTB"))
parameters
#log.prior(parameters  )

# where the config file is located
base.config.file = "src/seedConfig.cfg"
# where executable is located
base.model.file="src/mdrtb.out"

sim<-run.simulation(parameters = parameters)
sim
log.lik(sim)


####################################################################################################
# Manunal test of metropolis hesting
####################################################################################################
startvalue=t(parameters)


log.posterior <- function(param){
  ll.sim=log.lik(run.simulation(param))
  ll.prior=log.prior(param)
  return ( ll.sim+ll.prior)
}
proposalfunction <- function(param){
  var<-as.list(rnorm(4,mean = param, sd= c(0.01,0.01,0.01,.01)))
  names(var)<-names(param)
  return(var)
}
run_metropolis_MCMC <- function(startvalue, iterations){
  chain = array(dim = c(iterations+1,4))
  chain[1,] = unlist(startvalue)
  colnames(chain)=names(startvalue)
  
  for (i in 1:iterations){
    proposal = proposalfunction(chain[i,])
    print("new proposal "); print(t(proposal))
    
    probab = exp(log.posterior(proposal) - log.posterior(chain[i,])) #subtract logs mean post(p1)/post()
    print("prob of acceptance ");print(probab)
    
    if (runif(1) < probab){
      chain[i+1,] = proposal
      print("accepted")
    }else{
      chain[i+1,] = chain[i,]
      print("rejected")
    }
  }
  return(chain)
}

coefTrans=.1
coefComTrans=0.06
probMaxSeekCare=0.9
probMaxMortalityTB=0.05
#getting a named list
startvalue=mget(c("coefTrans","coefComTrans","probMaxSeekCare","probMaxMortalityTB"))
itarations=10
run_metropolis_MCMC(startvalue ,iterations )


analyzeOutputs("")
 

