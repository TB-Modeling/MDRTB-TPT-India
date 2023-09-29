##https://theoreticalecology.wordpress.com/2010/09/17/metropolis-hastings-mcmc-in-r/
# MCMC Example with MH:

trueA <- 5
trueB <- 0
trueSd <- 10
sampleSize <- 31

# create independent x-values 
x <- (-(sampleSize-1)/2):((sampleSize-1)/2)
# create dependent values according to ax + b + N(0,sd)
y <-  trueA * x + trueB + rnorm(n=sampleSize,mean=0,sd=trueSd)

plot(x,y, main="Test Data")

#we simply calculate the difference between predictions y = b + a*x and the observed y, and then we have to look up the probability densities (using dnorm) for such deviations to occur.
likelihood <- function(param){
  a = param[1]
  b = param[2]
  sd = param[3]
  
  pred = a*x + b
  singlelikelihoods = dnorm(y, mean = pred, sd = sd, log = T)
  sumll = sum(singlelikelihoods) # can sum because we used log
  #why? if we didn, we'd have to multiply. where a lot of small probabilities are multiplied, can get ridiculously small pretty fast (something like 10^-34)
  return(sumll)   
}

# Example: plot the likelihood profile of the slope a
slopevalues <- function(x){return( likelihood(c(x, trueB, trueSd)))}
slopelikelihoods <- lapply(seq(3, 7, by=.05), slopevalues )
plot (seq(3, 7, by=.05), slopelikelihoods , type="l", xlab = "values of slope parameter a", ylab = "Log likelihood")


# Prior distribution
prior <- function(param){
  a = param[1]
  b = param[2]
  sd = param[3]
  aprior = dunif(a, min=0, max=10, log = T)
  bprior = dnorm(b, sd = 5, log = T)
  sdprior = dunif(sd, min=0, max=30, log = T)
  return(aprior+bprior+sdprior)
}
#The product of prior and likelihood is the actual quantity the MCMC will be working on. here we work with the sum because we work with logarithms.
posterior <- function(param){
  return (likelihood(param) + prior(param))
}

### Metropolis-Hastings algorithm
# Starting at a random parameter value
# Choosing a new parameter value close to the old value based on some probability density that is called the proposal function
# Jumping to this new point with a probability p(new)/p(old), where p is the target function, and p>1 means jumping as well


######## Metropolis algorithm ################
# a fucntion proposing a new point in the space for next itteration. takes the current param value and assumes a normal dist with some sd tofor next jump
proposalfunction <- function(param){
  return(rnorm(3,mean = param, sd= c(0.1,0.7,0.3)))
}

run_metropolis_MCMC <- function(startvalue, iterations){
  chain = array(dim = c(iterations+1,3))
  chain[1,] = startvalue
  for (i in 1:iterations){
    proposal = proposalfunction(chain[i,])
    
    probab = exp(posterior(proposal) - posterior(chain[i,])) #subtract logs mean post(p1)/post()
    if (runif(1) < probab){
      chain[i+1,] = proposal
    }else{
      chain[i+1,] = chain[i,]
    }
  }
  return(chain)
}
#choose a starting point
startvalue = c(4,0,10)
#run the mh for 10000 chains
chain = run_metropolis_MCMC(startvalue, 100000)
#take out the first 5000 itterations
burnIn = 5000
#how often was a proposal rejected by the metropolis-hastings acceptance criterion?
acceptance = 1-mean(duplicated(chain[-(1:burnIn),]))
#negative ind returns all rows/columns except negative ones
#duplicated = TRUE means that the proposal was rejected and the chain stayed at the same point.
acceptance
# generally, the closer the proposals are, the larger the acceptance rate. Very high acceptance rates, however, are usually not beneficial: this means that the algorithms is “staying” at the same point,

### Summary: #######################

par(mfrow = c(2,3))
hist(chain[-(1:burnIn),1],nclass=30, , main="Posterior of a", xlab="True value = red line" )
abline(v = mean(chain[-(1:burnIn),1]))
abline(v = trueA, col="red" )
hist(chain[-(1:burnIn),2],nclass=30, main="Posterior of b", xlab="True value = red line")
abline(v = mean(chain[-(1:burnIn),2]))
abline(v = trueB, col="red" )
hist(chain[-(1:burnIn),3],nclass=30, main="Posterior of sd", xlab="True value = red line")
abline(v = mean(chain[-(1:burnIn),3]) )
abline(v = trueSd, col="red" )
plot(chain[-(1:burnIn),1], type = "l", xlab="True value = red line" , main = "Chain values of a", )
abline(h = trueA, col="red" )
plot(chain[-(1:burnIn),2], type = "l", xlab="True value = red line" , main = "Chain values of b", )
abline(h = trueB, col="red" )
plot(chain[-(1:burnIn),3], type = "l", xlab="True value = red line" , main = "Chain values of sd", )
abline(h = trueSd, col="red" )

# for comparison:
summary(lm(y~x))