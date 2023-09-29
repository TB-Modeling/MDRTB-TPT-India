# setting up priors
# install.packages("LearnBayes")
library(LearnBayes)
par(mfrow=c(2,2))
#Prob Mortality
# For mortality, the total number of patients who are diagnosed and never start treatment, combined with the average treatment delay, 
# could set an upper bound for the monthly mortality rate at peak disease severity. 
# In this study https://www.ncbi.nlm.nih.gov/pubmed/28222095, more than 60% of patients eventually started on treatment, and the average 
# delay was ~1 month, so that puts a ceiling of 40% on monthly mortality, even in this population with high HIV prevalence. Most of those 
# were probably still alive at 1 month, and mortality in now-HIV and non-DR populations would be lower. (I'm looking at DR-TB data only 
# because the delays, but the same estimates could apply to all TB.) On the other hand, the mortality rate after TB treatment begins sets a 
# floor for the untreated mortality rate. In India, about 4% of TB patients are reported to die during treatment (per WHO global report, plus 
# probably uncounted deaths among the 15% 'lost to follow up' and 'not evaluated'), with higher mortality in the earlier weeks of treatment, 
# so you could use 1%/month as your lower bound. As for how we choose the prior within that, I don't have a strong opinion, but we should 
# probably be consistent. One option is to use a calculator to get the shape parameters for a beta distribution that spans this 0.01-0.4 range 
# as the 95% CI
quantile1=list(p=.025, x=0.01)     # 2.5% quantile should be 0.01
quantile2=list(p=.975, x=0.20)      # 97.5% quantile should be 0.40
params<-beta.select(quantile1, quantile2)
hist(rbeta(n=1000000,shape1 = params[1],shape2 = params[2]),main="probMaxMortality ~ beta(1.41,8.65)",xlab="",ylab="",breaks = 100)
# EK: “Month 1” isn’t the first month of illness in the data above. It’s the first month of treatment delay for patients with DR TB who have 
# gotten a DR diagnosis. That’s pretty late in the game, and I think it’s reasonable to assume their disease has reached the 9-month plateau already. 


# ###################################################################################################
# pSeekingcare
# we could use studies from India on care-seeking delay, e.g. https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0152287: median 
# delay was 15 days, ~75% sought care within a month, and ~90% sought care within 2 months, all supporting an estimate of about 0.75/mo. 
# EK: Similar to above, the data I’m citing are about the amount of time that patients report having had whatever symptoms finally led them to seek care.
# It’s not the total time with TB. It’s not even the total duration of symptoms, since prevalence surveys make clear that the total duration of symptomatic
# TB (a person would say “yes I have a cough” if you asked them, for example) is longer than the duration that patients report having had the (presumably 
# more severe) symptoms that led to their diagnosis. I think it makes sense to use these data to estimate care seeking behavior once they reach the fully s
# ymptomatic stage.
# Or, if you account for the fact that some people seek care before they reach max infectiousness, then you could estimate a slightly higher value for our 
# parameter. But I would consider the data above to represent a point close to the month-nine plateau, not month 1.
# I do hear you saying that this is not really just prob of seeking care but also of getting diagnosed and offered treatment. In that case, we should probably
# include the delays on the health system side as well in our parameter estimate. 
# In the paper cited above, the median total delay is ~35 days, and exponential decay gets you to 50% untreated at 35 days if your rate of diagnosis and treatment 
# is 0.45/month, so we could use 0.45 instead of the 0.75 I suggested before. A range of 0.3/month (median 60 days to diagnosis) to 0.65/month (median 20 days 
# to diagnosis) seems like a reasonable confidence interval, or you could push it a bit higher to account for diagnoses before peak severity
quantile1=list(p=.25, x=.3)     
quantile2=list(p=.75, x=.65)      
params<-beta.select(quantile1, quantile2)
hist(rbeta(n=1000000,shape1 = params[1],shape2 = params[2]),main="probMaxSeekcare~beta(1.87,2.06)",xlab="",ylab="",breaks=100)


computeMedTimeToVisit<-function(m){
  days=0
  month=0
  b=T
  while (b){
    if (month>9) rate=m else rate=m/9*month
    if(runif(1)<rate)
      b=F
    else{
      days=days+30
      month=month+1}
  }
  return(days)
}
v<-lapply(c(1:10000),function(x){
  return(computeMedTimeToVisit(.7))
})
mean(unlist(v))

# ###################################################################################################
# coefTrans
# maybe we could get the right order of magnitude by using estimates of the annual risk of TB infection in the population (~2%/year for India, according to 
# the same IJTLD paper of Nim's that I sent by email Wednesday) combined with the estimated number of contact pairs that are modeled to occur per year between a TB 
# case and an uninfected person per year? 
# (2% infected per population per year) *(~80% of infections outside of household) / (~0.2% TB cases per population)*(~300*12 contact events per case per year)*
# ( ~0.5 at risk for new infection per contact event [roughly estimating ~50% will have LTBI already]) = 0.004 .  That worked out nicely! 
# I think that means we can justify setting our prior pretty close to the values that have been working well in manual calibration. 
# Maybe a gamma distribution with a mean of 0.004 and a pretty wide shape?
mean=.1
shape=3
scale=mean/shape;scale
var<-rgamma(n = 100000,shape = shape,scale = scale)
hist(var,breaks = 100,main="coefTrans ~gamma(3,0.033)")
range(var)
mean(var);median(var)
var[var>1]


hist(runif(100000),main="coefComTrans ~Unif(0,1)",breaks = 100)

# ###################################################################################################
# ProbResistace
# for pResistance, the prior I suggested last week was a beta with a mean of 1% and a 95% quantile range from 0.1% to 3%. 
# (I’m estimating here a weighted probability of RR acquisition in a DS-TB population with ~10% INH resistance, and references 
# include Dick Menzies’s two systematic reviews from Plos Med 2009, and a systematic review of INH-R outcomes by Gegia et al 
# Lancet ID 2017.) We can be pretty confident that 0.0001 is too low. On the other hand, the prior for reduction in fitness would 
# be pretty broad (you could probably exclude values <0.3, but that’s it), I would just use a uniform [0,1].

quantile1=list(p=.025, x=0.001)     # 2.5% quantile should be 0.01
quantile2=list(p=.975, x=0.03)      # 97.5% quantile should be 0.40
params<-beta.select(quantile1, quantile2)
hist(rbeta(n=1000000,shape1 = params[1],shape2 = params[2]),main="probResistance ~ beta(1.75,168.23)",xlab="",ylab="",breaks = 100)

hist(runif(100000),main="cRelInfectiousnessDRtoDS ~Unif(0,1)",breaks = 100)
